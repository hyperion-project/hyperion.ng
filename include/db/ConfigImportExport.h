#ifndef CONFIGIMPORTEXPORT_H
#define CONFIGIMPORTEXPORT_H

#include <db/DBManager.h>

#include <QFile>
#include <QJsonDocument>

class ConfigImportExport : public DBManager
{
public:
	ConfigImportExport(QObject* parent = nullptr);

	// TODO: Check naming seConfiguration
	QPair<bool, QStringList> importJson(const QString& configFile);
	bool exportJson(const QString& path = "") const;

	QPair<bool, QStringList> setConfiguration(const QJsonObject& config);
	QJsonObject getConfiguration(const QList<quint8>& instances = {}, bool addGlobalConfig = true, const QStringList& instanceFilteredTypes = {}, const QStringList& globalFilterTypes = {} ) const;
};

#endif // CONFIGIMPORTEXPORT_H
