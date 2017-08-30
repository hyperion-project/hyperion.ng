#include "process.h"

Process::Process(QString configPath, QString daemonPath)
	: QObject()
	, _configPath(configPath)
	, _daemonPath(daemonPath)
{
}

Process::~Process()
{
	for (procInfo & info : procInfoList)
	{
		if(info.running)
		{
			info.proc->terminate();
			info.proc->waitForFinished(1000);
		}
		delete info.proc;
	}
}

void Process::createProcess(QString configName)
{
	Process::procInfo p;
	if(!getStructByCName(p, configName))
	{
		QProcess* proc = new QProcess();
		connect(proc, &QProcess::stateChanged, this, &Process::stateChanged);
		connect(proc, &QProcess::readyReadStandardError, this, &Process::readyReadStandardError);
		connect(proc, &QProcess::readyReadStandardOutput, this, &Process::readyReadStandardOutput);
		connect(proc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &Process::finished);
		proc->setArguments(QStringList(_configPath+configName));
		proc->setProgram(_daemonPath);

		procInfo pEntry{proc, configName, false, 0};
		procInfoList.append(pEntry);
		qDebug() << "Create Process:" << configName;
	}

}

bool Process::getStructByCName(Process::procInfo &p, QString cName)
{
	for (const procInfo & info : procInfoList)
	{
		if(info.cName == cName)
		{
			p = info;
			return true;
		}
	}
	return false;
}

bool Process::getStructByProcess(Process::procInfo &p, QProcess* proc)
{
	for (const procInfo & info : procInfoList)
	{
		if(info.proc == proc)
		{
			p = info;
			return true;
		}
	}
	return false;
}

bool Process::updateStateByProcess(QProcess* proc, bool newState)
{
	for (procInfo & info : procInfoList)
	{
		if(info.proc == proc)
		{
			info.running = newState;
			return true;
		}
	}
	return false;
}

bool Process::updateRetryCountByProcess(QProcess* proc, quint8 newCount)
{
	for (procInfo & info : procInfoList)
	{
		if(info.proc == proc)
		{
			info.restCount = newCount;
			return true;
		}
	}
	return false;
}

void Process::startProcessByCName(QString cName)
{
	Process::procInfo p;
	if(getStructByCName(p, cName) && !p.running)
	{
		qDebug() << "Start Process:" << cName;
		p.proc->start();
	}
}

void Process::stopProcessByCName(QString cName)
{
	Process::procInfo p;
	if(getStructByCName(p, cName) && p.running)
	{
		qDebug() << "Stop Process:" << cName;
		updateStateByProcess(p.proc,false);
		p.proc->terminate();
	}
}

void Process::stateChanged(QProcess::ProcessState newState)
{
	QProcess* proc = qobject_cast<QProcess*>(sender());
	Process::procInfo p;

	if(getStructByProcess(p, proc))
	{
		switch (newState) {
		case QProcess::NotRunning:
			qDebug() << "Process state changed to stopped:" << p.cName;
			break;
		case QProcess::Starting:
			qDebug() << "Process state changed to starting:" << p.cName;
			break;
		case QProcess::Running:
			qDebug() << "Process state changed to running:" << p.cName;
			updateStateByProcess(proc,true);
			break;
		}
	}
}

void Process::readyReadStandardError(void)
{
	QProcess* proc = qobject_cast<QProcess*>(sender());
	Process::procInfo p;

	if(getStructByProcess(p, proc))
	{
		qDebug() << p.cName << "ERR:" << proc->readAllStandardError();
	}
}

void Process::readyReadStandardOutput(void)
{
	QProcess* proc = qobject_cast<QProcess*>(sender());
	Process::procInfo p;

	if(getStructByProcess(p, proc))
	{
		qDebug() << p.cName << "OUT:" << proc->readAllStandardOutput();
	}
}

void Process::finished(int exitCode, QProcess::ExitStatus exitStatus)
{
	QProcess* proc = qobject_cast<QProcess*>(sender());
	Process::procInfo p;

	if(getStructByProcess(p, proc))
	{
		// check if the terminate was requested before
		if(p.running)
		{
			if(p.restCount < 6)
			{
				updateStateByProcess(proc,false);
				updateRetryCountByProcess(proc, p.restCount += 1);
				qDebug() << "It seems that the process crashed, restart now" << p.cName << "try" << p.restCount << "of 5";
				p.proc->start();
			}
			else
			{
				qDebug()<< "The process crashed more than 5 times, stop restart attempts for"<< p.cName;
			}
		}
		else
		{
			p.running = false;
			qDebug() << "Proccess" << p.cName << "finished with exitCode:" << exitCode << "and exitStatus:" << exitStatus;
		}
	}
}
