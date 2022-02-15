#include "HyperionConfig.h"

#if defined(ENABLE_EFFECTENGINE)
// Python includes
#include <Python.h>
#endif

#include "utils/SysInfo.h"
#include "utils/FileUtils.h"

#include <QHostInfo>
#include <QSysInfo>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include <iostream>
#ifndef _WIN32
#include <unistd.h>
#endif

#ifdef _WIN32
#include <shlobj_core.h>
#endif

std::unique_ptr<SysInfo> SysInfo::_instance = nullptr;

SysInfo::SysInfo()
	: QObject()
{
	_sysinfo.kernelType = QSysInfo::kernelType();
	_sysinfo.kernelVersion = QSysInfo::kernelVersion();
	_sysinfo.architecture = QSysInfo::currentCpuArchitecture();
	_sysinfo.wordSize = QString::number(QSysInfo::WordSize);
	_sysinfo.productType = QSysInfo::productType();
	_sysinfo.productVersion = QSysInfo::productVersion();
	_sysinfo.prettyName = QSysInfo::prettyProductName();
	_sysinfo.hostName = QHostInfo::localHostName();
	_sysinfo.domainName = QHostInfo::localDomainName();
	_sysinfo.isUserAdmin = isUserAdmin();
	getCPUInfo();
	_sysinfo.qtVersion = QT_VERSION_STR;
#if defined(ENABLE_EFFECTENGINE)
	_sysinfo.pyVersion = PY_VERSION;
#endif
}

SysInfo::HyperionSysInfo SysInfo::get()
{
	if (SysInfo::_instance == nullptr)
		SysInfo::_instance = std::unique_ptr<SysInfo>(new SysInfo());

	return SysInfo::_instance->_sysinfo;
}

void SysInfo::getCPUInfo()
{
	QString cpuInfo;
	if (FileUtils::readFile("/proc/cpuinfo", cpuInfo, Logger::getInstance("DAEMON"), true))
	{
		QRegularExpression regEx("^model\\s*:\\s(.*)", QRegularExpression::CaseInsensitiveOption | QRegularExpression::MultilineOption);
		QRegularExpressionMatch match;

		match = regEx.match(cpuInfo);
		if (match.hasMatch())
		{
			_sysinfo.cpuModelType = match.captured(1);
		}

		regEx.setPattern("^model name\\s*:\\s(.*)");
		match = regEx.match(cpuInfo);
		if (match.hasMatch())
		{
			_sysinfo.cpuModelName = match.captured(1);
		}

		regEx.setPattern("^hardware\\s*:\\s(.*)");
		match = regEx.match(cpuInfo);
		if (match.hasMatch())
		{
			_sysinfo.cpuHardware = match.captured(1);
		}

		regEx.setPattern("^revision\\s*:\\s(.*)");
		match = regEx.match(cpuInfo);
		if (match.hasMatch())
		{
			_sysinfo.cpuRevision = match.captured(1);
		}

		regEx.setPattern("^revision\\s*:\\s(.*)");
		match = regEx.match(cpuInfo);
		if (match.hasMatch())
		{
			_sysinfo.cpuRevision = match.captured(1);
		}
	}
}

QString SysInfo::userName()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
	#if defined (WIN32)
		return qEnvironmentVariable("USERNAME");
	#else
		return qEnvironmentVariable("USER");
	#endif
#else
	#if defined (WIN32)
		return getenv("USERNAME");
	#else
		return getenv("USER");
	#endif
#endif
}

bool SysInfo::isUserAdmin()
{
	bool isAdmin{ false };

#ifdef _WIN32
	BOOL b;
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup;
	b = AllocateAndInitializeSid(
		&NtAuthority,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&AdministratorsGroup);
	if (b)
	{
		if (!CheckTokenMembership(NULL, AdministratorsGroup, &b))
		{
			b = false;
		}
		FreeSid(AdministratorsGroup);
	}
	// TRUE - User has Administrators local group.
	// FALSE - User does not have Administrators local group.
	isAdmin = b;
#else
	if (getuid() == 0U)
	{
		isAdmin = true;
	}
#endif
	return isAdmin;
}
