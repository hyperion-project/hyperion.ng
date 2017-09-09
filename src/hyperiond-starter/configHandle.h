#pragma once

#include "process.h"

#include <QResource>
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTcpServer>
#include <QProcess>
#include <QFileSystemWatcher>
#include <QStringList>
#include <QDebug>

// Hyperion includes
#include <utils/jsonschema/QJsonUtils.h>

class configHandle : public QObject
{
	Q_OBJECT
public:
	configHandle(QString configPath, QString daemonPath);
	~configHandle();

private:
	QString _configPath;
	Process _process;
	QFileSystemWatcher _fWatch;
	QList<quint16> _usedPorts;

	void modifyJsonValue(QJsonValue& destValue, const QString& path, const QJsonValue& newValue);
	void modifyJsonValue(QJsonDocument& doc, const QString& path, const QJsonValue& newValue);
	quint16 checkPort(quint16 port, bool incOne = false);
	bool configExists(QString configName);
	bool createConfig(QString configName, QString pName);
	QString convertToCName(QString pName);
	void processConfigChange(QString configPath);
	void readAllPorts(void);
	QString readFile(QString cfile);

private slots:
	void fileChanged(const QString path);

};
