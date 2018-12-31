#include <effectengine/EffectFileHandler.h>

// util
#include <utils/JsonUtils.h>

// qt
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>
#include <QMap>

// createEffect helper
struct find_schema: std::unary_function<EffectSchema, bool>
{
	QString pyFile;
	find_schema(QString pyFile):pyFile(pyFile) { }
	bool operator()(EffectSchema const& schema) const
	{
		return schema.pyFile == pyFile;
	}
};

// deleteEffect helper
struct find_effect: std::unary_function<EffectDefinition, bool>
{
	QString effectName;
	find_effect(QString effectName) :effectName(effectName) { }
	bool operator()(EffectDefinition const& effectDefinition) const
	{
		return effectDefinition.name == effectName;
	}
};

EffectFileHandler* EffectFileHandler::efhInstance;

EffectFileHandler::EffectFileHandler(const QString& rootPath, const QJsonDocument& effectConfig, QObject* parent)
	: QObject(parent)
	, _effectConfig()
	, _log(Logger::getInstance("EFFECTFILES"))
	, _rootPath(rootPath)
{
	EffectFileHandler::efhInstance = this;

	Q_INIT_RESOURCE(EffectEngine);

	// init
	handleSettingsUpdate(settings::EFFECTS, effectConfig);
}

void EffectFileHandler::handleSettingsUpdate(const settings::type& type, const QJsonDocument& config)
{
	if(type == settings::EFFECTS)
	{
		_effectConfig = config.object();
		// update effects and schemas
		updateEffects();
	}
}

const bool EffectFileHandler::deleteEffect(const QString& effectName, QString& resultMsg)
{
	std::list<EffectDefinition> effectsDefinition = getEffects();
	std::list<EffectDefinition>::iterator it = std::find_if(effectsDefinition.begin(), effectsDefinition.end(), find_effect(effectName));

	if (it != effectsDefinition.end())
	{
		QFileInfo effectConfigurationFile(it->file);
		if (effectConfigurationFile.absoluteFilePath().mid(0, 1)  != ":" )
		{
			if (effectConfigurationFile.exists())
			{
				bool result = QFile::remove(effectConfigurationFile.absoluteFilePath());
				if (result)
				{
					updateEffects();
					return true;
				} else
					resultMsg = "Can't delete effect configuration file: " + effectConfigurationFile.absoluteFilePath() + ". Please check permissions";
			} else
				resultMsg = "Can't find effect configuration file: " + effectConfigurationFile.absoluteFilePath();
		} else
			resultMsg = "Can't delete internal effect: " + effectName;
	} else
		resultMsg = "Effect " + effectName + " not found";

	return false;
}

const bool EffectFileHandler::saveEffect(const QJsonObject& message, QString& resultMsg)
{
	if (!message["args"].toObject().isEmpty())
	{
		QString scriptName;
		(message["script"].toString().mid(0, 1)  == ":" )
			? scriptName = ":/effects//" + message["script"].toString().mid(1)
			: scriptName = message["script"].toString();

		std::list<EffectSchema> effectsSchemas = getEffectSchemas();
		std::list<EffectSchema>::iterator it = std::find_if(effectsSchemas.begin(), effectsSchemas.end(), find_schema(scriptName));

		if (it != effectsSchemas.end())
		{
			if(!JsonUtils::validate("EffectFileHandler", message["args"].toObject(), it->schemaFile, _log))
			{
				resultMsg = "Error during arg validation against schema, please consult the Hyperion Log";
				return false;
			}

			QJsonObject effectJson;
			QJsonArray effectArray;
			effectArray = _effectConfig["paths"].toArray();

			if (effectArray.size() > 0)
			{
				if (message["name"].toString().trimmed().isEmpty() || message["name"].toString().trimmed().startsWith("."))
				{
					resultMsg = "Can't save new effect. Effect name is empty or begins with a dot.";
					return false;
				}

				effectJson["name"] = message["name"].toString();
				effectJson["script"] = message["script"].toString();
				effectJson["args"] = message["args"].toObject();

				std::list<EffectDefinition> availableEffects = getEffects();
				std::list<EffectDefinition>::iterator iter = std::find_if(availableEffects.begin(), availableEffects.end(), find_effect(message["name"].toString()));

				QFileInfo newFileName;
				if (iter != availableEffects.end())
				{
					newFileName.setFile(iter->file);
					if (newFileName.absoluteFilePath().mid(0, 1)  == ":")
					{
						resultMsg = "The effect name '" + message["name"].toString() + "' is assigned to an internal effect. Please rename your effekt.";
						return false;
					}
				} else
				{
					// TODO global special keyword handling
					QString f = effectArray[0].toString().replace("$ROOT",_rootPath) + "/" + message["name"].toString().replace(QString(" "), QString("")) + QString(".json");
					newFileName.setFile(f);
				}

				if(!JsonUtils::write(newFileName.absoluteFilePath(), effectJson, _log))
				{
					resultMsg = "Error while saving effect, please check the Hyperion Log";
					return false;
				}

				Info(_log, "Reload effect list");
				updateEffects();
				resultMsg = "";
				return true;
			} else
				resultMsg = "Can't save new effect. Effect path empty";
		} else
			resultMsg = "Missing schema file for Python script " + message["script"].toString();
	} else
		resultMsg = "Missing or empty Object 'args'";

	return false;
}

