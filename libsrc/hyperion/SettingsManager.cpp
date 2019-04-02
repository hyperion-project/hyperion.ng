// proj
#include <hyperion/SettingsManager.h>

// util
#include <utils/JsonUtils.h>

// json schema process
#include <utils/jsonschema/QJsonFactory.h>
#include <utils/jsonschema/QJsonSchemaChecker.h>

// write config to filesystem
#include <utils/JsonUtils.h>

// hyperion
#include <hyperion/Hyperion.h>

QJsonObject SettingsManager::schemaJson;

SettingsManager::SettingsManager(Hyperion* hyperion, const quint8& instance, const QString& configFile)
	: _hyperion(hyperion)
	, _log(Logger::getInstance("SettingsManager"))
{
	connect(this, &SettingsManager::settingsChanged, _hyperion, &Hyperion::settingsChanged);
	// get schema
	if(schemaJson.isEmpty())
	{
		Q_INIT_RESOURCE(resource);
		try
		{
			schemaJson = QJsonFactory::readSchema(":/hyperion-schema");
		}
		catch(const std::runtime_error& error)
		{
			throw std::runtime_error(error.what());
		}
	}
	// get default config
	QJsonObject defaultConfig;
	if(!JsonUtils::readFile(":/hyperion_default.config", defaultConfig, _log))
		throw std::runtime_error("Failed to read default config");

	Info(_log, "Selected configuration file: %s", QSTRING_CSTR(configFile));
	QJsonSchemaChecker schemaCheckerT;
	schemaCheckerT.setSchema(schemaJson);

	if(!JsonUtils::readFile(configFile, _qconfig, _log))
		throw std::runtime_error("Failed to load config!");

	// validate config with schema and correct it if required
	QPair<bool, bool> validate = schemaCheckerT.validate(_qconfig);

	// errors in schema syntax, abort
	if (!validate.second)
	{
		foreach (auto & schemaError, schemaCheckerT.getMessages())
			Error(_log, "Schema Syntax Error: %s", QSTRING_CSTR(schemaError));

		throw std::runtime_error("ERROR: Hyperion schema has syntax errors!");
	}
	// errors in configuration, correct it!
	if (!validate.first)
	{
		Warning(_log,"Errors have been found in the configuration file. Automatic correction has been applied");
		_qconfig = schemaCheckerT.getAutoCorrectedConfig(_qconfig);

		foreach (auto & schemaError, schemaCheckerT.getMessages())
			Warning(_log, "Config Fix: %s", QSTRING_CSTR(schemaError));

		if (!JsonUtils::write(configFile, _qconfig, _log))
			throw std::runtime_error("ERROR: Can't save configuration file, aborting");
	}

	Debug(_log,"Settings database initialized")
}

SettingsManager::SettingsManager(const quint8& instance, const QString& configFile)
	: _hyperion(nullptr)
	, _log(Logger::getInstance("SettingsManager"))
{
	Q_INIT_RESOURCE(resource);
	// get schema
	if(schemaJson.isEmpty())
	{
		try
		{
			schemaJson = QJsonFactory::readSchema(":/hyperion-schema");
		}
		catch(const std::runtime_error& error)
		{
			throw std::runtime_error(error.what());
		}
	}
	// get default config
	QJsonObject defaultConfig;
	if(!JsonUtils::readFile(":/hyperion_default.config", defaultConfig, _log))
		throw std::runtime_error("Failed to read default config");

	Info(_log, "Selected configuration file: %s", QSTRING_CSTR(configFile));
	QJsonSchemaChecker schemaCheckerT;
	schemaCheckerT.setSchema(schemaJson);

	if(!JsonUtils::readFile(configFile, _qconfig, _log))
		throw std::runtime_error("Failed to load config!");

	// validate config with schema and correct it if required
	QPair<bool, bool> validate = schemaCheckerT.validate(_qconfig);

	// errors in schema syntax, abort
	if (!validate.second)
	{
		foreach (auto & schemaError, schemaCheckerT.getMessages())
			Error(_log, "Schema Syntax Error: %s", QSTRING_CSTR(schemaError));

		throw std::runtime_error("ERROR: Hyperion schema has syntax errors!");
	}
	// errors in configuration, correct it!
	if (!validate.first)
	{
		Warning(_log,"Errors have been found in the configuration file. Automatic correction has been applied");
		_qconfig = schemaCheckerT.getAutoCorrectedConfig(_qconfig);

		foreach (auto & schemaError, schemaCheckerT.getMessages())
			Warning(_log, "Config Fix: %s", QSTRING_CSTR(schemaError));

		if (!JsonUtils::write(configFile, _qconfig, _log))
			throw std::runtime_error("ERROR: Can't save configuration file, aborting");
	}

	Debug(_log,"Settings database initialized")
}

SettingsManager::~SettingsManager()
{
}

const QJsonDocument SettingsManager::getSetting(const settings::type& type)
{
	QString key = settings::typeToString(type);
	if(_qconfig[key].isObject())
		return QJsonDocument(_qconfig[key].toObject());
	else
		return QJsonDocument(_qconfig[key].toArray());
}

const bool SettingsManager::saveSettings(QJsonObject config, const bool& correct)
{
	// we need to validate data against schema
	QJsonSchemaChecker schemaChecker;
	schemaChecker.setSchema(schemaJson);
	if (!schemaChecker.validate(config).first)
	{
		if(!correct)
		{
			Error(_log,"Failed to save configuration, errors during validation");
			return false;
		}
		Warning(_log,"Fixing json data!");
		config = schemaChecker.getAutoCorrectedConfig(config);

		foreach (auto & schemaError, schemaChecker.getMessages())
			Warning(_log, "Config Fix: %s", QSTRING_CSTR(schemaError));
	}

	// save data to file
	if(_hyperion != nullptr)
	{
		if(!JsonUtils::write(_hyperion->getConfigFilePath(), config, _log))
			return false;
	}

	// compare old data with new data to emit/save changes accordingly
	for(const auto key : config.keys())
	{
		QString newData, oldData;

		_qconfig[key].isObject()
		? oldData = QString(QJsonDocument(_qconfig[key].toObject()).toJson(QJsonDocument::Compact))
		: oldData = QString(QJsonDocument(_qconfig[key].toArray()).toJson(QJsonDocument::Compact));

		config[key].isObject()
		? newData = QString(QJsonDocument(config[key].toObject()).toJson(QJsonDocument::Compact))
		: newData = QString(QJsonDocument(config[key].toArray()).toJson(QJsonDocument::Compact));

		if(oldData != newData)
			emit settingsChanged(settings::stringToType(key), QJsonDocument::fromJson(newData.toLocal8Bit()));
	}

	// store the current state
	_qconfig = config;

	return true;
}
