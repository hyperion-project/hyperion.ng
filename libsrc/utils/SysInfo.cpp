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
	_sysinfo.wordSize       = QString::number(QSysInfo::WordSize);
	_sysinfo.productType    = QSysInfo::productType();
	_sysinfo.productVersion = QSysInfo::productVersion();
	_sysinfo.prettyName     = QSysInfo::prettyProductName();
	_sysinfo.hostName       = QHostInfo::localHostName();
	_sysinfo.domainName     = QHostInfo::localDomainName();
	getCPUInfo();
}

SysInfo::HyperionSysInfo SysInfo::get()
{
	if (SysInfo::_instance == nullptr)
		SysInfo::_instance = new SysInfo();

	return SysInfo::_instance->_sysinfo;
}

void SysInfo::getCPUInfo()
{
	QString modelName;

	QFile cpuInfoFile("/proc/cpuinfo");
	if ( cpuInfoFile.open(QIODevice::ReadOnly| QIODevice::Text) )
	{
		QString line;
		do
		{
			line = cpuInfoFile.readLine();

			if ( !line.isEmpty() )
			{
				if ( line.startsWith("model",Qt::CaseInsensitive) )
				{
					QString modelTag = line.split(":").first();
					if ( modelTag.startsWith("model name",Qt::CaseInsensitive) )
					{
						_sysinfo.cpuModelName = line.split(":").last().trimmed();
					}
					else
					{
						_sysinfo.cpuModelType = line.split(":").last().trimmed();
					}
					continue;
				}

				if ( line.startsWith("Revision",Qt::CaseInsensitive) )
				{
					_sysinfo.cpuRevision = line.split(":").last().trimmed();
					continue;
				}
			}
		}
		while ( !line.isNull() );
	}
}

