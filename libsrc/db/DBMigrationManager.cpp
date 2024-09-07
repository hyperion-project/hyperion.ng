#include <db/DBMigrationManager.h>

#include "db/SettingsTable.h"
#include <utils/Logger.h>
#include <utils/QStringUtils.h>

#include <HyperionConfig.h>

#include <QJsonObject>

DBMigrationManager::DBMigrationManager(QObject *parent)
	: DBManager{parent}
{
}

bool DBMigrationManager::isMigrationRequired()
{
	bool isNewRelease = false;

	SettingsTable settingsTableGlobal;

	if (settingsTableGlobal.resolveConfigVersion())
	{
		semver::version BUILD_VERSION(HYPERION_VERSION);

		if (!BUILD_VERSION.isValid())
		{
			Error(_log, "Current Hyperion version [%s] is invalid. Exiting...", BUILD_VERSION.getVersion().c_str());
			exit(1);
		}

		const semver::version& currentVersion = settingsTableGlobal.getConfigVersion();
		if (currentVersion > BUILD_VERSION)
		{
			Error(_log, "Database version [%s] is greater than current Hyperion version [%s]. Exiting...", currentVersion.getVersion().c_str(), BUILD_VERSION.getVersion().c_str());
			exit(1);
		}

		if (currentVersion < BUILD_VERSION)
		{
			isNewRelease = true;
		}
	}
	return isNewRelease;
}

bool DBMigrationManager::migrateSettings(QJsonObject& config)
{
	bool migrated = false;
	semver::version BUILD_VERSION(HYPERION_VERSION);

	SettingsTable settingsTableGlobal;
	QJsonObject generalConfig = config.value("global").toObject().value("settings").toObject().value("general").toObject();

	if (settingsTableGlobal.resolveConfigVersion(generalConfig))
	{
		semver::version currentVersion = settingsTableGlobal.getConfigVersion();

		if (currentVersion < BUILD_VERSION)
		{
			Info(_log, "Migration from current version [%s] to new version [%s] started", currentVersion.getVersion().c_str(), BUILD_VERSION.getVersion().c_str());

			// Extract, modify, and reinsert the global settings
			QJsonObject globalSettings = config.value("global").toObject().value("settings").toObject();
			upgradeGlobalSettings(currentVersion, globalSettings);

			QJsonObject globalConfig = config.value("global").toObject();
			globalConfig.insert("settings", globalSettings);
			config.insert("global", globalConfig);

			// Update each instance directly within the config
			QJsonArray instancesConfig = config.value("instances").toArray();
			for (int i = 0; i < instancesConfig.size(); ++i)
			{
				QJsonObject instanceConfig = instancesConfig[i].toObject();
				QJsonObject instanceSettings = instanceConfig.value("settings").toObject();

				upgradeInstanceSettings(currentVersion, static_cast<quint8>(i), instanceSettings);

				// Reinsert the modified instance settings back into the instanceConfig
				instanceConfig.insert("settings", instanceSettings);
				instancesConfig.replace(i, instanceConfig);
			}
			config.insert("instances", instancesConfig);

			Info(_log, "Migration from current version [%s] to new version [%s] finished", currentVersion.getVersion().c_str(), BUILD_VERSION.getVersion().c_str());
			migrated = true;
		}
	}

	return migrated;
}

bool DBMigrationManager::upgradeGlobalSettings(const semver::version& currentVersion, QJsonObject& config)
{
	bool migrated = false;

	semver::version migratedVersion = currentVersion;

	//Migration step for versions < alpha 9
	upgradeGlobalSettings_alpha_9(migratedVersion, config);
	//Migration step for versions < 2.0.12
	upgradeGlobalSettings_2_0_12(migratedVersion, config);
	//Migration step for versions < 2.0.16
	upgradeGlobalSettings_2_0_16(migratedVersion, config);
	//Migration step for versions < 2.0.17
	upgradeGlobalSettings_2_1_0(migratedVersion, config);

	// Set the daqtabase version to the current build version
	QJsonObject generalConfig = config["general"].toObject();
	// Update the configVersion if necessary
	if (generalConfig["configVersion"].toString() != HYPERION_VERSION) {
		generalConfig["configVersion"] = HYPERION_VERSION;
		migrated = true;
	}
	// Re-insert the modified "general" object back into the config
	config["general"] = generalConfig;

	return migrated;
}

