#include "utils/SysInfo.h"

#include <QHostInfo>
#include <QSysInfo>
#include <iostream>
#include <sys/utsname.h>
#include "HyperionConfig.h"

#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

SysInfo* SysInfo::_instance = nullptr;

SysInfo::SysInfo()
	: QObject()
{
	SysInfo::QUnixOSVersion v;
	findUnixOsVersion(v);

	_sysinfo.kernelType     = kernelType();
	_sysinfo.kernelVersion  = kernelVersion();
	_sysinfo.architecture   = currentCpuArchitecture();
	_sysinfo.wordSize       = QString::number(QSysInfo::WordSize);
	_sysinfo.productType    = v.productType;
	_sysinfo.productVersion = v.productVersion;
	_sysinfo.prettyName     = v.prettyName;
	_sysinfo.hostName       = QHostInfo::localHostName();
	_sysinfo.domainName       = QHostInfo::localDomainName();
}

SysInfo::~SysInfo()
{
}

SysInfo::HyperionSysInfo SysInfo::get()
{
	if ( SysInfo::_instance == nullptr )
		SysInfo::_instance = new SysInfo();
	
	return SysInfo::_instance->_sysinfo;
}



QString SysInfo::kernelType()
{
#if defined(Q_OS_WIN)
	return QStringLiteral("winnt");
#elif defined(Q_OS_UNIX)
	struct utsname u;
	if (uname(&u) == 0)
		return QString::fromLatin1(u.sysname).toLower();
#endif
	return QString();
}

QString SysInfo::kernelVersion()
{
	struct utsname u;
	if (uname(&u) == 0)
		return QString::fromLocal8Bit(u.release).toLower();

	return QString();
}

QString SysInfo::machineHostName()
{
#if defined(Q_OS_LINUX)
	// gethostname(3) on Linux just calls uname(2), so do it ourselves and avoid a memcpy
	struct utsname u;
	if (uname(&u) == 0)
		return QString::fromLocal8Bit(u.nodename);
#else
	char hostName[512];
	if (gethostname(hostName, sizeof(hostName)) == -1)
		return QString();
	hostName[sizeof(hostName) - 1] = '\0';
	return QString::fromLocal8Bit(hostName);
#endif
	return QString();
}


QString SysInfo::currentCpuArchitecture()
{
#if defined(Q_OS_UNIX)
	long ret = -1;
	struct utsname u;

	if (ret == -1)
		ret = uname(&u);

	// we could use detectUnixVersion() above, but we only need a field no other function does
	if (ret != -1)
	{
		// the use of QT_BUILD_INTERNAL here is simply to ensure all branches build
		// as we don't often build on some of the less common platforms
#  if defined(Q_PROCESSOR_ARM)
		if (strcmp(u.machine, "aarch64") == 0)
			return QStringLiteral("arm64");
		if (strncmp(u.machine, "armv", 4) == 0)
			return QStringLiteral("arm");
#  endif
#  if defined(Q_PROCESSOR_POWER)
		// harmonize "powerpc" and "ppc" to "power"
		if (strncmp(u.machine, "ppc", 3) == 0)
			return QLatin1String("power") + QLatin1String(u.machine + 3);
		if (strncmp(u.machine, "powerpc", 7) == 0)
			return QLatin1String("power") + QLatin1String(u.machine + 7);
		if (strcmp(u.machine, "Power Macintosh") == 0)
			return QLatin1String("power");
#  endif
#  if defined(Q_PROCESSOR_X86)
		// harmonize all "i?86" to "i386"
		if (strlen(u.machine) == 4 && u.machine[0] == 'i' && u.machine[2] == '8' && u.machine[3] == '6')
			return QStringLiteral("i386");
		if (strcmp(u.machine, "amd64") == 0) // Solaris
			return QStringLiteral("x86_64");
#  endif
		return QString::fromLatin1(u.machine);
	}
#endif
	return QString();
}

bool SysInfo::findUnixOsVersion(SysInfo::QUnixOSVersion &v)
{
	if (readEtcOsRelease(v))
		return true;
	if (readEtcLsbRelease(v))
		return true;
#if defined(Q_OS_LINUX)
	if (readEtcRedHatRelease(v))
		return true;
	if (readEtcDebianVersion(v))
		return true;
#endif
	return false;
}


QByteArray SysInfo::getEtcFileFirstLine(const char *fileName)
{
	QByteArray buffer = getEtcFileContent(fileName);
	if (buffer.isEmpty())
		return QByteArray();

	const char *ptr = buffer.constData();
	int eol = buffer.indexOf("\n");
	return QByteArray(ptr, eol).trimmed();
}

