// proj
#include <hyperion/SettingsManager.h>

// util
#include <utils/JsonUtils.h>
#include <db/SettingsTable.h>
#include "HyperionConfig.h"

// json schema process
#include <utils/jsonschema/QJsonFactory.h>
#include <utils/jsonschema/QJsonSchemaChecker.h>

// write config to filesystem
#include <utils/JsonUtils.h>

#include <utils/version.hpp>
using namespace semver;

// Constants
namespace {
const char DEFAULT_VERSION[] = "2.0.0-alpha.8";
} //End of constants

QJsonObject SettingsManager::schemaJson;

SettingsManager::SettingsManager(quint8 instance, QObject* parent, bool readonlyMode)
	: QObject(parent)
	, _log(Logger::getInstance("SETTINGSMGR"))
	, _instance(instance)
	, _sTable(new SettingsTable(instance, this))
	, _configVersion(DEFAULT_VERSION)
	, _previousVersion(DEFAULT_VERSION)
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
	{
		throw std::runtime_error("Failed to read default config");
	}

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

	// need to validate all data in database construct the entire data object
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

	//Check, if database requires migration
	bool isNewRelease = false;
	// Use instance independent SettingsManager to track migration status
	if ( instance == GLOABL_INSTANCE_ID)
	{
		if ( resolveConfigVersion(dbConfig) )
		{
			QJsonObject newGeneralConfig = dbConfig["general"].toObject();

			semver::version BUILD_VERSION(HYPERION_VERSION);
			if ( _configVersion > BUILD_VERSION )
			{
				Error(_log, "Database version [%s] is greater that current Hyperion version [%s]", _configVersion.getVersion().c_str(), BUILD_VERSION.getVersion().c_str());
				// TODO: Remove version checking and Settingsmanager from components' constructor to be able to stop hyperion.
			}
			else
			{
				if ( _previousVersion < BUILD_VERSION )
				{
					if ( _configVersion == BUILD_VERSION )
					{
						newGeneralConfig["previousVersion"] = BUILD_VERSION.getVersion().c_str();
						dbConfig["general"] = newGeneralConfig;
						isNewRelease = true;
						Info(_log, "Migration completed to version [%s]", BUILD_VERSION.getVersion().c_str());
					}
					else
					{
						Info(_log, "Migration from current version [%s] to new version [%s] started", _previousVersion.getVersion().c_str(), BUILD_VERSION.getVersion().c_str());

						newGeneralConfig["previousVersion"] = _configVersion.getVersion().c_str();
						newGeneralConfig["configVersion"] = BUILD_VERSION.getVersion().c_str();
						dbConfig["general"] = newGeneralConfig;
						isNewRelease = true;
					}
				}
			}
		}
	}

	// possible data upgrade steps to prevent data loss
	bool migrated = handleConfigUpgrade(dbConfig);
	if ( isNewRelease || migrated )
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

		saveSettings(dbConfig,true);
	}
	else
		_qconfig = dbConfig;

	Debug(_log,"Settings database initialized");
}

QJsonDocument SettingsManager::getSetting(settings::type type) const
{
	return _sTable->getSettingsRecord(settings::typeToString(type));
}

