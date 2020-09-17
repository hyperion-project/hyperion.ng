#include "utils/SysInfo.h"

#include <QHostInfo>
#include <QSysInfo>

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
}

SysInfo::HyperionSysInfo SysInfo::get()
{
	if (SysInfo::_instance == nullptr)
		SysInfo::_instance = new SysInfo();

	return SysInfo::_instance->_sysinfo;
}
