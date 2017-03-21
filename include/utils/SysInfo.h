#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>

class SysInfo : public QObject
{

public:
	struct HyperionSysInfo
	{
		QString kernelType;
		QString kernelVersion;
		QString architecture;
		QString wordSize;
		QString productType;
		QString productVersion;
		QString prettyName;
		QString hostName;
		QString domainName;
	};

	~SysInfo();
	static HyperionSysInfo get();

private:
	SysInfo();

	static SysInfo* _instance;

	HyperionSysInfo _sysinfo;

	struct QUnixOSVersion
	{
		QString productType;
		QString productVersion;
		QString prettyName;
	};

	QString machineHostName();
	QString currentCpuArchitecture();
	QString kernelType();
	QString kernelVersion();
	bool findUnixOsVersion(QUnixOSVersion &v);

	QByteArray getEtcFileFirstLine(const char *fileName);
	bool readEtcRedHatRelease(QUnixOSVersion &v);
	bool readEtcDebianVersion(QUnixOSVersion &v);

	bool readEtcOsRelease(SysInfo::QUnixOSVersion &v);
	bool readEtcFile(SysInfo::QUnixOSVersion &v, const char *filename, const QByteArray &idKey, const QByteArray &versionKey, const QByteArray &prettyNameKey);
	QByteArray getEtcFileContent(const char *filename);
	QString unquote(const char *begin, const char *end);
	bool readEtcLsbRelease(SysInfo::QUnixOSVersion &v);
};
