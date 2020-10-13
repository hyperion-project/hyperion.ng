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

SettingsManager::SettingsManager(quint8 instance, QObject* parent, bool readonlyMode)
	: QObject(parent)
	, _log(Logger::getInstance("SETTINGSMGR"))
	, _sTable(new SettingsTable(instance, this))
	, _readonlyMode(readonlyMode)
{
	_sTable->setReadonlyMode(_readonlyMode);
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
	for(const auto & key : keyList)
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
	for(const auto & key : keyList)
	{
		QString val = defValueList.takeFirst();
		// prevent overwrite
		if(!_sTable->recordExist(key))
			_sTable->createSettingsRecord(key,val);
	}

	// need to validate all data in database constuct the entire data object
	// TODO refactor schemaChecker to accept QJsonArray in validate(); QJsonDocument container? To validate them per entry...
	QJsonObject dbConfig;
	for(const auto & key : keyList)
	{
		QJsonDocument doc = _sTable->getSettingsRecord(key);
		if(doc.isArray())
			dbConfig[key] = doc.array();
		else
			dbConfig[key] = doc.object();
	}

	// possible data upgrade steps to prevent data loss
	if(handleConfigUpgrade(dbConfig))
	{
		saveSettings(dbConfig, true);
	}

	// validate full dbconfig against schema, on error we need to rewrite entire table
	QJsonSchemaChecker schemaChecker;
	schemaChecker.setSchema(schemaJson);
	QPair<bool,bool> valid = schemaChecker.validate(dbConfig);
	// check if our main schema syntax is IO
	if (!valid.second)
	{
		for (auto & schemaError : schemaChecker.getMessages())
			Error(_log, "Schema Syntax Error: %s", QSTRING_CSTR(schemaError));
		throw std::runtime_error("The config schema has invalid syntax. This should never happen! Go fix it!");
	}
	if (!valid.first)
	{
		Info(_log,"Table upgrade required...");
		dbConfig = schemaChecker.getAutoCorrectedConfig(dbConfig);

		for (auto & schemaError : schemaChecker.getMessages())
			Warning(_log, "Config Fix: %s", QSTRING_CSTR(schemaError));

		saveSettings(dbConfig);
	}
	else
		_qconfig = dbConfig;

	Debug(_log,"Settings database initialized");
}

QJsonDocument SettingsManager::getSetting(settings::type type) const
{
	return _sTable->getSettingsRecord(settings::typeToString(type));
}

bool SettingsManager::saveSettings(QJsonObject config, bool correct)
{
	// optional data upgrades e.g. imported legacy/older configs
	// handleConfigUpgrade(config);

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

		for (const auto & schemaError : schemaChecker.getMessages())
			Warning(_log, "Config Fix: %s", QSTRING_CSTR(schemaError));
	}

	// store the new config
	_qconfig = config;

	// extract keys and data
	QStringList keyList = config.keys();
	QStringList newValueList;
	for(const auto & key : keyList)
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

	int rc = true;
	// compare database data with new data to emit/save changes accordingly
	for(const auto & key : keyList)
	{
		QString data = newValueList.takeFirst();
		if(_sTable->getSettingsRecordString(key) != data)
		{
			if ( ! _sTable->createSettingsRecord(key, data) )
			{
				rc = false;
			}
			else
			{
				emit settingsChanged(settings::stringToType(key), QJsonDocument::fromJson(data.toLocal8Bit()));
			}
		}
	}
	return rc;
}

bool SettingsManager::handleConfigUpgrade(QJsonObject& config)
{
	bool migrated = false;

	// LED LAYOUT UPGRADE
	// from { hscan: { minimum: 0.2, maximum: 0.3 }, vscan: { minimum: 0.2, maximumn: 0.3 } }
	// from { h: { min: 0.2, max: 0.3 }, v: { min: 0.2, max: 0.3 } }
	// to   { hmin: 0.2, hmax: 0.3, vmin: 0.2, vmax: 0.3}
	if(config.contains("leds"))
	{
		const QJsonArray ledarr = config["leds"].toArray();
		const QJsonObject led = ledarr[0].toObject();

		if(led.contains("hscan") || led.contains("h"))
		{
			const bool whscan = led.contains("hscan");
			QJsonArray newLedarr;

			for(const auto & entry : ledarr)
			{
				const QJsonObject led = entry.toObject();
				QJsonObject hscan;
				QJsonObject vscan;
				QJsonValue hmin;
				QJsonValue hmax;
				QJsonValue vmin;
				QJsonValue vmax;
				QJsonObject nL;

				if(whscan)
				{
					hscan = led["hscan"].toObject();
					vscan = led["vscan"].toObject();
					hmin = hscan["minimum"];
					hmax = hscan["maximum"];
					vmin = vscan["minimum"];
					vmax = vscan["maximum"];
				}
				else
				{
					hscan = led["h"].toObject();
					vscan = led["v"].toObject();
					hmin = hscan["min"];
					hmax = hscan["max"];
					vmin = vscan["min"];
					vmax = vscan["max"];
				}
				// append to led object
				nL["hmin"] = hmin;
				nL["hmax"] = hmax;
				nL["vmin"] = vmin;
				nL["vmax"] = vmax;
				newLedarr.append(nL);
			}
			// replace
			config["leds"] = newLedarr;
			migrated = true;
			Debug(_log,"LED Layout migrated");
		}
	}
	return migrated;
}
