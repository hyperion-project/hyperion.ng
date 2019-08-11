// proj
#include <hyperion/SettingsManager.h>

// util
#include <utils/JsonUtils.h>
#include <db/SettingsTable.h>

// json schema process
#include <utils/jsonschema/QJsonFactory.h>
#include <utils/jsonschema/QJsonSchemaChecker.h>

// write config to filesystem
#include <utils/JsonUtils.h>

QJsonObject SettingsManager::schemaJson;

SettingsManager::SettingsManager(const quint8& instance, QObject* parent)
	: QObject(parent)
	, _log(Logger::getInstance("SettingsManager"))
	, _sTable(new SettingsTable(instance, this))
{
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

	// transform json to string lists
	QStringList keyList = defaultConfig.keys();
	QStringList defValueList;
	for(const auto key : keyList)
	{
		if(defaultConfig[key].isObject())
		{
			defValueList << QString(QJsonDocument(defaultConfig[key].toObject()).toJson(QJsonDocument::Compact));
		}
		else if(defaultConfig[key].isArray())
		{
			defValueList << QString(QJsonDocument(defaultConfig[key].toArray()).toJson(QJsonDocument::Compact));
		}
	}

	// fill database with default data if required
	for(const auto key : keyList)
	{
		QString val = defValueList.takeFirst();
		// prevent overwrite
		if(!_sTable->recordExist(key))
			_sTable->createSettingsRecord(key,val);
	}

	// need to validate all data in database constuct the entire data object
	// TODO refactor schemaChecker to accept QJsonArray in validate(); QJsonDocument container? To validate them per entry...
	QJsonObject dbConfig;
	for(const auto key : keyList)
	{
		QJsonDocument doc = _sTable->getSettingsRecord(key);
		if(doc.isArray())
			dbConfig[key] = doc.array();
		else
			dbConfig[key] = doc.object();
	}

	// validate full dbconfig against schema, on error we need to rewrite entire table
	QJsonSchemaChecker schemaChecker;
	schemaChecker.setSchema(schemaJson);
	QPair<bool,bool> valid = schemaChecker.validate(dbConfig);
	// check if our main schema syntax is IO
	if (!valid.second)
	{
		foreach (auto & schemaError, schemaChecker.getMessages())
			Error(_log, "Schema Syntax Error: %s", QSTRING_CSTR(schemaError));
		throw std::runtime_error("The config schema has invalid syntax. This should never happen! Go fix it!");
	}
	if (!valid.first)
	{
		Info(_log,"Table upgrade required...");
		dbConfig = schemaChecker.getAutoCorrectedConfig(dbConfig);

		foreach (auto & schemaError, schemaChecker.getMessages())
			Warning(_log, "Config Fix: %s", QSTRING_CSTR(schemaError));

		saveSettings(dbConfig);
	}
	else
		_qconfig = dbConfig;

	Debug(_log,"Settings database initialized")
}

const QJsonDocument SettingsManager::getSetting(const settings::type& type)
{
	return _sTable->getSettingsRecord(settings::typeToString(type));
}

bool SettingsManager::saveSettings(QJsonObject config, const bool& correct)
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

	// store the new config
	_qconfig = config;

	// extract keys and data
	QStringList keyList = config.keys();
	QStringList newValueList;
	for(const auto key : keyList)
	{
		if(config[key].isObject())
		{
			newValueList << QString(QJsonDocument(config[key].toObject()).toJson(QJsonDocument::Compact));
		}
		else if(config[key].isArray())
		{
			newValueList << QString(QJsonDocument(config[key].toArray()).toJson(QJsonDocument::Compact));
		}
	}

	// compare database data with new data to emit/save changes accordingly
	for(const auto key : keyList)
	{
		QString data = newValueList.takeFirst();
		if(_sTable->getSettingsRecordString(key) != data)
		{
			_sTable->createSettingsRecord(key, data);

			emit settingsChanged(settings::stringToType(key), QJsonDocument::fromJson(data.toLocal8Bit()));
		}
	}
	return true;
}
