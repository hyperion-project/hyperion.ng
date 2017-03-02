#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>

class SysInfo : public QObject
{
//	Q_OBJECT
	
public:
	struct HyperionSysInfo
	{
		QString kernelType;
		QString kernelVersion;
		QString architecture;
		QString wordSize;
		QString productType;    // $ID                          $DISTRIB_ID                    // single line file containing:       // Debian
		QString productVersion; // $VERSION_ID                  $DISTRIB_RELEASE               // <Vendor_ID release Version_ID>     // single line file <Release_ID/sid>
		QString prettyName;     // $PRETTY_NAME                 $DISTRIB_DESCRIPTION
	};

	static HyperionSysInfo get();

private:
	SysInfo();
	~SysInfo();
	static SysInfo* _instance;

	HyperionSysInfo _sysinfo;

	struct QUnixOSVersion
	{
	                        // from /etc/os-release         older /etc/lsb-release         // redhat /etc/redhat-release         // debian /etc/debian_version
	QString productType;    // $ID                          $DISTRIB_ID                    // single line file containing:       // Debian
	QString productVersion; // $VERSION_ID                  $DISTRIB_RELEASE               // <Vendor_ID release Version_ID>     // single line file <Release_ID/sid>
	QString prettyName;     // $PRETTY_NAME                 $DISTRIB_DESCRIPTION
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
