#include "utils/SysInfo.h"

#include <QHostInfo>
#include <QSysInfo>
#include <QFile>

SysInfo* SysInfo::_instance = nullptr;

SysInfo::SysInfo()
	: QObject()
{
	_sysinfo.kernelType     = QSysInfo::kernelType();
	_sysinfo.kernelVersion  = QSysInfo::kernelVersion();
	_sysinfo.architecture   = QSysInfo::currentCpuArchitecture();
	_sysinfo.cpuModel		= getCPUModel();
	_sysinfo.wordSize       = QString::number(QSysInfo::WordSize);
	_sysinfo.productType    = QSysInfo::productType();
	_sysinfo.productVersion = QSysInfo::productVersion();
	_sysinfo.prettyName     = QSysInfo::prettyProductName();
	_sysinfo.hostName       = QHostInfo::localHostName();
	_sysinfo.domainName     = QHostInfo::localDomainName();
}

SysInfo::HyperionSysInfo SysInfo::get()
{
	if (SysInfo::_instance == nullptr)
		SysInfo::_instance = new SysInfo();

	return SysInfo::_instance->_sysinfo;
}

const QString SysInfo::getCPUModel() const
{
	QString modelName;

	QFile cpuInfoFile("/proc/cpuinfo");
	if (cpuInfoFile.open(QIODevice::ReadOnly| QIODevice::Text))
	{
		QString line;
		do
		{
			line = cpuInfoFile.readLine();

			if (!line.isEmpty())
			{
				if (line.startsWith("model name"))
				{
					modelName = line.split(":").last().trimmed();
					break;
				}
			}
		}
		while ( !line.isNull() );
	}
	return modelName;
}

