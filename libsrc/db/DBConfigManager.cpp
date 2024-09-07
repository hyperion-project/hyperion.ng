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

			jsonFile = exportPath.absoluteFilePath(QString("HyperionBackup_%1_v%2.json").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh:mm:ss:zzz"), configVersion ));
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

	//Ensure that first instance as default one exists
	instanceTable.createDefaultInstance();

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
	if (!idb.transaction())
	{
		QString errorText = QString("Could not create a database transaction. Error: %1").arg(idb.lastError().text());
		Error(_log, "'%s'", QSTRING_CSTR(errorText));
		errorList.append(errorText);
		return qMakePair (false, errorList );
	}

	bool errorOccured {false};
	if (!deleteTable("instances") || !deleteTable("settings"))
	{
		QString errorText = "Failed to clear tables before import";
		Error(_log, "'%s'", QSTRING_CSTR(errorText));
		errorList.append(errorText);
		errorOccured = true;
	}
	else
	{
		SettingsTable settingsTableGlobal;
		const QJsonObject globalConfig = config.value("global").toObject();
		const QJsonObject globalSettings = globalConfig.value("settings").toObject();

		for (QJsonObject::const_iterator it = globalSettings.constBegin(); it != globalSettings.constEnd(); ++it)
		{
			if (!settingsTableGlobal.createSettingsRecord(it.key(), JsonUtils::jsonValueToQString(it.value())))
			{
				errorOccured = true;
			}
		}

		InstanceTable instanceTable;
		const QJsonArray instancesConfig = config.value("instances").toArray();
		quint8 instanceIdx {0};

		for (const auto &instanceItem : instancesConfig)
		{
			QJsonObject instanceConfig = instanceItem.toObject();
			QString instanceName = instanceConfig.value("name").toString(QString("Instance %1").arg(instanceIdx));
			bool isInstanceEnabled = instanceConfig.value("enabled").toBool(true);

			if (instanceIdx == 0)
			{
				isInstanceEnabled = true;
			}

			if (!instanceTable.createInstance(instanceName, instanceIdx) ||
				!instanceTable.setEnable(instanceIdx, isInstanceEnabled))
			{
				errorOccured = true;
			}

			SettingsTable settingsTableInstance(instanceIdx);
			const QJsonObject instanceSettings = instanceConfig.value("settings").toObject();
			for (QJsonObject::const_iterator it = instanceSettings.constBegin(); it != instanceSettings.constEnd(); ++it)
			{
				if (!settingsTableInstance.createSettingsRecord(it.key(), JsonUtils::jsonValueToQString(it.value())))
				{
					errorOccured = true;
				}
			}
			++instanceIdx;
		}

		if (errorOccured)
		{
			QString errorText = "Errors occured during instances' and/or settings' configuration";
			Error(_log, "'%s'", QSTRING_CSTR(errorText));
			errorList.append(errorText);
		}
	}

	if (errorOccured)
	{
		if (!idb.rollback())
		{
			QString errorText = QString("Could not create a database transaction. Error: %1").arg(idb.lastError().text());
			Error(_log, "'%s'", QSTRING_CSTR(errorText));
			errorList.append(errorText);
		}
	}

	if (!idb.commit())
	{
		QString errorText = QString("Could not finalise the database changes. Error: %1").arg(idb.lastError().text());
		Error(_log, "'%s'", QSTRING_CSTR(errorText));
		errorList.append(errorText);
	}

	if(errorList.isEmpty())
	{
		Info(_log, "Successfully imported new configuration");
	}

	return qMakePair (errorList.isEmpty(), errorList );
}

QJsonObject DBConfigManager::getConfiguration(const QList<quint8>& instancesFilter, const QStringList& instanceFilteredTypes, const QStringList& globalFilterTypes ) const
{
	QSqlDatabase idb = getDB();
	if (!idb.transaction())
	{
		Error(_log, "Could not create a database transaction. Error: %s", QSTRING_CSTR(idb.lastError().text()));
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

	QList<quint8> instances {instancesFilter};
	if (instances.isEmpty())
	{
		instances = instanceTable.getAllInstanceIDs();
	}

	QList<quint8> sortedInstances = instances;
	std::sort(sortedInstances.begin(), sortedInstances.end());

	QJsonArray instanceIdList;
	QJsonArray configInstanceList;
	for (const quint8 instanceIdx : sortedInstances)
	{
		QJsonObject instanceConfig;
		instanceConfig.insert("id",instanceIdx);
		instanceConfig.insert("name", instanceTable.getNamebyIndex(instanceIdx));
		instanceConfig.insert("enabled", instanceTable.isEnabled(instanceIdx));
		instanceConfig.insert("settings", settingsTable.getSettings(static_cast<quint8>(instanceIdx), instanceFilteredTypes));
		configInstanceList.append(instanceConfig);

		instanceIdList.append(instanceIdx);
	}

	config.insert("instanceIds", instanceIdList);
	config.insert("instances", configInstanceList);

	if (!idb.commit())
	{
		Error(_log, "Could not finalise a database transaction. Error: %s", QSTRING_CSTR(idb.lastError().text()));
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

