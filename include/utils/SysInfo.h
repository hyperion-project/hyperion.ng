#pragma once

#include <QObject>
#include <QString>

class SysInfo : public QObject
{
public:
	struct HyperionSysInfo
	{
		QString kernelType;
		QString kernelVersion;
		QString architecture;
		QString cpuModelName;
		QString cpuModelType;
		QString cpuRevision;
		QString cpuHardware;
		QString wordSize;
		QString productType;
		QString productVersion;
		QString prettyName;
		QString hostName;
		QString domainName;
		bool isUserAdmin;
		QString qtVersion;
		QString pyVersion;
	};

	static HyperionSysInfo get();

	static bool isUserAdmin();
	static QString userName();

private:
	SysInfo();
	void getCPUInfo();

	static SysInfo* _instance;

	HyperionSysInfo _sysinfo;
};
