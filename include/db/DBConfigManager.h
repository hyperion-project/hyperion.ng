#ifndef DBCONFGMANAGER_H
#define DBConfigManager_H

#include <db/DBManager.h>

class DBConfigManager : public DBManager
{
public:
	DBConfigManager(QObject* parent = nullptr);

	// TODO: Check naming seConfiguration
	QPair<bool, QStringList> importJson(const QString& configFile);
	bool exportJson(const QString& path = "") const;

	QJsonObject getConfiguration(const QList<quint8>& instances = {}, const QStringList& instanceFilteredTypes = {}, const QStringList& globalFilterTypes = {} ) const;

	QPair<bool, QStringList> validateConfiguration();
	QPair<bool, QStringList> validateConfiguration(QJsonObject& config, bool doCorrections = false);

	QPair<bool, QStringList> addMissingDefaults();
	//QPair<bool, QStringList> addMissingDefaults(QJsonObject& config);

	QPair<bool, QStringList> updateConfiguration();
	QPair<bool, QStringList> updateConfiguration(QJsonObject& config, bool doCorrections = false);

	QPair<bool, QStringList> migrateConfiguration();
};

#endif // DBCONFGMANAGER_H
