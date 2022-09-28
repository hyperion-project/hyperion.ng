// proj
#include <hyperion/SettingsManager.h>

// util
#include <utils/JsonUtils.h>
#include <utils/QStringUtils.h>

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
	  , _log(Logger::getInstance("SETTINGSMGR", "I"+QString::number(instance)))
	  , _instance(instance)
	  , _sTable(new SettingsTable(instance, this))
	  , _configVersion(DEFAULT_VERSION)
	  , _previousVersion(DEFAULT_VERSION)
	  , _readonlyMode(readonlyMode)
{
	_sTable->setReadonlyMode(_readonlyMode);
	// get schema
	if (schemaJson.isEmpty())
	{
		Q_INIT_RESOURCE(resource);
		try
		{
			schemaJson = QJsonFactory::readSchema(":/hyperion-schema");
		}
		catch (const std::runtime_error& error)
		{
			throw std::runtime_error(error.what());
		}
	}

	// get default config
	QJsonObject defaultConfig;
	if (!JsonUtils::readFile(":/hyperion_default.config", defaultConfig, _log))
	{
		throw std::runtime_error("Failed to read default config");
	}

	// transform json to string lists
	QStringList keyList = defaultConfig.keys();
	QStringList defValueList;
	for (const auto& key : qAsConst(keyList))
	{
		if (defaultConfig[key].isObject())
		{
			defValueList << QString(QJsonDocument(defaultConfig[key].toObject()).toJson(QJsonDocument::Compact));
		}
		else if (defaultConfig[key].isArray())
		{
			defValueList << QString(QJsonDocument(defaultConfig[key].toArray()).toJson(QJsonDocument::Compact));
		}
	}

	// fill database with default data if required
	for (const auto& key : qAsConst(keyList))
	{
		QString val = defValueList.takeFirst();
		// prevent overwrite
		if (!_sTable->recordExist(key))
		{
			_sTable->createSettingsRecord(key, val);
		}
	}

	// need to validate all data in database construct the entire data object
	// TODO refactor schemaChecker to accept QJsonArray in validate(); QJsonDocument container? To validate them per entry...
	QJsonObject dbConfig;
	for (const auto& key : qAsConst(keyList))
	{
		QJsonDocument doc = _sTable->getSettingsRecord(key);
		if (doc.isArray())
		{
			dbConfig[key] = doc.array();
		}
		else
		{
			dbConfig[key] = doc.object();
		}
	}

	//Check, if database requires migration
	bool isNewRelease = false;
	// Use instance independent SettingsManager to track migration status
	if (_instance == GLOABL_INSTANCE_ID)
	{
		if (resolveConfigVersion(dbConfig))
		{
			QJsonObject newGeneralConfig = dbConfig["general"].toObject();

			semver::version BUILD_VERSION(HYPERION_VERSION);

			if (!BUILD_VERSION.isValid())
			{
				Error(_log, "Current Hyperion version [%s] is invalid. Exiting...", BUILD_VERSION.getVersion().c_str());
				exit(1);
			}

			if (_configVersion > BUILD_VERSION)
			{
				Error(_log, "Database version [%s] is greater than current Hyperion version [%s]", _configVersion.getVersion().c_str(), BUILD_VERSION.getVersion().c_str());
				// TODO: Remove version checking and Settingsmanager from components' constructor to be able to stop hyperion.
			}
			else
			{
				if (_previousVersion < BUILD_VERSION)
				{
					if (_configVersion == BUILD_VERSION)
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
	if (isNewRelease || migrated)
	{
		saveSettings(dbConfig, true);
	}

	// validate full dbconfig against schema, on error we need to rewrite entire table
	QJsonSchemaChecker schemaChecker;
	schemaChecker.setSchema(schemaJson);
	QPair<bool, bool> valid = schemaChecker.validate(dbConfig);
	// check if our main schema syntax is IO
	if (!valid.second)
	{
		for (auto& schemaError : schemaChecker.getMessages())
		{
			Error(_log, "Schema Syntax Error: %s", QSTRING_CSTR(schemaError));
		}
		throw std::runtime_error("The config schema has invalid syntax. This should never happen! Go fix it!");
	}
	if (!valid.first)
	{
		Info(_log, "Table upgrade required...");
		dbConfig = schemaChecker.getAutoCorrectedConfig(dbConfig);

		for (auto& schemaError : schemaChecker.getMessages())
		{
			Warning(_log, "Config Fix: %s", QSTRING_CSTR(schemaError));
		}

		saveSettings(dbConfig, true);
	}
	else
	{
		_qconfig = dbConfig;
	}

	Debug(_log, "Settings database initialized");
}

QJsonDocument SettingsManager::getSetting(settings::type type) const
{
	return _sTable->getSettingsRecord(settings::typeToString(type));
}

QJsonObject SettingsManager::getSettings() const
{
	QJsonObject config;
	for (const auto& key : _qconfig.keys())
	{
		//Read all records from database to ensure that global settings are read across instances
		QJsonDocument doc = _sTable->getSettingsRecord(key);
		if (doc.isArray())
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

bool SettingsManager::restoreSettings(QJsonObject config, bool correct)
{
	// optional data upgrades e.g. imported legacy/older configs
	handleConfigUpgrade(config);
	return saveSettings(config, correct);
}

bool SettingsManager::saveSettings(QJsonObject config, bool correct)
{
	// we need to validate data against schema
	QJsonSchemaChecker schemaChecker;
	schemaChecker.setSchema(schemaJson);
	if (!schemaChecker.validate(config).first)
	{
		if (!correct)
		{
			Error(_log, "Failed to save configuration, errors during validation");
			return false;
		}
		Warning(_log, "Fixing json data!");
		config = schemaChecker.getAutoCorrectedConfig(config);

		for (const auto& schemaError : schemaChecker.getMessages())
		{
			Warning(_log, "Config Fix: %s", QSTRING_CSTR(schemaError));
		}
	}

	// store the new config
	_qconfig = config;

	// extract keys and data
	QStringList keyList = config.keys();
	QStringList newValueList;
	for (const auto& key : qAsConst(keyList))
	{
		if (config[key].isObject())
		{
			newValueList << QString(QJsonDocument(config[key].toObject()).toJson(QJsonDocument::Compact));
		}
		else if (config[key].isArray())
		{
			newValueList << QString(QJsonDocument(config[key].toArray()).toJson(QJsonDocument::Compact));
		}
	}

	bool rc = true;
	// compare database data with new data to emit/save changes accordingly
	for (const auto& key : qAsConst(keyList))
	{
		QString data = newValueList.takeFirst();
		if (_sTable->getSettingsRecordString(key) != data)
		{
			if (!_sTable->createSettingsRecord(key, data))
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

inline QString fixVersion(const QString& version)
{
	QString newVersion;
	//Try fixing version number, remove dot separated pre-release identifiers not supported
	QRegularExpression regEx("(\\d+\\.\\d+\\.\\d+-?[a-zA-Z-\\d]*\\.?[\\d]*)", QRegularExpression::CaseInsensitiveOption | QRegularExpression::MultilineOption);
	QRegularExpressionMatch match;

	match = regEx.match(version);
	if (match.hasMatch())
	{
		newVersion = match.captured(1);
	}
	return newVersion;
}

bool SettingsManager::resolveConfigVersion(QJsonObject& config)
{
	bool isValid = false;
	if (config.contains("general"))
	{
		QJsonObject generalConfig = config["general"].toObject();
		QString configVersion = generalConfig["configVersion"].toString();
		QString previousVersion = generalConfig["previousVersion"].toString();

		if (!configVersion.isEmpty())
		{
			isValid = _configVersion.setVersion(configVersion.toStdString());
			if (!isValid)
			{
				isValid = _configVersion.setVersion(fixVersion(configVersion).toStdString());
				if (isValid)
				{
					Info(_log, "Invalid config version [%s] fixed. Updated to [%s]", QSTRING_CSTR(configVersion), _configVersion.getVersion().c_str());
				}
			}
		}
		else
		{
			isValid = true;
		}

		if (!previousVersion.isEmpty() && isValid)
		{
			isValid = _previousVersion.setVersion(previousVersion.toStdString());
			if (!isValid)
			{
				isValid = _previousVersion.setVersion(fixVersion(previousVersion).toStdString());
				if (isValid)
				{
					Info(_log, "Invalid previous version [%s] fixed. Updated to [%s]", QSTRING_CSTR(previousVersion), _previousVersion.getVersion().c_str());
				}
			}
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

	//Only migrate, if valid versions are available
	if (!resolveConfigVersion(config))
	{
		Warning(_log, "Invalid version information found in configuration. No database migration executed.");
	}
	else
	{
		//Do only migrate, if configuration is not up to date
		if (_previousVersion < _configVersion)
		{
			//Migration steps for versions <= alpha 9
			semver::version targetVersion{ "2.0.0-alpha.9" };
			if (_previousVersion <= targetVersion)
			{
				Info(_log, "Instance [%u]: Migrate from version [%s] to version [%s] or later", _instance, _previousVersion.getVersion().c_str(), targetVersion.getVersion().c_str());

				// LED LAYOUT UPGRADE
				// from { hscan: { minimum: 0.2, maximum: 0.3 }, vscan: { minimum: 0.2, maximum: 0.3 } }
				// from { h: { min: 0.2, max: 0.3 }, v: { min: 0.2, max: 0.3 } }
				// to   { hmin: 0.2, hmax: 0.3, vmin: 0.2, vmax: 0.3}
				if (config.contains("leds"))
				{
					const QJsonArray ledarr = config["leds"].toArray();
					const QJsonObject led = ledarr[0].toObject();

					if (led.contains("hscan") || led.contains("h"))
					{
						const bool whscan = led.contains("hscan");
						QJsonArray newLedarr;

						for (const auto& entry : ledarr)
						{
							const QJsonObject led = entry.toObject();
							QJsonObject hscan;
							QJsonObject vscan;
							QJsonValue hmin;
							QJsonValue hmax;
							QJsonValue vmin;
							QJsonValue vmax;
							QJsonObject nL;

							if (whscan)
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
						Info(_log, "Instance [%u]: LED Layout migrated", _instance);
					}
				}

				if (config.contains("ledConfig"))
				{
					QJsonObject oldLedConfig = config["ledConfig"].toObject();
					if (!oldLedConfig.contains("classic"))
					{
						QJsonObject newLedConfig;
						newLedConfig.insert("classic", oldLedConfig);
						QJsonObject defaultMatrixConfig{ {"ledshoriz", 1}
							,{"ledsvert", 1}
							,{"cabling","snake"}
							,{"start","top-left"}
						};
						newLedConfig.insert("matrix", defaultMatrixConfig);

						config["ledConfig"] = newLedConfig;
						migrated = true;
						Info(_log, "Instance [%u]: LED-Config migrated", _instance);
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

							if (hwLedcount < layoutLedCount)
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
						if (type == "atmoorb" || type == "fadecandy" || type == "philipshue")
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

			//Migration steps for versions <= 2.0.12
			_previousVersion = targetVersion;
			targetVersion.setVersion("2.0.12");
			if (_previousVersion <= targetVersion)
			{
				Info(_log, "Instance [%u]: Migrate from version [%s] to version [%s] or later", _instance, _previousVersion.getVersion().c_str(), targetVersion.getVersion().c_str());

				// Have Hostname/IP-address separate from port for LED-Devices
				if (config.contains("device"))
				{
					QJsonObject newDeviceConfig = config["device"].toObject();

					if (newDeviceConfig.contains("host"))
					{
						QString oldHost = newDeviceConfig["host"].toString();

						// Resolve hostname and port
						QStringList addressparts = QStringUtils::split(oldHost, ":", QStringUtils::SplitBehavior::SkipEmptyParts);

						newDeviceConfig["host"] = addressparts[0];

						if (addressparts.size() > 1)
						{
							if (!newDeviceConfig.contains("port"))
							{
								newDeviceConfig["port"] = addressparts[1].toInt();
							}
							migrated = true;
						}
					}

					if (newDeviceConfig.contains("type"))
					{
						QString type = newDeviceConfig["type"].toString();
						if (type == "apa102")
						{
							if (newDeviceConfig.contains("colorOrder"))
							{
								QString colorOrder = newDeviceConfig["colorOrder"].toString();
								if (colorOrder == "bgr")
								{
									newDeviceConfig["colorOrder"] = "rgb";
									migrated = true;
								}
							}
						}
					}

					if (migrated)
					{
						config["device"] = newDeviceConfig;
						Debug(_log, "LED-Device records migrated");
					}
				}

				// Have Hostname/IP-address separate from port for Forwarder
				if (config.contains("forwarder"))
				{
					QJsonObject newForwarderConfig = config["forwarder"].toObject();

					QJsonArray json;
					if (newForwarderConfig.contains("json"))
					{
						QJsonArray oldJson = newForwarderConfig["json"].toArray();
						QJsonObject newJsonConfig;

						for (const QJsonValue& value : qAsConst(oldJson))
						{
							if (value.isString())
							{
								QString oldHost = value.toString();
								// Resolve hostname and port
								QStringList addressparts = QStringUtils::split(oldHost, ":", QStringUtils::SplitBehavior::SkipEmptyParts);
								QString host = addressparts[0];

								if (host != "127.0.0.1")
								{
									newJsonConfig["host"] = host;

									if (addressparts.size() > 1)
									{
										newJsonConfig["port"] = addressparts[1].toInt();
									}
									else
									{
										newJsonConfig["port"] = 19444;
									}
									newJsonConfig["name"] = host;

									json.append(newJsonConfig);
									migrated = true;
								}
							}
						}

						if (!json.isEmpty())
						{
							newForwarderConfig["jsonapi"] = json;
						}
						newForwarderConfig.remove("json");
						migrated = true;
					}

					QJsonArray flatbuffer;
					if (newForwarderConfig.contains("flat"))
					{
						QJsonArray oldFlatbuffer = newForwarderConfig["flat"].toArray();
						QJsonObject newFlattbufferConfig;

						for (const QJsonValue& value : qAsConst(oldFlatbuffer))
						{
							if (value.isString())
							{
								QString oldHost = value.toString();
								// Resolve hostname and port
								QStringList addressparts = QStringUtils::split(oldHost, ":", QStringUtils::SplitBehavior::SkipEmptyParts);
								QString host = addressparts[0];

								if (host != "127.0.0.1")
								{
									newFlattbufferConfig["host"] = host;

									if (addressparts.size() > 1)
									{
										newFlattbufferConfig["port"] = addressparts[1].toInt();
									}
									else
									{
										newFlattbufferConfig["port"] = 19400;
									}
									newFlattbufferConfig["name"] = host;

									flatbuffer.append(newFlattbufferConfig);
									migrated = true;
								}
							}

							if (!flatbuffer.isEmpty())
							{
								newForwarderConfig["flatbuffer"] = flatbuffer;
							}
							newForwarderConfig.remove("flat");
							migrated = true;
						}
					}

					if (json.isEmpty() && flatbuffer.isEmpty())
					{
						newForwarderConfig["enable"] = false;
					}

					if (migrated)
					{
						config["forwarder"] = newForwarderConfig;
						Debug(_log, "Forwarder records migrated");
					}
				}
			}

			//Migration steps for versions <= 2.0.13
			targetVersion.setVersion("2.0.13");
			if (_previousVersion <= targetVersion)
			{
				Info(_log, "Instance [%u]: Migrate from version [%s] to version [%s] or later", _instance, _previousVersion.getVersion().c_str(), targetVersion.getVersion().c_str());


				// Have Hostname/IP-address separate from port for LED-Devices
				if (config.contains("device"))
				{
					QJsonObject newDeviceConfig = config["device"].toObject();

					if (newDeviceConfig.contains("type"))
					{
						QString type = newDeviceConfig["type"].toString();

						const QStringList serialDevices {"adalight", "dmx", "atmo", "sedu", "tpm2", "karate"};
						if ( serialDevices.contains(type ))
						{
							if (!newDeviceConfig.contains("rateList"))
							{
								newDeviceConfig["rateList"] =  "CUSTOM";
								migrated = true;
							}
						}

						if (type == "adalight")
						{
							if (newDeviceConfig.contains("lightberry_apa102_mode"))
							{
								bool lightberry_apa102_mode = newDeviceConfig["lightberry_apa102_mode"].toBool();
								if (lightberry_apa102_mode)
								{
									newDeviceConfig["streamProtocol"] = "1";
								}
								else
								{
									newDeviceConfig["streamProtocol"] = "0";
								}
								newDeviceConfig.remove("lightberry_apa102_mode");
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
			}
		}
	}
	return migrated;
}
