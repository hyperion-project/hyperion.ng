#ifndef DBMIGRATIONMANAGER_H
#define DBMIGRATIONMANAGER_H

#include <db/DBManager.h>
#include <utils/version.hpp>

#include <QObject>

class DBMigrationManager : public DBManager
{
	Q_OBJECT
public:
	explicit DBMigrationManager(QObject *parent = nullptr);

	bool isMigrationRequired();
	bool migrateSettings(QJsonObject& config);

private:

	bool upgradeGlobalSettings(const semver::version& currentVersion, QJsonObject& config);
	bool upgradeGlobalSettings_alpha_9(semver::version& currentVersion, QJsonObject& config);
	bool upgradeGlobalSettings_2_0_12(semver::version& currentVersion, QJsonObject& config);
	bool upgradeGlobalSettings_2_0_16(semver::version& currentVersion, QJsonObject& config);
	bool upgradeGlobalSettings_2_1_0(semver::version& currentVersion, QJsonObject& config);

	bool upgradeInstanceSettings(const semver::version& currentVersion, quint8 instance, QJsonObject& config);
	bool upgradeInstanceSettings_alpha_9(semver::version& currentVersion, quint8 instance, QJsonObject& config);
	bool upgradeInstanceSettings_2_0_12(semver::version& currentVersion, quint8 instance, QJsonObject& config);
	bool upgradeInstanceSettings_2_0_13(semver::version& currentVersion, quint8 instance, QJsonObject& config);
	bool upgradeInstanceSettings_2_0_16(semver::version& currentVersion, quint8 instance, QJsonObject& config);
	bool upgradeInstanceSettings_2_1_0(semver::version& currentVersion, quint8 instance, QJsonObject& config);
};

#endif // DBMIGRATIONMANAGER_H
