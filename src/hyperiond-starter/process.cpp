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
			info.proc->waitForFinished(2000);
		}
		delete info.proc;
	}
}

void Process::createProcess(QString configName)
{
	if(!isCreated(configName))
	{
	    QProcess* proc = new QProcess();
	    connect(proc, &QProcess::stateChanged, this, &Process::stateChanged);
	    connect(proc, &QProcess::readyReadStandardError, this, &Process::readyReadStandardError);
	    connect(proc, &QProcess::readyReadStandardOutput, this, &Process::readyReadStandardOutput);
	    connect(proc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &Process::finished);
	    proc->setArguments(QStringList(_configPath+configName));
	    proc->setProgram(_daemonPath);

		procInfo pEntry;
		pEntry.proc = proc;
		pEntry.cName = configName;
		pEntry.running = false;
		pEntry.restCount= 0;
		procInfoList.append(pEntry);
	}

}

bool Process::isCreated(QString cName)
{
	for (const procInfo & info : procInfoList)
	{
		if(info.cName == cName)
		{
			return true;
		}
	}
	return false;
}

void Process::startProcess(QProcess* proc)
{
	proc->start();
}

void Process::startProcessByCName(QString cName)
{
	for (const procInfo & info : procInfoList)
	{
		if(info.cName == cName)
		{
			if(!info.running)
			{
				qDebug() << "start process:" << cName;
				info.proc->start();
			}
			break;
		}
	}
}

void Process::stopProcess(QProcess* proc)
{
	proc->terminate();
}

void Process::stopProcessByCName(QString cName)
{
	for (procInfo & info : procInfoList)
	{
		if(info.cName == cName)
		{
			if(info.running)
			{
				qDebug() << "stop process:" << cName;
				info.running = false;
				info.proc->terminate();
			}
			break;
		}
	}
}

void Process::stateChanged(QProcess::ProcessState newState)
{
    // get the sender
    QProcess* proc = qobject_cast<QProcess*>(sender());

	for (procInfo & info : procInfoList)
	{
		if(info.proc == proc)
		{
			switch (newState) {
		    case QProcess::NotRunning:
				qDebug() << "Process state changed to stopped:" << info.cName;
		        break;
		    case QProcess::Starting:
		        qDebug() << "Process state changed to starting:" << info.cName;
		        break;
		    case QProcess::Running:
				qDebug() << "Process state changed to running:" << info.cName;
				info.running = true;
		        break;
		    }
			break;
		}
	}

}

void Process::readyReadStandardError(void)
{
    QProcess* proc = qobject_cast<QProcess*>(sender());

	for (procInfo & info : procInfoList)
	{
		if(info.proc == proc)
		{
			qDebug() << "ERR:" << info.cName << ":" <<proc->readAllStandardError();
			break;
		}
	}
}

void Process::readyReadStandardOutput(void)
{
    QProcess* proc = qobject_cast<QProcess*>(sender());

	for (procInfo & info : procInfoList)
	{
		if(info.proc == proc)
		{
			qDebug() << "OUT:" << info.cName << ":" <<proc->readAllStandardOutput();
			break;
		}
	}
}

void Process::finished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QProcess* proc = qobject_cast<QProcess*>(sender());
	for (procInfo & info : procInfoList)
	{
		if(info.proc == proc)
		{
			// check if the terminate was requested before
			if(info.running)
			{
				if(info.restCount < 6)
				{
					info.running = false;
					info.restCount += 1;
					qDebug() << "It seems that the process crashed, restart now" << info.cName << "try" << info.restCount << "of 5";
					startProcess(proc);
				}
				else
				{
					qDebug()<< "The process crashed more than 5 times, stop restart attempts for"<< info.cName;
				}
			}
			else
			{
			    qDebug() << "Proccess" << info.cName << "finished with exitCode" << exitCode << "and exitStatus:" << exitStatus;
			}

		}
	}
}
