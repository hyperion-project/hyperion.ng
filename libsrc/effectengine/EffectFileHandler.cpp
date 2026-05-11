#include <effectengine/EffectFileHandler.h>

#include <algorithm>

#include <QJsonArray>
#include <QFileInfo>
#include <QDir>
#include <QMap>
#include <QByteArray>
#include <QString>

#include <utils/MemoryTracker.h>
#include <utils/JsonUtils.h>

QSharedPointer<EffectFileHandler> EffectFileHandler::_instance;

EffectFileHandler::EffectFileHandler(const QString& rootPath, const QJsonDocument& effectConfig, QObject* parent)
	: QObject(parent)
	, _log(Logger::getInstance("EFFECTFILES"))
	, _rootPath(rootPath)
{

	Q_INIT_RESOURCE(EffectEngine);

	// init
	handleSettingsUpdate(settings::EFFECTS, effectConfig);
}

void EffectFileHandler::createInstance(const QString& rootPath, const QJsonDocument& effectConfig, QObject* parent)
{
	CREATE_INSTANCE_WITH_TRACKING(_instance, EffectFileHandler, parent, rootPath, effectConfig, nullptr);
}

QSharedPointer<EffectFileHandler> EffectFileHandler::getInstance()
{
	return _instance;
}

bool EffectFileHandler::isValid()
{
	return !_instance.isNull();
}

void EffectFileHandler::destroyInstance()
{
	_instance.reset();
}

void EffectFileHandler::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if (type == settings::EFFECTS)
	{
		_effectConfig = config.object();

		QJsonArray effectPathArray = _effectConfig["paths"].toArray();
		if (effectPathArray.empty())
		{
			effectPathArray.append("$ROOT/custom-effects");
		}
		_effectConfig["paths"] = effectPathArray;

		// update effects and schemas
		updateEffects();
	}
}

QString EffectFileHandler::deleteEffect(const QString& effectName)
{
	const auto& effectsDefinition = getEffects();
	auto it = effectsDefinition.cbegin();
	for (; it != effectsDefinition.cend(); ++it)
	{
		if (it->name == effectName)
			break;
	}

	if (it == effectsDefinition.cend())
		return QString("Effect %1 not found").arg(effectName);

	QFileInfo effectConfigurationFile(it->file);
	if (effectConfigurationFile.absoluteFilePath().startsWith(':'))
		return QString("Can't delete internal effect: %1").arg(effectName);

	return removeEffectFiles(*it, effectConfigurationFile);
}

QString EffectFileHandler::removeEffectFiles(const EffectDefinition& effect, const QFileInfo& effectConfigurationFile)
{
	if (!effectConfigurationFile.exists())
		return QString("Can't find effect configuration file: %1").arg(effectConfigurationFile.absoluteFilePath());

	if ((effect.script == ":/effects/gif.py") && !effect.args.value("file").toString("").isEmpty())
	{
		QFileInfo effectImageFile(effect.args.value("file").toString());
		if (effectImageFile.exists())
		{
			QFile::remove(effectImageFile.absoluteFilePath());
		}
	}

	if (!QFile::remove(effectConfigurationFile.absoluteFilePath()))
	{
		return QString("Can't delete effect configuration file: %1. Please check permissions").arg(effectConfigurationFile.absoluteFilePath());
	}

	updateEffects();
	return "";
}

QString EffectFileHandler::saveEffect(const QJsonObject& message)
{
	if (message["args"].toObject().isEmpty())
		return "Missing or empty Object 'args'";

	const QString scriptName = message["script"].toString();
	const auto& effectsSchemas = getEffectSchemas();
	auto it = effectsSchemas.cbegin();
	for (; it != effectsSchemas.cend(); ++it)
	{
		if (it->pyFile == scriptName)
			break;
	}

	if (it == effectsSchemas.cend())
		return QString("Missing schema file for Python script %1").arg(message["script"].toString());

	if (!JsonUtils::validate("EffectFileHandler", message["args"], it->schemaFile, _log).first)
		return "Error during arg validation against schema, please consult the Hyperion Log";

	const QJsonArray effectArray = _effectConfig["paths"].toArray();
	if (effectArray.empty())
		return "Cannot save new effect. Effect path is empty";

	const QString effectName = message["name"].toString();
	if (effectName.trimmed().isEmpty() || effectName.trimmed().startsWith(":"))
		return "Cannot save new effect. Effect name is empty or begins with a colon (internal-effect prefix).";

	QJsonObject effectJson;
	effectJson["name"]   = effectName;
	effectJson["script"] = scriptName;
	effectJson["args"]   = message["args"].toObject();

	const QString resultMsg = resolveEffectFilePath(message, effectName, effectArray, effectJson);
	if (!resultMsg.isEmpty())
		return resultMsg;

	Info(_log, "Reload effect list");
	updateEffects();
	return "";
}