bool SysInfo::readEtcRedHatRelease(SysInfo::QUnixOSVersion &v)
{
	// /etc/redhat-release analysed should be a one line file
	// the format of its content is <Vendor_ID release Version>
	// i.e. "Red Hat Enterprise Linux Workstation release 6.5 (Santiago)"
	QByteArray line = getEtcFileFirstLine("/etc/redhat-release");
	if (line.isEmpty())
		return false;

	v.prettyName = QString::fromLatin1(line);

	const char keyword[] = "release ";
	int releaseIndex = line.indexOf(keyword);
	v.productType = QString::fromLatin1(line.mid(0, releaseIndex)).remove(QLatin1Char(' '));
	int spaceIndex = line.indexOf(' ', releaseIndex + strlen(keyword));
	v.productVersion = QString::fromLatin1(line.mid(releaseIndex + strlen(keyword),
													spaceIndex > -1 ? spaceIndex - releaseIndex - int(strlen(keyword)) : -1));
	return true;
}

bool SysInfo::readEtcDebianVersion(SysInfo::QUnixOSVersion &v)
{
	// /etc/debian_version analysed should be a one line file
	// the format of its content is <Release_ID/sid>
	// i.e. "jessie/sid"
	QByteArray line = getEtcFileFirstLine("/etc/debian_version");
	if (line.isEmpty())
		return false;

	v.productType = QStringLiteral("Debian");
	v.productVersion = QString::fromLatin1(line);
	return true;
}

QString SysInfo::unquote(const char *begin, const char *end)
{
	if (*begin == '"') {
		Q_ASSERT(end[-1] == '"');
		return QString::fromLatin1(begin + 1, end - begin - 2);
	}
	return QString::fromLatin1(begin, end - begin);
}

QByteArray SysInfo::getEtcFileContent(const char *filename)
{
	// we're avoiding QFile here
	int fd = open(filename, O_RDONLY);
	if (fd == -1)
		return QByteArray();

	struct stat sbuf;
	if (::fstat(fd, &sbuf) == -1) {
		close(fd);
		return QByteArray();
	}

	QByteArray buffer(sbuf.st_size, Qt::Uninitialized);
	buffer.resize(read(fd, buffer.data(), sbuf.st_size));
	close(fd);
	return buffer;
}

bool SysInfo::readEtcFile(SysInfo::QUnixOSVersion &v, const char *filename,
						const QByteArray &idKey, const QByteArray &versionKey, const QByteArray &prettyNameKey)
{

	QByteArray buffer = getEtcFileContent(filename);
	if (buffer.isEmpty())
		return false;

	const char *ptr = buffer.constData();
	const char *end = buffer.constEnd();
	const char *eol;
	QByteArray line;
	for ( ; ptr != end; ptr = eol + 1) {
		// find the end of the line after ptr
		eol = static_cast<const char *>(memchr(ptr, '\n', end - ptr));
		if (!eol)
			eol = end - 1;
		line.setRawData(ptr, eol - ptr);

		if (line.startsWith(idKey)) {
			ptr += idKey.length();
			v.productType = unquote(ptr, eol);
			continue;
		}

		if (line.startsWith(prettyNameKey)) {
			ptr += prettyNameKey.length();
			v.prettyName = unquote(ptr, eol);
			continue;
		}

		if (line.startsWith(versionKey)) {
			ptr += versionKey.length();
			v.productVersion = unquote(ptr, eol);
			continue;
		}
	}

	return true;
}

bool SysInfo::readEtcOsRelease(SysInfo::QUnixOSVersion &v)
{
	return readEtcFile(v, "/etc/os-release", QByteArrayLiteral("ID="),
					   QByteArrayLiteral("VERSION_ID="), QByteArrayLiteral("PRETTY_NAME="));
}

bool SysInfo::readEtcLsbRelease(SysInfo::QUnixOSVersion &v)
{
	bool ok = readEtcFile(v, "/etc/lsb-release", QByteArrayLiteral("DISTRIB_ID="),
						  QByteArrayLiteral("DISTRIB_RELEASE="), QByteArrayLiteral("DISTRIB_DESCRIPTION="));
	if (ok && (v.prettyName.isEmpty() || v.prettyName == v.productType)) {
		// some distributions have redundant information for the pretty name,
		// so try /etc/<lowercasename>-release

		// we're still avoiding QFile here
		QByteArray distrorelease = "/etc/" + v.productType.toLatin1().toLower() + "-release";
		int fd = open(distrorelease, O_RDONLY);
		if (fd != -1) {
			struct stat sbuf;
			if (::fstat(fd, &sbuf) != -1 && sbuf.st_size > v.prettyName.length()) {
				// file apparently contains interesting information
				QByteArray buffer(sbuf.st_size, Qt::Uninitialized);
				buffer.resize(read(fd, buffer.data(), sbuf.st_size));
				v.prettyName = QString::fromLatin1(buffer.trimmed());
			}
			close(fd);
		}
	}

	// some distributions have a /etc/lsb-release file that does not provide the values
	// we are looking for, i.e. DISTRIB_ID, DISTRIB_RELEASE and DISTRIB_DESCRIPTION.
	// Assuming that neither DISTRIB_ID nor DISTRIB_RELEASE were found, or contained valid values,
	// returning false for readEtcLsbRelease will allow further /etc/<lowercasename>-release parsing.
	return ok && !(v.productType.isEmpty() && v.productVersion.isEmpty());
}




