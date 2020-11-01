#include "utils/SysInfo.h"
#include "utils/FileUtils.h"

#include <QHostInfo>
#include <QSysInfo>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include <iostream>

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
	QString cpuInfo;
	if( FileUtils::readFile("/proc/cpuinfo", cpuInfo, Logger::getInstance("DAEMON"), true) )
	{
		QRegularExpression regEx ("^model\\s*:\\s(.*)", QRegularExpression::CaseInsensitiveOption | QRegularExpression::MultilineOption);
		QRegularExpressionMatch match;

		match = regEx.match(cpuInfo);
		if ( match.hasMatch() )
		{
			_sysinfo.cpuModelType = match.captured(1);
		}

		regEx.setPattern("^model name\\s*:\\s(.*)");
		match = regEx.match(cpuInfo);
		if ( match.hasMatch() )
		{
			_sysinfo.cpuModelName = match.captured(1);
		}

		regEx.setPattern("^hardware\\s*:\\s(.*)");
		match = regEx.match(cpuInfo);
		if ( match.hasMatch() )
		{
			_sysinfo.cpuHardware = match.captured(1);
		}

		regEx.setPattern("^revision\\s*:\\s(.*)");
		match = regEx.match(cpuInfo);
		if ( match.hasMatch() )
		{
			_sysinfo.cpuRevision = match.captured(1);
		}

		regEx.setPattern("^revision\\s*:\\s(.*)");
		match = regEx.match(cpuInfo);
		if ( match.hasMatch() )
		{
			_sysinfo.cpuRevision = match.captured(1);
		}
	}
}

