#include <effectengine/EffectFileHandler.h>

// util
#include <utils/JsonUtils.h>

// qt
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>
#include <QMap>
#include <QByteArray>

EffectFileHandler* EffectFileHandler::efhInstance;

EffectFileHandler::EffectFileHandler(const QString& rootPath, const QJsonDocument& effectConfig, QObject* parent)
	: QObject(parent)
	, _log(Logger::getInstance("EFFECTFILES"))
	, _rootPath(rootPath)
{
	EffectFileHandler::efhInstance = this;

	Q_INIT_RESOURCE(EffectEngine);

	// init
	handleSettingsUpdate(settings::EFFECTS, effectConfig);
}

void EffectFileHandler::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if (type == settings::EFFECTS)
	{
		_effectConfig = config.object();
		// update effects and schemas
		updateEffects();
	}
}

QString EffectFileHandler::deleteEffect(const QString& effectName)
{
	QString resultMsg;
	std::list<EffectDefinition> effectsDefinition = getEffects();
	std::list<EffectDefinition>::iterator it = std::find_if(effectsDefinition.begin(), effectsDefinition.end(),
		[&effectName](const EffectDefinition& effectDefinition) {return effectDefinition.name == effectName; }
	);

	if (it != effectsDefinition.end())
	{
		QFileInfo effectConfigurationFile(it->file);
		if (!effectConfigurationFile.absoluteFilePath().startsWith(':'))
		{
			if (effectConfigurationFile.exists())
			{
				if ((it->script == ":/effects/gif.py") && !it->args.value("file").toString("").isEmpty())
				{
					QFileInfo effectImageFile(it->args.value("file").toString());
					if (effectImageFile.exists())
					{
						QFile::remove(effectImageFile.absoluteFilePath());
					}
				}

				bool result = QFile::remove(effectConfigurationFile.absoluteFilePath());

				if (result)
				{
					updateEffects();
					resultMsg = "";
				}
				else
				{
					resultMsg = "Can't delete effect configuration file: " + effectConfigurationFile.absoluteFilePath() + ". Please check permissions";
				}
			}
			else
			{
				resultMsg = "Can't find effect configuration file: " + effectConfigurationFile.absoluteFilePath();
			}
		}
		else
		{
			resultMsg = "Can't delete internal effect: " + effectName;
		}
	}
	else
	{
		resultMsg = "Effect " + effectName + " not found";
	}

	return resultMsg;
}

QString EffectFileHandler::saveEffect(const QJsonObject& message)
{
	QString resultMsg;
	if (!message["args"].toObject().isEmpty())
	{
		QString scriptName = message["script"].toString();

		std::list<EffectSchema> effectsSchemas = getEffectSchemas();
		std::list<EffectSchema>::iterator it = std::find_if(effectsSchemas.begin(), effectsSchemas.end(),
			[&scriptName](const EffectSchema& schema) {return schema.pyFile == scriptName; }
		);

		if (it != effectsSchemas.end())
		{
			if (!JsonUtils::validate("EffectFileHandler", message["args"].toObject(), it->schemaFile, _log))
			{
				return "Error during arg validation against schema, please consult the Hyperion Log";
			}

			QJsonObject effectJson;
			QJsonArray effectArray;
			effectArray = _effectConfig["paths"].toArray();

			if (!effectArray.empty())
			{
				QString effectName = message["name"].toString();
				if (effectName.trimmed().isEmpty() || effectName.trimmed().startsWith(":"))
				{
					return "Can't save new effect. Effect name is empty or begins with a dot.";
				}

				effectJson["name"] = effectName;
				effectJson["script"] = message["script"].toString();
				effectJson["args"] = message["args"].toObject();

				std::list<EffectDefinition> availableEffects = getEffects();
				std::list<EffectDefinition>::iterator iter = std::find_if(availableEffects.begin(), availableEffects.end(),
					[&effectName](const EffectDefinition& effectDefinition) {return effectDefinition.name == effectName; }
				);

				QFileInfo newFileName;
				if (iter != availableEffects.end())
				{
					newFileName.setFile(iter->file);
					if (newFileName.absoluteFilePath().startsWith(':'))
					{
						return "The effect name '" + effectName + "' is assigned to an internal effect. Please rename your effect.";
					}
				}
				else
				{
					QString f = effectArray[0].toString().replace("$ROOT", _rootPath) + '/' + effectName.replace(QString(" "), QString("")) + QString(".json");
					newFileName.setFile(f);
				}

				if (!message["imageData"].toString("").isEmpty() && !message["args"].toObject().value("file").toString("").isEmpty())
				{
					QJsonObject args = message["args"].toObject();
					QString imageFilePath = effectArray[0].toString().replace("$ROOT", _rootPath) + '/' + args.value("file").toString();

					QFileInfo imageFileName(imageFilePath);
					if (!FileUtils::writeFile(imageFileName.absoluteFilePath(), QByteArray::fromBase64(message["imageData"].toString("").toUtf8()), _log))
					{
						return "Error while saving image file '" + message["args"].toObject().value("file").toString() + ", please check the Hyperion Log";
					}

					//Update json with image file location
					args["file"] = imageFilePath;
					effectJson["args"] = args;
				}

				if (message["args"].toObject().value("imageSource").toString("") == "url" || message["args"].toObject().value("imageSource").toString("") == "file")
				{
					QJsonObject args = message["args"].toObject();
					args.remove(args.value("imageSource").toString("") == "url" ? "file" : "url");
					effectJson["args"] = args;
				}

				if (!JsonUtils::write(newFileName.absoluteFilePath(), effectJson, _log))
				{
					return "Error while saving effect, please check the Hyperion Log";
				}

				Info(_log, "Reload effect list");
				updateEffects();
				resultMsg = "";
			}
			else
			{
				resultMsg = "Can't save new effect. Effect path empty";
			}
		}
		else
		{
			resultMsg = "Missing schema file for Python script " + message["script"].toString();
		}
	}
	else
	{
		resultMsg = "Missing or empty Object 'args'";
	}

	return resultMsg;
}

