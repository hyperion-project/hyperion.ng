#include "db/SettingsTable.h"
#include <db/MetaTable.h>
#include <db/ConfigImportExport.h>

#include <db/InstanceTable.h>
#include <hyperion/SettingsManager.h>

#include <utils/JsonUtils.h>
#include <utils/jsonschema/QJsonFactory.h>

#include <HyperionConfig.h>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QDateTime>

ConfigImportExport::ConfigImportExport(QObject* parent)
	: DBManager(parent)
{
	Q_INIT_RESOURCE(DB_schemas);
}

QPair<bool, QStringList> ConfigImportExport::importJson(const QString& configFile)
{
	Info(_log,"Import configuration file '%s'", QSTRING_CSTR(configFile));

	QJsonObject config;
	QPair<bool, QStringList> result = JsonUtils::readFile(configFile, config, _log, false);

	if (!result.first)
	{
		QString errorText = QString("Import configuration file '%s' failed!").arg(configFile);
		result.second.prepend(errorText);
		Error(_log, "'%s'", QSTRING_CSTR(errorText));
		return result;
	}
	return setConfiguration(config);
}

bool ConfigImportExport::exportJson(const QString& path) const
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

QPair<bool, QStringList> ConfigImportExport::setConfiguration(const QJsonObject& config)
{
	Debug(_log, "Start import JSON configuration");

	QStringList errorList;
	if (config.isEmpty())
	{
		QString errorText {"No configuration data provided!"};
		Error(_log, "'%s'", QSTRING_CSTR(errorText));
		errorList.append(errorText);
		return qMakePair (false, errorList );
	}

	// check basic message
	QJsonObject schema = QJsonFactory::readSchema(":schema-config-exchange");
	QPair<bool, QStringList> validationResult = JsonUtils::validate("importConfig", config, schema, _log);
	if (!validationResult.first)
	{
		Error(_log, "Invalid JSON configuration data provided!");
		return qMakePair (false, validationResult.second );
	}

	Info(_log, "Create backup of current configuration");
	if (!exportJson())
	{
		Warning(_log, "Backup of current configuration failed");
	}

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
		SettingsTable settingsTableGlobal(GLOABL_INSTANCE_ID);
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

		for (auto instanceItem : instancesConfig)
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

QJsonObject ConfigImportExport::getConfiguration(const QList<quint8>& instancesFilter, const QStringList& instanceFilteredTypes, const QStringList& globalFilterTypes ) const
{
	QSqlDatabase idb = getDB();
	if (!idb.transaction())
	{
		Error(_log, "Could not create a database transaction. Error: %s", QSTRING_CSTR(idb.lastError().text()));
		return {};
	}

	InstanceTable instanceTable;
	SettingsManager settingsManager(0, nullptr);

	QJsonObject config;

	QJsonObject globalConfig;
	MetaTable metaTable;
	globalConfig.insert("uuid", metaTable.getUUID());
	globalConfig.insert("settings", settingsManager.getSettings({}, globalFilterTypes));
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
		instanceConfig.insert("settings", settingsManager.getSettings(static_cast<quint8>(instanceIdx), instanceFilteredTypes));
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