QString EffectFileHandler::resolveEffectFilePath(const QJsonObject& message, const QString& effectName, const QJsonArray& effectArray, QJsonObject& effectJson)
{
	QList<EffectDefinition> availableEffects = getEffects();
	auto iter = std::find_if(availableEffects.cbegin(), availableEffects.cend(),
		[&effectName](const EffectDefinition& effectDefinition) { return effectDefinition.name == effectName; }
	);

	QFileInfo newFileName;
	if (iter != availableEffects.cend())
	{
		newFileName.setFile(iter->file);
		if (newFileName.absoluteFilePath().startsWith(':'))
			return QString("The effect name '%1' is assigned to an internal effect. Please rename your effect.").arg(effectName);
	}
	else
	{
		const QString sanitizedName = QString(effectName).replace(QString(" "), QString(""));
		if (sanitizedName.contains('/') || sanitizedName.contains('\\') || sanitizedName.contains(".."))
			return QString("The effect name '%1' contains invalid characters.").arg(effectName);
		const QString f = effectArray[0].toString().replace("$ROOT", _rootPath) + '/' + sanitizedName + QString(".json");
		newFileName.setFile(f);
	}

	const QString imgResult = saveEffectImage(message, effectArray, effectJson);
	if (!imgResult.isEmpty())
		return imgResult;

	cleanImageSource(message, effectJson);

	if (!JsonUtils::write(newFileName.absoluteFilePath(), effectJson, _log))
		return "Error while saving effect, please check the Hyperion Log";

	return "";
}

QString EffectFileHandler::saveEffectImage(const QJsonObject& message, const QJsonArray& effectArray, QJsonObject& effectJson)
{
	if (message["imageData"].toString("").isEmpty() || message["args"].toObject().value("file").toString("").isEmpty())
		return "";

	QJsonObject args = message["args"].toObject();
	const QString effectsDirPath = effectArray[0].toString().replace("$ROOT", _rootPath);
	const QString effectsDirAbs = QFileInfo(effectsDirPath).absoluteFilePath();
	const QString imageFilePath = effectsDirPath + '/' + args.value("file").toString();
	const QFileInfo imageFileName(imageFilePath);
	if (!imageFileName.absoluteFilePath().startsWith(effectsDirAbs + '/'))
		return QString("Invalid image file path '%1'.").arg(args.value("file").toString());

	if (!FileUtils::writeFile(imageFileName.absoluteFilePath(), QByteArray::fromBase64(message["imageData"].toString("").toUtf8()), _log))
		return QString("Error while saving image file '%1', please check the Hyperion Log").arg(message["args"].toObject().value("file").toString());

	args["file"] = imageFilePath;
	effectJson["args"] = args;
	return "";
}

void EffectFileHandler::cleanImageSource(const QJsonObject& message, QJsonObject& effectJson) const
{
	const QString imageSource = message["args"].toObject().value("imageSource").toString("");
	qCDebug(effect) << "Image source:" << imageSource;
	if (imageSource != "url" && imageSource != "file")
		return;

	QJsonObject args = message["args"].toObject();
	args.remove(imageSource == "url" ? "file" : "url");
	effectJson["args"] = args;
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
	for (const QString& path : std::as_const(efxPathList))
	{
		QDir directory(path);
		if (!directory.exists())
		{
			if (directory.mkpath(path))
				Info(_log, "New Effect path \"%s\" created successfully", QSTRING_CSTR(path));
			else
				Warning(_log, "Failed to create Effect path \"%s\", please check permissions", QSTRING_CSTR(path));
		}
		else
		{
			loadEffectsFromDirectory(path, directory, disableList, availableEffects);
			loadSchemasFromDirectory(path, directory);
		}
	}

	for (const auto& item : std::as_const(availableEffects))
	{
		_availableEffects.push_back(item);
	}

	ErrorIf(_availableEffects.empty(), _log, "no effects found, check your effect directories");

	emit effectListChanged();
}

void EffectFileHandler::loadEffectsFromDirectory(const QString& path, const QDir& directory, const QStringList& disableList, QMap<QString, EffectDefinition>& availableEffects)
{
	int efxCount = 0;
	const QStringList filenames = directory.entryList(QStringList() << "*.json", QDir::Files, QDir::Name | QDir::IgnoreCase);
	for (const QString& filename : filenames)
	{
		EffectDefinition def;
		if (!loadEffectDefinition(path, filename, def))
			continue;

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
	Info(_log, "%d effects loaded from directory %s", efxCount, QSTRING_CSTR(path));
}

void EffectFileHandler::loadSchemasFromDirectory(const QString& path, QDir& directory)
{
	int efxCount = 0;
	const QString schemaPath = path + "schema" + '/';
	directory.setPath(schemaPath);
	const QStringList schemaFileNames = directory.entryList(QStringList() << "*.json", QDir::Files, QDir::Name | QDir::IgnoreCase);
	for (const QString& schemaFileName : schemaFileNames)
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

bool EffectFileHandler::loadEffectDefinition(const QString& path, const QString& effectConfigFile, EffectDefinition& effectDefinition)
{
	QString fileName = path + effectConfigFile;

	// Read and parse the effect json config file
	QJsonObject configEffect;
	if (!JsonUtils::readFile(fileName, configEffect, _log).first) {
		return false;
	}

	// validate effect config with effect schema(path)
	if (!JsonUtils::validate(fileName, configEffect, ":effect-schema", _log).first) {
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
	if (!JsonUtils::readFile(schemaFilePath, schemaEffect, _log).first)
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
