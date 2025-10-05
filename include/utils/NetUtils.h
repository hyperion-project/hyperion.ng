#pragma once

#include <QTcpServer>
#include <QUrl>
#include <QHostAddress>
#include <QHostInfo>
#include <QThread>
#include <QEventLoop>
#include <QTimer>
#include <QJsonArray>
#include <utility>

#include <HyperionConfig.h>
#include <utils/Logger.h>

#ifdef ENABLE_MDNS
#include <mdns/MdnsBrowser.h>
#include <mdns/MdnsServiceRegister.h>
#endif

namespace NetUtils {

const int MAX_PORT = 65535;

///
/// @brief Check if the port is available for listening
/// @param[in/out] port  The port to test, will be incremented if port is in use
/// @param         log   The logger of the caller to print
/// @return        True on success else false
///
inline bool portAvailable(quint16& port, Logger* log)
{
	const quint16 prevPort = port;
	QTcpServer server;
	while (!server.listen(QHostAddress::Any, port))
	{
		Warning(log,"Port '%d' is already in use, will increment", port);
		port ++;
	}
	server.close();
	if(port != prevPort)
	{
		Warning(log, "The requested Port '%d' is already in use, will use Port '%d' instead", prevPort, port);
		return false;
	}
	return true;
}

///
/// @brief Check if the port is in the valid range
/// @param     log   The logger of the caller to print///
/// @param[in] port  The port to be tested (port = -1 is ignored for testing)
/// @param[in] host  A hostname/IP-address to make reference to during logging
/// @return          True on success else false
///
inline bool isValidPort(Logger* log, int port, const QString& host)
{
	if ((port <= 0 || port > MAX_PORT) && port != -1)
	{
		Error(log, "Invalid port [%d] for host: [%s]! - Port must be in range [1 - %d]", port, QSTRING_CSTR(host), MAX_PORT);
		return false;
	}
	return true;
}

///
/// @brief Get host and port from an host address
/// @param[in]     address Hostname or IP-address with or without port (e.g. 192.168.1.100:4711, 2003:e4:c73a:8e00:d5bb:dc3c:50cb:c76e, hyperion.fritz.box)
/// @param[in/out] host    The resolved hostname or IP-address
/// @param[in/out] port    The resolved port, if available.
/// @return        True on success else false
///
inline bool resolveHostPort(const QString& address, QString& host, int& port)
{
	if (address.isEmpty())
	{
		return false;
	}

	QString testUrl;
	if (address.at(0) != '[' && address.count(':') > 1)
	{
		testUrl = QString("http://[%1]").arg(address);
	}
	else
	{
		testUrl = QString("http://%1").arg(address);
	}

	QUrl const url(testUrl);
	if (!url.isValid())
	{
		return false;
	}

	host = url.host();
	if (url.port() != -1)
	{
		port = url.port();
	}
	return true;
}

///
/// @brief Trigger discovery of mDNS services
/// @param[in]     serviceType    Services to be discovered
///
inline void discoverMdnsServices(const QString& serviceType)
{
#ifdef ENABLE_MDNS
		QMetaObject::invokeMethod(MdnsBrowser::getInstance().data(), "browseForServiceType",
								  Qt::QueuedConnection, Q_ARG(QByteArray, MdnsServiceRegister::getServiceType(serviceType)));
#endif		
}

inline QJsonArray getMdnsServicesDiscovered(const QString& serviceType)
{
	return MdnsBrowser::getInstance().data()->getServicesDiscoveredJson(
		MdnsServiceRegister::getServiceType(serviceType),
		MdnsServiceRegister::getServiceNameFilter(serviceType),
		DEFAULT_DISCOVER_TIMEOUT);
}

///
/// @brief Resolve a mDNS service name into a nmDNS serice record including mDNS hostname & port for the given service instance
/// @param[in/out] log             The logger of the caller to print
/// @param[in]     serviceInstance The service instance to be resolved
/// @return        A service record
///
#ifdef ENABLE_MDNS
inline QMdnsEngine::Record resolveMdnsServiceRecord(const QByteArray& serviceInstance)
{
	QMdnsEngine::Record serviceRecord;

	if (MdnsBrowser::isMdnsService(serviceInstance))
	{
		qRegisterMetaType<QMdnsEngine::Record>("QMdnsEngine::Record");

		MdnsBrowser* browser = MdnsBrowser::getInstance().get();
		QEventLoop loop;

		// Connect the signal to capture the resolved service record
		QObject::connect(browser, &MdnsBrowser::isServiceRecordResolved, &loop, [&serviceInstance, &serviceRecord, &loop](const QByteArray& resolvedServiceInstance, const QMdnsEngine::Record& resolvedServiceRecord) {
			if (serviceInstance == resolvedServiceInstance)
			{
				serviceRecord = resolvedServiceRecord;
				loop.quit();
			}
		});

		// Call the function asynchronously in MdnsBrowser's thread
		QMetaObject::invokeMethod(browser, "resolveServiceInstance",
								  Qt::QueuedConnection,  // Ensures it runs in the correct thread
								  Q_ARG(QByteArray, serviceInstance)
								  );

		// Wait for the result
		loop.exec();
	}
	return serviceRecord;
}
#endif

///
/// @brief Resolve an mDNS hostname into an IP-address. A given IP-address will be passed through
/// @param[in/out] log         The logger of the caller to print
/// @param[in]     hostname    The mDNS hostname to be resolved
/// @param[out]    hostAddress The resolved IP-Address
/// @return        True on success else false
///
inline bool resolveMdnsHostToAddress(Logger *log, QString &hostname)
{
	if (hostname.isEmpty())
	{
		return false;
	}

	if (!MdnsBrowser::isMdns(hostname))
	{
		Debug(log, "Given name [%s] is not an mDNS hostname, no mDNS resolution is required", QSTRING_CSTR(hostname));
		return true;
	}

#ifdef ENABLE_MDNS
	MdnsBrowser *browser = MdnsBrowser::getInstance().get();
	QEventLoop loop;

	QString address{hostname};
	// Connect the signal to capture the resolved address
	QObject::connect(browser, &MdnsBrowser::isFirstAddressResolved, &loop, [&hostname, &address, &loop](const QString &resolvedHostname, const QHostAddress &addr) {
				if (hostname == resolvedHostname)
				{
					address = addr.toString();
					loop.quit();
				} });

	// Call the function asynchronously in MdnsBrowser's thread
	QMetaObject::invokeMethod(browser, "resolveFirstAddress",
							  Qt::QueuedConnection, // Ensures it runs in the correct thread
							  Q_ARG(Logger *, log),
							  Q_ARG(QString, hostname));

	// Wait for the result
	loop.exec();

	if (address.isNull())
	{
		Debug(log, "Failed resolving mDNS hostname [%s] into an IP-address", QSTRING_CSTR(hostname));
		return false;
	}
	else
	{
		Debug(log, "Resolved mDNS hostname [%s] to IP-address [%s]", QSTRING_CSTR(hostname), QSTRING_CSTR(address));
		hostname = address;
	}
	return true;
#else
	return false;
#endif
}

///
/// @brief Resolve a mDNS service- or hostname into an IP-address.
/// @param[in/out] log         The logger of the caller to print
/// @param[in/out] hostname    The mDNS service- or hostname to be resolved, if mDNS the input mdnsName is replaced with the resolved IP-address
/// @param[in/out] port        The port provided by the mDNS service, if not mDNS the input port is returned
/// @return        True on success else false
///
inline bool convertMdnsToIp(Logger* log, QString& mdnsName, int& port)
{
	if (!MdnsBrowser::isMdns(mdnsName))
	{
		Debug(log, "Given name [%s] is not an mDNS name, no mDNS resolution is required", QSTRING_CSTR(mdnsName));
		return true;
	}

	// 1. Treat mdnsName as service instance name that requires to be resolved into an mDNS-Hostname
	QString mdnsHostname{mdnsName};
	if (MdnsBrowser::isMdnsService(mdnsName))
	{
		Debug(log, "Given hostname [%s] is an mDNS service name, will try to resolve it into an mDNS hostname", QSTRING_CSTR(mdnsName));
#ifdef ENABLE_MDNS
		QMdnsEngine::Record const service = resolveMdnsServiceRecord(mdnsName.toUtf8());
		if (!service.target().isEmpty())
		{
			Info(log, "Resolved mDNS service [%s] to the mDNS hostname [%s], service port [%d]", QSTRING_CSTR(mdnsName), service.target().constData(), service.port());
			mdnsHostname = service.target();
			port = service.port();
		}
		else
		{
			Error(log, "Failed to resolve the mDNS service [%s] into an mDNS hostname!", QSTRING_CSTR(mdnsName));
			return false;
		}
#else
		return false;
#endif		
	}

	qDebug() << "Input mdnsName:" << mdnsName;
	qDebug() << "Input mdnsHostname:" << mdnsHostname;

	QString address{mdnsHostname};
	// 2. Resolve the mDNS-Hostname into an IP-address
	if (NetUtils::resolveMdnsHostToAddress(log, address))
	{
			Info(log, "Successfully resolved mDNS hostname [%s] into IP-address [%s]", QSTRING_CSTR(mdnsHostname), QSTRING_CSTR(address));
	}
	else
	{
		// If mDNS resolver failed, let the OS resolve it
		Debug(log, "Cannot resolve IP-address for given mDNS hostname [%s], let the operation system handle it!", QSTRING_CSTR(mdnsHostname));
	}

	mdnsName = address;

	return true;
}

inline bool convertMdnsToIp(Logger* log, QString& mdnsName)
{
	int ignoredPort {MAX_PORT};
	return convertMdnsToIp(log, mdnsName, ignoredPort);
}

///
/// @brief Resolve a hostname(DNS) or mDNS service- or hostname into an IP-address. A given IP-address will be passed through
/// @param[in/out] log         The logger of the caller to print
/// @param[in]     hostname    The hostname/mDNS service name to be resolved
/// @param[out]    hostAddress The resolved IP-Address
/// @param[in/out] port        The port provided by the mDNS service, if not mDNS the input port is returned
/// @return        True on success else false
///
inline bool resolveHostToAddress(Logger *log, const QString &hostname, QHostAddress &hostAddress, int &port)
{
	QString resolvedHost{hostname};

	if (!convertMdnsToIp(log, resolvedHost, port))
	{
		return false;
	}

	if (!hostAddress.setAddress(resolvedHost))
	{
		QHostInfo const hostInfo = QHostInfo::fromName(resolvedHost);
		if (hostInfo.error() != QHostInfo::NoError)
		{

			QString const errorText = QString("Failed resolving hostname [%1] into an IP-address. Error: [%2] %3")
										  .arg(hostname)
										  .arg(hostInfo.error())
										  .arg(hostInfo.errorString());
			Error(log, "%s", QSTRING_CSTR(errorText));
			return false;
		}
		hostAddress = hostInfo.addresses().at(0);
		Debug(log, "Successfully resolved hostname [%s] to IP-address [%s]", QSTRING_CSTR(hostname), QSTRING_CSTR(hostAddress.toString()));
	}
	return true;
}

inline bool resolveHostToAddress(Logger* log, const QString& hostname, QHostAddress& hostAddress)
{
	int ignoredPort {MAX_PORT};
	return resolveHostToAddress(log, hostname, hostAddress, ignoredPort);
}

}
