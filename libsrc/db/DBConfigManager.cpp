#include <db/DBConfigManager.h>

#include <db/DBMigrationManager.h>
#include "db/SettingsTable.h"
#include <db/MetaTable.h>

#include <db/InstanceTable.h>
#include <hyperion/SettingsManager.h>

#include <qsqlrecord.h>
#include <utils/JsonUtils.h>
#include <utils/jsonschema/QJsonFactory.h>

#include <HyperionConfig.h>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QDateTime>

namespace {
const char SETTINGS_FULL_SCHEMA_FILE[] = ":/schema-settings-full.json";
}

DBConfigManager::DBConfigManager(QObject* parent)
	: DBManager(parent)
{
}

QPair<bool, QStringList> DBConfigManager::importJson(const QString& configFile)
{
	Info(_log,"Import configuration file '%s'", QSTRING_CSTR(configFile));

	QJsonObject config;
	QPair<bool, QStringList> result = JsonUtils::readFile(configFile, config, _log, false);

	if (!result.first)
	{
		QString errorText = QString("Import configuration file '%1' failed!").arg(configFile);
		result.second.prepend(errorText);
		Error(_log, "%s", QSTRING_CSTR(errorText));
		return result;
	}

	DBMigrationManager migtrationManger;
	migtrationManger.migrateSettings(config);

	return updateConfiguration(config, true);
}

