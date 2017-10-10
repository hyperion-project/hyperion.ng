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
	bool isRunning(QString configName);
	void startProcessByCName(QString cName);
	void stopProcessByCName(QString cName);
	bool getStructByCName(Process::procInfo &p, QString cName);

private slots:
	void stateChanged(QProcess::ProcessState newState);
	void readyReadStandardError(void);
	void readyReadStandardOutput(void);
	void finished(int exitCode, QProcess::ExitStatus exitStatus);

private:
	QString _configPath;
	QString _daemonPath;

	bool getStructByProcess(Process::procInfo &p, QProcess* proc);
	bool updateStateByProcess(QProcess* proc, bool newState);
	bool updateRetryCountByProcess(QProcess* proc, quint8 newCount);
};
