#ifndef DBCONFGMANAGER_H
#define DBConfigManager_H

#include <db/DBManager.h>
#include "db/SettingsTable.h"
#include "db/InstanceTable.h"

class DBConfigManager : public DBManager
{
public:
	DBConfigManager(QObject* parent = nullptr);

	QPair<bool, QStringList> importJson(const QString& configFile);
	bool exportJson(const QString& path = "") const;

	QJsonObject getConfiguration(const QList<quint8>& instanceIdsFilter = {}, const QStringList& instanceFilteredTypes = {}, const QStringList& globalFilterTypes = {} ) const;

	QPair<bool, QStringList> validateConfiguration();
	QPair<bool, QStringList> validateConfiguration(QJsonObject& config, bool doCorrections = false);

	QPair<bool, QStringList> addMissingDefaults();

	QPair<bool, QStringList> updateConfiguration();
	QPair<bool, QStringList> updateConfiguration(QJsonObject& config, bool doCorrections = false);

	QPair<bool, QStringList> migrateConfiguration();

private:
	// Function to import global settings from the configuration
	bool importGlobalSettings(const QJsonObject& config, QStringList& errorList);

	// Function to import all instances from the configuration
	bool importInstances(const QJsonObject& config, QStringList& errorList);

	// Function to import a single instance
	bool importInstance(InstanceTable& instanceTable, const QJsonObject& instanceConfig, quint8 instanceIdx, QStringList& errorList);

	// Function to import settings for a specific instance
	bool importInstanceSettings(SettingsTable& settingsTable, const QJsonObject& instanceSettings, QStringList& errorList);
};

#endif // DBCONFGMANAGER_H
