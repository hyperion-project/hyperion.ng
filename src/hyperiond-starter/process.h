#pragma once

#include <QObject>
#include <QString>
#include <QProcess>
#include <QVector>

#include <QDebug>

class Process : public QObject
{
	Q_OBJECT

public:
	Process(QString configPath, QString daemonPath);
	~Process();

	struct procInfo{
		QProcess* proc;
		QString   cName;
		bool      running;
		quint8    restCount;
	};

	QVector<procInfo> procInfoList;
    void createProcess(QString configName);
	bool isCreated(QString cName);
	bool isRunning(QString configName);
	void startProcessByCName(QString cName);
	void stopProcessByCName(QString cName);

private slots:
	void startProcess(QProcess* proc);
	void stopProcess(QProcess* proc);
    void stateChanged(QProcess::ProcessState newState);
    void readyReadStandardError(void);
    void readyReadStandardOutput(void);
    void finished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QString _configPath;
    QString _daemonPath;
};