bool DBMigrationManager::upgradeInstanceSettings(const semver::version& currentVersion, quint8 instance, QJsonObject& config)
{
	bool migrated = false;
	semver::version migratedVersion = currentVersion;

	//Migration step for versions < alpha 9
	upgradeInstanceSettings_alpha_9(migratedVersion, instance, config);
	//Migration step for versions < 2.0.12
	upgradeInstanceSettings_2_0_12(migratedVersion, instance, config);
	//Migration step for versions < 2.0.13
	upgradeInstanceSettings_2_0_13(migratedVersion, instance, config);
	//Migration step for versions < 2.0.16
	upgradeInstanceSettings_2_0_16(migratedVersion, instance, config);

	return migrated;
}

bool DBMigrationManager::upgradeGlobalSettings_alpha_9(semver::version& currentVersion, QJsonObject& config)
{
	bool migrated = false;
	const semver::version targetVersion{ "2.0.0-alpha.9" };

	if (currentVersion < targetVersion)
	{
		Info(_log, "Global settings: Migrate from version [%s] to version [%s] or later", currentVersion.getVersion().c_str(), targetVersion.getVersion().c_str());
		currentVersion = targetVersion;

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

		if (config.contains("grabberAudio"))
		{
			QJsonObject newGrabberAudioConfig = config["grabberAudio"].toObject();

			//Add new element enable
			if (!newGrabberAudioConfig.contains("enable"))
			{
				newGrabberAudioConfig["enable"] = false;
				migrated = true;
			}
			config["grabberAudio"] = newGrabberAudioConfig;
			Debug(_log, "GrabberAudio records migrated");
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

	return migrated;
}

bool DBMigrationManager::upgradeGlobalSettings_2_0_12(semver::version& currentVersion, QJsonObject& config)
{
	bool migrated = false;
	const semver::version targetVersion{ "2.0.12" };

	if (currentVersion < targetVersion)
	{
		Info(_log, "Global settings: Migrate from version [%s] to version [%s] or later", currentVersion.getVersion().c_str(), targetVersion.getVersion().c_str());
		currentVersion = targetVersion;

		// Have Hostname/IP-address separate from port for Forwarder
		if (config.contains("forwarder"))
		{
			QJsonObject newForwarderConfig = config["forwarder"].toObject();

			QJsonArray json;
			if (newForwarderConfig.contains("json"))
			{
				const QJsonArray oldJson = newForwarderConfig["json"].toArray();
				QJsonObject newJsonConfig;

				for (const QJsonValue& value : oldJson)
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
				const QJsonArray oldFlatbuffer = newForwarderConfig["flat"].toArray();
				QJsonObject newFlattbufferConfig;

				for (const QJsonValue& value : oldFlatbuffer)
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
				currentVersion = targetVersion;
			}
		}
	}

	return migrated;
}

bool DBMigrationManager::upgradeGlobalSettings_2_0_16(semver::version& currentVersion, QJsonObject& config)
{
	bool migrated = false;
	const semver::version targetVersion{ "2.0.16" };

	if (currentVersion < targetVersion)
	{
		Info(_log, "Global settings: Migrate from version [%s] to version [%s] or later", currentVersion.getVersion().c_str(), targetVersion.getVersion().c_str());
		currentVersion = targetVersion;

		if (config.contains("cecEvents"))
		{
			bool isCECEnabled {false};
			if (config.contains("grabberV4L2"))
			{
				QJsonObject newGrabberV4L2Config = config["grabberV4L2"].toObject();
				if (newGrabberV4L2Config.contains("cecDetection"))
				{
					isCECEnabled = newGrabberV4L2Config.value("cecDetection").toBool(false);
					newGrabberV4L2Config.remove("cecDetection");
					config["grabberV4L2"] = newGrabberV4L2Config;

					QJsonObject newGCecEventsConfig = config["cecEvents"].toObject();
					newGCecEventsConfig["enable"] = isCECEnabled;
					if (!newGCecEventsConfig.contains("actions"))
					{
						QJsonObject action1
						{
							{"action", "Suspend"},
							{"event", "standby"}
						};
						QJsonObject action2
						{
							{"action", "Resume"},
							{"event", "set stream path"}
						};

						QJsonArray actions { action1, action2 };
						newGCecEventsConfig.insert("actions",actions);
					}
					config["cecEvents"] = newGCecEventsConfig;

					migrated = true;
					Debug(_log, "CEC configuration records migrated");
				}
			}
		}
	}

	return migrated;
}

bool DBMigrationManager::upgradeGlobalSettings_2_1_0(semver::version& currentVersion, QJsonObject& config)
{
	bool migrated = false;
	const semver::version targetVersion{ "2.0.17-beta.2" };

	if (currentVersion < targetVersion)
	{
		Info(_log, "Global settings: Migrate from version [%s] to version [%s] or later", currentVersion.getVersion().c_str(), targetVersion.getVersion().c_str());
		currentVersion = targetVersion;

		if (config.contains("general"))
		{
			QJsonObject newGeneralConfig = config["general"].toObject();
			newGeneralConfig.remove("previousVersion");
			config.insert("general", newGeneralConfig);

			Debug(_log, "General settings migrated");
			migrated = true;
		}

		if (config.contains("network"))
		{
			QJsonObject newNetworkConfig = config["network"].toObject();
			newNetworkConfig.remove("apiAuth");
			newNetworkConfig.remove("localAdminAuth");
			config.insert("network", newNetworkConfig);

			Debug(_log, "Network settings migrated");
			migrated = true;
		}

	}

	//Remove wrong instance 255 configuration records, created by the global instance #255
	SettingsTable globalSettingsTable(255);
	globalSettingsTable.deleteInstance();
	migrated = true;

	return migrated;
}

bool DBMigrationManager::upgradeInstanceSettings_alpha_9(semver::version& currentVersion, quint8 instance, QJsonObject& config)
{
	bool migrated = false;
	const semver::version targetVersion{ "2.0.0-alpha.9" };

	if (currentVersion < targetVersion)
	{
		Info(_log, "Settings instance [%u]: Migrate from version [%s] to version [%s] or later", instance, currentVersion.getVersion().c_str(), targetVersion.getVersion().c_str());
		currentVersion = targetVersion;

		// LED LAYOUT UPGRADE
		// from { hscan: { minimum: 0.2, maximum: 0.3 }, vscan: { minimum: 0.2, maximum: 0.3 } }
		// from { h: { min: 0.2, max: 0.3 }, v: { min: 0.2, max: 0.3 } }
		// to   { hmin: 0.2, hmax: 0.3, vmin: 0.2, vmax: 0.3}
		if (config.contains("leds"))
		{
			const QJsonArray ledarr = config["leds"].toArray();
			const QJsonObject firstLed = ledarr[0].toObject();

			if (firstLed.contains("hscan") || firstLed.contains("h"))
			{
				const bool whscan = firstLed.contains("hscan");
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
				Info(_log, "Instance [%u]: LED Layout migrated", instance);
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
				Info(_log, "Instance [%u]: LED-Config migrated", instance);
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
						Warning(_log, "Instance [%u]: HwLedCount/Layout mismatch! Setting Hardware LED count to number of LEDs configured via layout", instance);
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
	}

	return migrated;
}

bool DBMigrationManager::upgradeInstanceSettings_2_0_12(semver::version& currentVersion, quint8 instance, QJsonObject& config)
{
	bool migrated = false;
	const semver::version targetVersion{ "2.0.12" };

	if (currentVersion < targetVersion)
	{
		Info(_log, "Settings instance [%u]: Migrate from version [%s] to version [%s] or later", instance, currentVersion.getVersion().c_str(), targetVersion.getVersion().c_str());
		currentVersion = targetVersion;

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

	}

	return migrated;
}

bool DBMigrationManager::upgradeInstanceSettings_2_0_13(semver::version& currentVersion, quint8 instance, QJsonObject& config)
{
	bool migrated = false;
	const semver::version targetVersion{ "2.0.13" };

	if (currentVersion < targetVersion)
	{
		Info(_log, "Settings instance [%u]: Migrate from version [%s] to version [%s] or later", instance, currentVersion.getVersion().c_str(), targetVersion.getVersion().c_str());
		currentVersion = targetVersion;

		// Have Hostname/IP-address separate from port for LED-Devices
		if (config.contains("device"))
		{
			QJsonObject newDeviceConfig = config["device"].toObject();

			if (newDeviceConfig.contains("type"))
			{
				QString type = newDeviceConfig["type"].toString();

				const QStringList serialDevices{ "adalight", "dmx", "atmo", "sedu", "tpm2", "karate" };
				if (serialDevices.contains(type))
				{
					if (!newDeviceConfig.contains("rateList"))
					{
						newDeviceConfig["rateList"] = "CUSTOM";
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

	return migrated;
}

bool DBMigrationManager::upgradeInstanceSettings_2_0_16(semver::version& currentVersion, quint8 instance, QJsonObject& config)
{
	bool migrated = false;
	const semver::version targetVersion{ "2.0.16" };

	if (currentVersion < targetVersion)
	{
		Info(_log, "Settings instance [%u]: Migrate from version [%s] to version [%s] or later", instance, currentVersion.getVersion().c_str(), targetVersion.getVersion().c_str());
		currentVersion = targetVersion;

		// Have Hostname/IP-address separate from port for LED-Devices
		if (config.contains("device"))
		{
			QJsonObject newDeviceConfig = config["device"].toObject();

			if (newDeviceConfig.contains("type"))
			{
				QString type = newDeviceConfig["type"].toString();

				if (type == "philipshue")
				{
					if (newDeviceConfig.contains("groupId"))
					{
						if (newDeviceConfig["groupId"].isDouble())
						{
							int groupID = newDeviceConfig["groupId"].toInt();
							newDeviceConfig["groupId"] = QString::number(groupID);
							migrated = true;
						}
					}

					if (newDeviceConfig.contains("lightIds"))
					{
						QJsonArray lightIds = newDeviceConfig.value("lightIds").toArray();
						// Iterate through the JSON array and update integer values to strings
						for (int i = 0; i < lightIds.size(); ++i) {
							QJsonValue value = lightIds.at(i);
							if (value.isDouble())
							{
								int lightId = value.toInt();
								lightIds.replace(i, QString::number(lightId));
								migrated = true;
							}
						}
						newDeviceConfig["lightIds"] = lightIds;

					}
				}

				if (type == "nanoleaf")
				{
					if (newDeviceConfig.contains("panelStartPos"))
					{
						newDeviceConfig.remove("panelStartPos");
						migrated = true;
					}

					if (newDeviceConfig.contains("panelOrderTopDown"))
					{
						int panelOrderTopDown;
						if (newDeviceConfig["panelOrderTopDown"].isDouble())
						{
							panelOrderTopDown = newDeviceConfig["panelOrderTopDown"].toInt();
						}
						else
						{
							panelOrderTopDown = newDeviceConfig["panelOrderTopDown"].toString().toInt();
						}

						newDeviceConfig.remove("panelOrderTopDown");
						if (panelOrderTopDown == 0)
						{
							newDeviceConfig["panelOrderTopDown"] = "top2down";
							migrated = true;
						}
						else
						{
							if (panelOrderTopDown == 1)
							{
								newDeviceConfig["panelOrderTopDown"] = "bottom2up";
								migrated = true;
							}
						}
					}

					if (newDeviceConfig.contains("panelOrderLeftRight"))
					{
						int panelOrderLeftRight;
						if (newDeviceConfig["panelOrderLeftRight"].isDouble())
						{
							panelOrderLeftRight = newDeviceConfig["panelOrderLeftRight"].toInt();
						}
						else
						{
							panelOrderLeftRight = newDeviceConfig["panelOrderLeftRight"].toString().toInt();
						}

						newDeviceConfig.remove("panelOrderLeftRight");
						if (panelOrderLeftRight == 0)
						{
							newDeviceConfig["panelOrderLeftRight"] = "left2right";
							migrated = true;
						}
						else
						{
							if (panelOrderLeftRight == 1)
							{
								newDeviceConfig["panelOrderLeftRight"] = "right2left";
								migrated = true;
							}
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
	}

	return migrated;
}