void EffectFileHandler::updateEffects()
{
	// clear all lists
	_availableEffects.clear();
	_effectSchemas.clear();

	// read all effects
	const QJsonArray& paths = _effectConfig["paths"].toArray();
	const QJsonArray& disabledEfx = _effectConfig["disable"].toArray();

	QStringList efxPathList;
	efxPathList << ":/effects/";
	QStringList disableList;

	for (const auto& p : paths)
	{
		QString effectPath = p.toString();
		if (!effectPath.endsWith('/'))
		{
			effectPath.append('/');
		}
		efxPathList << effectPath.replace("$ROOT", _rootPath);
	}

	for (const auto& efx : disabledEfx)
	{
		disableList << efx.toString();
	}

	QMap<QString, EffectDefinition> availableEffects;
	for (const QString& path : qAsConst(efxPathList))
	{
		QDir directory(path);
		if (!directory.exists())
		{
			if (directory.mkpath(path))
			{
				Info(_log, "New Effect path \"%s\" created successfully", QSTRING_CSTR(path));
			}
			else
			{
				Warning(_log, "Failed to create Effect path \"%s\", please check permissions", QSTRING_CSTR(path));
			}
		}
		else
		{
			int efxCount = 0;
			QStringList filenames = directory.entryList(QStringList() << "*.json", QDir::Files, QDir::Name | QDir::IgnoreCase);
			for (const QString& filename : qAsConst(filenames))
			{
				EffectDefinition def;
				if (loadEffectDefinition(path, filename, def))
				{
					InfoIf(availableEffects.find(def.name) != availableEffects.end(), _log,
						"effect overload effect '%s' is now taken from '%s'", QSTRING_CSTR(def.name), QSTRING_CSTR(path));

					if (disableList.contains(def.name))
					{
						Info(_log, "effect '%s' not loaded, because it is disabled in hyperion config", QSTRING_CSTR(def.name));
					}
					else
					{
						availableEffects[def.name] = def;
						efxCount++;
					}
				}
			}
			Info(_log, "%d effects loaded from directory %s", efxCount, QSTRING_CSTR(path));

			// collect effect schemas
			efxCount = 0;

			QString schemaPath = path + "schema" + '/';
			directory.setPath(schemaPath);
			QStringList schemaFileNames = directory.entryList(QStringList() << "*.json", QDir::Files, QDir::Name | QDir::IgnoreCase);
			for (const QString& schemaFileName : qAsConst(schemaFileNames))
			{
				EffectSchema pyEffect;
				if (loadEffectSchema(path, directory.filePath(schemaFileName), pyEffect))
				{
					_effectSchemas.push_back(pyEffect);
					efxCount++;
				}
			}
			InfoIf(efxCount > 0, _log, "%d effect schemas loaded from directory %s", efxCount, QSTRING_CSTR(schemaPath));
		}
	}

	for (const auto& item : qAsConst(availableEffects))
	{
		_availableEffects.push_back(item);
	}

	ErrorIf(_availableEffects.empty(), _log, "no effects found, check your effect directories");

	emit effectListChanged();
}

bool EffectFileHandler::loadEffectDefinition(const QString& path, const QString& effectConfigFile, EffectDefinition& effectDefinition)
{
	QString fileName = path + effectConfigFile;

	// Read and parse the effect json config file
	QJsonObject configEffect;
	if (!JsonUtils::readFile(fileName, configEffect, _log)) {
		return false;
	}

	// validate effect config with effect schema(path)
	if (!JsonUtils::validate(fileName, configEffect, ":effect-schema", _log)) {
		return false;
	}

	// setup the definition
	effectDefinition.file = fileName;
	QJsonObject config = configEffect;
	QString scriptName = config["script"].toString();
	effectDefinition.name = config["name"].toString();
	if (scriptName.isEmpty()) {
		return false;
	}

	QFile fileInfo(scriptName);
	if (!fileInfo.exists())
	{
		effectDefinition.script = path + scriptName;
	}
	else
	{
		effectDefinition.script = scriptName;
	}

	effectDefinition.args = config["args"].toObject();
	effectDefinition.smoothCfg = 1; // pause config
	return true;
}

bool EffectFileHandler::loadEffectSchema(const QString& path, const QString& schemaFilePath, EffectSchema& effectSchema)
{
	// Read and parse the effect schema file
	QJsonObject schemaEffect;
	if (!JsonUtils::readFile(schemaFilePath, schemaEffect, _log))
	{
		return false;
	}

	// setup the definition
	QString scriptName = schemaEffect["script"].toString();
	effectSchema.schemaFile = schemaFilePath;

	QString scriptFilePath = path + scriptName;
	QFile pyScriptFile(scriptFilePath);

	if (scriptName.isEmpty() || !pyScriptFile.open(QIODevice::ReadOnly))
	{
		Error(_log, "Python script '%s' in effect schema '%s' could not be loaded", QSTRING_CSTR(scriptName), QSTRING_CSTR(schemaFilePath));
		return false;
	}

	pyScriptFile.close();

	effectSchema.pyFile = scriptFilePath;
	effectSchema.pySchema = schemaEffect;

	return true;
}