QJsonObject SettingsManager::getSettings() const
{
	QJsonObject config;
	for(const auto & key : _qconfig.keys())
	{
		//Read all records from database to ensure that global settings are read across instances
		QJsonDocument doc = _sTable->getSettingsRecord(key);
		if(doc.isArray())
		{
			config.insert(key, doc.array());
		}
		else
		{
			config.insert(key, doc.object());
		}
	}
	return config;
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

bool SettingsManager::resolveConfigVersion(QJsonObject& config)
{
	bool isValid = false;
	if (config.contains("general"))
	{
		QJsonObject generalConfig = config["general"].toObject();
		QString configVersion = generalConfig["configVersion"].toString();
		QString previousVersion = generalConfig["previousVersion"].toString();

		if ( !configVersion.isEmpty() )
		{
			isValid = _configVersion.setVersion(configVersion.toStdString());
		}
		else
		{
			_configVersion.setVersion(DEFAULT_VERSION);
			isValid = true;
		}

		if ( !previousVersion.isEmpty() && isValid )
		{
			isValid = _previousVersion.setVersion(previousVersion.toStdString());
		}
		else
		{
			_previousVersion.setVersion(DEFAULT_VERSION);
			isValid = true;
		}
	}
	return isValid;
}

bool SettingsManager::handleConfigUpgrade(QJsonObject& config)
{
	bool migrated = false;

	resolveConfigVersion(config);

	//Do only migrate, if configuration is not up to date
	if (_previousVersion < _configVersion)
	{
		//Migration steps for versions <= alpha 9
		semver::version targetVersion {"2.0.0-alpha.9"};
		if (_previousVersion <= targetVersion )
		{
			Info(_log, "Instance [%u]: Migrate LED Layout from current version [%s] to version [%s] or later", _instance, _previousVersion.getVersion().c_str(), targetVersion.getVersion().c_str());

			// LED LAYOUT UPGRADE
			// from { hscan: { minimum: 0.2, maximum: 0.3 }, vscan: { minimum: 0.2, maximum: 0.3 } }
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
					Info(_log,"Instance [%u]: LED Layout migrated", _instance);
				}
			}

			if(config.contains("ledConfig"))
			{
				QJsonObject oldLedConfig = config["ledConfig"].toObject();
				if ( !oldLedConfig.contains("classic"))
				{
					QJsonObject newLedConfig;
					newLedConfig.insert("classic", oldLedConfig );
					QJsonObject defaultMatrixConfig {{"ledshoriz", 1}
													 ,{"ledsvert", 1}
													 ,{"cabling","snake"}
													 ,{"start","top-left"}
													};
					newLedConfig.insert("matrix", defaultMatrixConfig );

					config["ledConfig"] = newLedConfig;
					migrated = true;
					Info(_log,"Instance [%u]: LED-Config migrated", _instance);
				}
			}

			// LED Hardware count is leading for versions after alpha 9
			// Setting Hardware LED count to number of LEDs configured via layout, if layout number is greater than number of hardware LEDs
			if (config.contains("device"))
			{
				QJsonObject newDeviceConfig = config["device"].toObject();

				if (newDeviceConfig.contains("hardwareLedCount"))
				{
					int hwLedcount = newDeviceConfig["hardwareLedCount"].toInt();
					if (config.contains("leds"))
					{
						const QJsonArray ledarr = config["leds"].toArray();
						int layoutLedCount = ledarr.size();

						if (hwLedcount < layoutLedCount )
						{
							Warning(_log, "Instance [%u]: HwLedCount/Layout mismatch! Setting Hardware LED count to number of LEDs configured via layout", _instance);
							hwLedcount = layoutLedCount;
							newDeviceConfig["hardwareLedCount"] = hwLedcount;
							migrated = true;
						}
					}
				}

				if (newDeviceConfig.contains("type"))
				{
					QString type = newDeviceConfig["type"].toString();
					if (type == "atmoorb" || type == "fadecandy" || type == "philipshue" )
					{
						if (newDeviceConfig.contains("output"))
						{
							newDeviceConfig["host"] = newDeviceConfig["output"].toString();
							newDeviceConfig.remove("output");
							migrated = true;
						}
					}
				}

				if (migrated)
				{
					config["device"] = newDeviceConfig;
					Debug(_log, "LED-Device records migrated");
				}
			}

			if (config.contains("grabberV4L2"))
			{
				QJsonObject newGrabberV4L2Config = config["grabberV4L2"].toObject();

				if (newGrabberV4L2Config.contains("encoding_format"))
				{
					newGrabberV4L2Config.remove("encoding_format");
					newGrabberV4L2Config["grabberV4L2"] = newGrabberV4L2Config;
					migrated = true;
				}

				//Add new element enable
				if (!newGrabberV4L2Config.contains("enable"))
				{
					newGrabberV4L2Config["enable"] = false;
					migrated = true;
				}
				config["grabberV4L2"] = newGrabberV4L2Config;
				Debug(_log, "GrabberV4L2 records migrated");
			}

			if (config.contains("framegrabber"))
			{
				QJsonObject newFramegrabberConfig = config["framegrabber"].toObject();

				//Align element namings with grabberV4L2
				//Rename element type -> device
				if (newFramegrabberConfig.contains("type"))
				{
					newFramegrabberConfig["device"] = newFramegrabberConfig["type"].toString();
					newFramegrabberConfig.remove("type");
					migrated = true;
				}
				//Rename element frequency_Hz -> fps
				if (newFramegrabberConfig.contains("frequency_Hz"))
				{
					newFramegrabberConfig["fps"] = newFramegrabberConfig["frequency_Hz"].toInt(25);
					newFramegrabberConfig.remove("frequency_Hz");
					migrated = true;
				}

				//Rename element display -> input
				if (newFramegrabberConfig.contains("display"))
				{
					newFramegrabberConfig["input"] = newFramegrabberConfig["display"];
					newFramegrabberConfig.remove("display");
					migrated = true;
				}

				//Add new element enable
				if (!newFramegrabberConfig.contains("enable"))
				{
					newFramegrabberConfig["enable"] = false;
					migrated = true;
				}

				config["framegrabber"] = newFramegrabberConfig;
				Debug(_log, "Framegrabber records migrated");
			}
		}
	}
	return migrated;
}