void EffectFileHandler::updateEffects()
{
	// clear all lists
	_availableEffects.clear();
	_effectSchemas.clear();

	// read all effects
	const QJsonArray & paths       = _effectConfig["paths"].toArray();
	const QJsonArray & disabledEfx = _effectConfig["disable"].toArray();

	QStringList efxPathList;
	efxPathList << ":/effects/";
	QStringList disableList;

	for(auto p : paths)
	{
		efxPathList << p.toString().replace("$ROOT",_rootPath);
	}
	for(auto efx : disabledEfx)
	{
		disableList << efx.toString();
	}

	QMap<QString, EffectDefinition> availableEffects;
	for (const QString & path : efxPathList )
	{
		QDir directory(path);
		if (!directory.exists())
		{
			if(directory.mkpath(path))
			{
				Info(_log, "New Effect path \"%s\" created successfull", QSTRING_CSTR(path) );
			}
			else
			{
				Warning(_log, "Failed to create Effect path \"%s\", please check permissions", QSTRING_CSTR(path) );
			}
		}
		else
		{
			int efxCount = 0;
			QStringList filenames = directory.entryList(QStringList() << "*.json", QDir::Files, QDir::Name | QDir::IgnoreCase);
			for (const QString & filename : filenames)
			{
				EffectDefinition def;
				if (loadEffectDefinition(path, filename, def))
				{
					InfoIf(availableEffects.find(def.name) != availableEffects.end(), _log,
						"effect overload effect '%s' is now taken from '%s'", QSTRING_CSTR(def.name), QSTRING_CSTR(path) );

					if ( disableList.contains(def.name) )
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
			directory = path.endsWith("/") ? (path + "schema/") : (path + "/schema/");
			QStringList pynames = directory.entryList(QStringList() << "*.json", QDir::Files, QDir::Name | QDir::IgnoreCase);
			for (const QString & pyname : pynames)
			{
				EffectSchema pyEffect;
				if (loadEffectSchema(path, pyname, pyEffect))
				{
					_effectSchemas.push_back(pyEffect);
					efxCount++;
				}
			}
			InfoIf(efxCount > 0, _log, "%d effect schemas loaded from directory %s", efxCount, QSTRING_CSTR((path + "schema/")));
		}
	}

	for(auto item : availableEffects)
	{
		_availableEffects.push_back(item);
	}

	ErrorIf(_availableEffects.size()==0, _log, "no effects found, check your effect directories");

	emit effectListChanged();
}

bool EffectFileHandler::loadEffectDefinition(const QString &path, const QString &effectConfigFile, EffectDefinition & effectDefinition)
{
	QString fileName = path + QDir::separator() + effectConfigFile;

	// Read and parse the effect json config file
	QJsonObject configEffect;
	if(!JsonUtils::readFile(fileName, configEffect, _log))
		return false;

	// validate effect config with effect schema(path)
	if(!JsonUtils::validate(fileName, configEffect, ":effect-schema", _log))
		return false;

	// setup the definition
	effectDefinition.file = fileName;
	QJsonObject config = configEffect;
	QString scriptName = config["script"].toString();
	effectDefinition.name = config["name"].toString();
	if (scriptName.isEmpty())
		return false;

	QFile fileInfo(scriptName);

	if (scriptName.mid(0, 1)  == ":" )
	{
		(!fileInfo.exists())
		? effectDefinition.script = ":/effects/"+scriptName.mid(1)
		: effectDefinition.script = scriptName;
	} else
	{
		(!fileInfo.exists())
		? effectDefinition.script = path + QDir::separator() + scriptName
		: effectDefinition.script = scriptName;
	}

	effectDefinition.args = config["args"].toObject();
	effectDefinition.smoothCfg = 1; // pause config
	return true;
}

bool EffectFileHandler::loadEffectSchema(const QString &path, const QString &effectSchemaFile, EffectSchema & effectSchema)
{
	QString fileName = path + "schema/" + QDir::separator() + effectSchemaFile;

	// Read and parse the effect schema file
	QJsonObject schemaEffect;
	if(!JsonUtils::readFile(fileName, schemaEffect, _log))
		return false;

	// setup the definition
	QString scriptName = schemaEffect["script"].toString();
	effectSchema.schemaFile = fileName;
	fileName = path + QDir::separator() + scriptName;
	QFile pyFile(fileName);

	if (scriptName.isEmpty() || !pyFile.open(QIODevice::ReadOnly))
	{
		fileName = path + "schema/" + QDir::separator() + effectSchemaFile;
		Error( _log, "Python script '%s' in effect schema '%s' could not be loaded", QSTRING_CSTR(scriptName), QSTRING_CSTR(fileName));
		return false;
	}

	pyFile.close();

	effectSchema.pyFile = (scriptName.mid(0, 1)  == ":" ) ? ":/effects/"+scriptName.mid(1) : path + QDir::separator() + scriptName;
	effectSchema.pySchema = schemaEffect;

	return true;
}
