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
		QString cpuModel;
		QString wordSize;
		QString productType;
		QString productVersion;
		QString prettyName;
		QString hostName;
		QString domainName;
	};

	static HyperionSysInfo get();

private:
	SysInfo();
	const QString getCPUModel() const;

	static SysInfo* _instance;

	HyperionSysInfo _sysinfo;
};
