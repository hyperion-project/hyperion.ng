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
		QString qtVersion;
		QString pyVersion;
	};

	static HyperionSysInfo get();

private:
	SysInfo();
	void getCPUInfo();

	static SysInfo* _instance;

	HyperionSysInfo _sysinfo;
};