bool DBConfigManager::exportJson(const QString& path) const
{
	bool isExported {false};

	QDir exportPath{path};
	if (path.isEmpty())
	{
		exportPath.setPath(getDataDirectory().absoluteFilePath("archive"));
	}

	QString jsonFile;
	if (QDir().mkpath(exportPath.absolutePath()))
	{
		const QJsonObject configurtion = getConfiguration();
		if (!configurtion.isEmpty())
		{
			const QJsonObject generalSettings = configurtion.value("global").toObject().value("settings").toObject().value("general").toObject();
			const QString configVersion = generalSettings.value("configVersion").toString();

			jsonFile = exportPath.absoluteFilePath(QString("HyperionBackup_%1_v%2.json").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd-hhmmss-zzz"), configVersion ));
			if (FileUtils::writeFile(jsonFile, QJsonDocument(configurtion).toJson(QJsonDocument::Indented), _log))
			{
				isExported = true;
			}
		}
	}

	if (isExported)
	{
		Info(_log, "Successfully exported configuration to '%s'", QSTRING_CSTR(jsonFile));
	}
	else
	{
		Error(_log, "Failed to export configuration to '%s'", QSTRING_CSTR(jsonFile));
	}

	return isExported;
}

QPair<bool, QStringList> DBConfigManager::validateConfiguration()
{
		QJsonObject config = getConfiguration();
		return validateConfiguration(config, false);
}

QPair<bool, QStringList> DBConfigManager::validateConfiguration(QJsonObject& config, bool doCorrections)
{
	Info(_log, "Validate configuration%s", doCorrections ? " and apply corrections, if required" : "");

	QStringList errorList;
	if (config.isEmpty())
	{
		QString errorText {"No configuration data provided!"};
		Error(_log, "'%s'", QSTRING_CSTR(errorText));
		errorList.append(errorText);
		return qMakePair (false, errorList );
	}

	QJsonObject schema = QJsonFactory::readSchema(SETTINGS_FULL_SCHEMA_FILE);

	bool wasCorrected {false};
	if (doCorrections)
	{
		QJsonValue configValue(config);
		QPair<bool, QStringList> correctionResult = JsonUtils::correct(__FUNCTION__, configValue, schema, _log);

		wasCorrected = correctionResult.first;
		if (wasCorrected)
		{
			config = configValue.toObject();
		}
	}
	else
	{
		QPair<bool, QStringList> validationResult = JsonUtils::validate(__FUNCTION__, config, schema, _log);
		if (!validationResult.first)
		{
			Error(_log, "Configuration has errors!");
			return qMakePair (false, validationResult.second );
		}
	}

	Info(_log, "Configuration is valid%s", wasCorrected ? ", but had to be corrected" : "");
	return qMakePair (true, errorList );
}

QPair<bool, QStringList> DBConfigManager::updateConfiguration()
{
		QJsonObject config = getConfiguration();
		return updateConfiguration(config, true);
}


QPair<bool, QStringList> DBConfigManager::addMissingDefaults()
{
	Debug(_log, "Add default settings for missing configuration items");

	QStringList errorList;

	SettingsTable globalSettingsTable;
	QPair<bool, QStringList> result = globalSettingsTable.addMissingDefaults();
	errorList.append(result.second);

	InstanceTable instanceTable;

	//Ensure that one initial instance exists
	instanceTable.createInitialInstance();

	const QList<quint8> instances = instanceTable.getAllInstanceIDs();
	for (const auto &instanceIdx : instances)
	{
		SettingsTable instanceSettingsTable(instanceIdx);
		result = instanceSettingsTable.addMissingDefaults();
		errorList.append(result.second);
	}

	if(errorList.isEmpty())
	{
		Debug(_log, "Successfully defaulted settings for missing configuration items");
	}

	return qMakePair (errorList.isEmpty(), errorList );
}

QPair<bool, QStringList> DBConfigManager::updateConfiguration(QJsonObject& config, bool doCorrections)
{
	Info(_log, "Update configuration database");

	QPair<bool, QStringList> validationResult = validateConfiguration(config, doCorrections);
	if (!validationResult.first)
	{
		return validationResult;
	}

	Info(_log, "Create backup of current configuration");
	if (!exportJson())
	{
		Warning(_log, "Backup of current configuration failed");
	}

	QStringList errorList;
	QSqlDatabase idb = getDB();

	if (!startTransaction(idb, errorList))
	{
		return qMakePair(false, errorList);
	}

	// Clear existing tables and import the new configuration.
	bool errorOccurred = false;
	if (!(deleteTable("instances") && deleteTable("settings")))
	{
		errorOccurred = true;
		logErrorAndAppend("Failed to clear tables before import", errorList);
	}
	else
	{
		errorOccurred = !importGlobalSettings(config, errorList) || !importInstances(config, errorList);
	}

	// Rollback if any error occurred during the import process.
	if (errorOccurred)
	{
		if (!rollbackTransaction(idb, errorList))
		{
			return qMakePair(false, errorList);
		}
	}

	commiTransaction(idb, errorList);

	if (errorList.isEmpty())
	{
		Info(_log, "Successfully imported new configuration");
	}

	return qMakePair(errorList.isEmpty(), errorList);
}

// Function to import global settings
bool DBConfigManager::importGlobalSettings(const QJsonObject& config, QStringList& errorList)
{
	SettingsTable settingsTableGlobal;
	const QJsonObject globalConfig = config.value("global").toObject();
	const QJsonObject globalSettings = globalConfig.value("settings").toObject();

	bool errorOccurred = false;
	for (QJsonObject::const_iterator it = globalSettings.constBegin(); it != globalSettings.constEnd(); ++it)
	{
		if (!settingsTableGlobal.createSettingsRecord(it.key(), JsonUtils::jsonValueToQString(it.value())))
		{
			errorOccurred = true;
			logErrorAndAppend("Failed to import global setting", errorList);
		}
	}

	return !errorOccurred;
}

// Function to import instances
bool DBConfigManager::importInstances(const QJsonObject& config, QStringList& errorList)
{
	InstanceTable instanceTable;
	const QJsonArray instancesConfig = config.value("instances").toArray();

	bool errorOccurred = false;
	quint8 instanceIdx = 0;
	for (const auto& instanceItem : instancesConfig)
	{
		if (!importInstance(instanceTable, instanceItem.toObject(), instanceIdx, errorList))
		{
			errorOccurred = true;
		}
		++instanceIdx;
	}

	return !errorOccurred;
}

// Function to import a single instance
bool DBConfigManager::importInstance(InstanceTable& instanceTable, const QJsonObject& instanceConfig, quint8 instanceIdx, QStringList& errorList)
{
	QString instanceName = instanceConfig.value("name").toString(QString("Instance %1").arg(instanceIdx));
	bool isInstanceEnabled = instanceConfig.value("enabled").toBool(true);

	if (!instanceTable.createInstance(instanceName, instanceIdx) ||
		!instanceTable.setEnable(instanceIdx, isInstanceEnabled))
	{
		logErrorAndAppend("Failed to import instance", errorList);
		return false;
	}

	SettingsTable settingsTableInstance(instanceIdx);
	const QJsonObject instanceSettings = instanceConfig.value("settings").toObject();
	return importInstanceSettings(settingsTableInstance, instanceSettings, errorList);
}

// Function to import instance settings
bool DBConfigManager::importInstanceSettings(SettingsTable& settingsTable, const QJsonObject& instanceSettings, QStringList& errorList)
{
	bool errorOccurred = false;
	for (QJsonObject::const_iterator it = instanceSettings.constBegin(); it != instanceSettings.constEnd(); ++it)
	{
		if (!settingsTable.createSettingsRecord(it.key(), JsonUtils::jsonValueToQString(it.value())))
		{
			errorOccurred = true;
			logErrorAndAppend("Failed to import instance setting", errorList);
		}
	}

	return !errorOccurred;
}

QJsonObject DBConfigManager::getConfiguration(const QList<quint8>& instanceIdsFilter, const QStringList& instanceFilteredTypes, const QStringList& globalFilterTypes ) const
{
	QSqlDatabase idb = getDB();

	if (!startTransaction(idb))
	{
		return {};
	}

	InstanceTable instanceTable;
	SettingsTable settingsTable;

	QJsonObject config;

	QJsonObject globalConfig;
	MetaTable metaTable;
	globalConfig.insert("uuid", metaTable.getUUID());
	globalConfig.insert("settings", settingsTable.getSettings(globalFilterTypes));
	config.insert("global", globalConfig);

	QList<quint8> instanceIds {instanceIdsFilter};
	if (instanceIds.isEmpty())
	{
		instanceIds = instanceTable.getAllInstanceIDs();
	}

	QList<quint8> sortedInstanceIds = instanceIds;
	std::sort(sortedInstanceIds.begin(), sortedInstanceIds.end());

	QJsonArray instanceIdList;
	QJsonArray configInstanceList;
	for (const quint8 instanceId : sortedInstanceIds)
	{
		QJsonObject instanceConfig;
		instanceConfig.insert("id",instanceId);
		instanceConfig.insert("name", instanceTable.getNamebyIndex(instanceId));
		instanceConfig.insert("enabled", instanceTable.isEnabled(instanceId));
		instanceConfig.insert("settings", settingsTable.getSettings(static_cast<quint8>(instanceId), instanceFilteredTypes));
		configInstanceList.append(instanceConfig);

		instanceIdList.append(instanceId);
	}

	config.insert("instanceIds", instanceIdList);
	config.insert("instances", configInstanceList);

	if (!commiTransaction(idb))
	{
		return {};
	}

	return config;
}

QPair<bool, QStringList> DBConfigManager::migrateConfiguration()
{
	Info(_log, "Check, if configuration database is required to be migrated");

	DBMigrationManager migtrationManger;
	if (migtrationManger.isMigrationRequired())
	{
		QJsonObject config = getConfiguration();

		if (migtrationManger.migrateSettings(config))
		{
			return updateConfiguration(config, true);
		}
	}

	Info(_log, "Database migration is not required");
	return qMakePair (true, QStringList{} );
}

