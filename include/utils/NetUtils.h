#pragma once

#include <QTcpServer>
#include <QUrl>
#include <QHostAddress>
#include <QHostInfo>
#include <QThread>
#include <QEventLoop>
#include <QTimer>
#include <utility>

#include <HyperionConfig.h>
#include <utils/Logger.h>

#ifdef ENABLE_MDNS
#include <mdns/MdnsBrowser.h>
#endif

namespace NetUtils {

const int MAX_PORT = 65535;

///
/// @brief Check if the port is available for listening
/// @param[in/out] port  The port to test, will be incremented if port is in use
/// @param         log   The logger of the caller to print
/// @return        True on success else false
///
inline bool portAvailable(quint16& port, QSharedPointer<Logger> log)
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
inline bool isValidPort(QSharedPointer<Logger> log, int port, const QString& host)
{
	if ((port <= 0 || port > MAX_PORT) && port != -1)
	{
		Error(log, "Invalid port [%d] for host: (%s)! - Port must be in range [1 - %d]", port, QSTRING_CSTR(host), MAX_PORT);
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
/// @brief Resolve a hostname (DNS/mDNS) into an IP-address. A given IP address will be passed through
/// @param[in/out] log         The logger of the caller to print
/// @param[in]     hostname    The hostname to be resolved
/// @param[out]    hostAddress The resolved IP-Address
/// @return        True on success else false
///
inline bool resolveMDnsHostToAddress(QSharedPointer<Logger> log, const QString& hostname, QHostAddress& hostAddress)
{
	bool isHostAddressOK{ false };
	if (!hostname.isEmpty())
	{
#ifdef ENABLE_MDNS
		if (hostname.endsWith(".local") || hostname.endsWith(".local."))
		{
			MdnsBrowser* browser = MdnsBrowser::getInstance().get();
			QEventLoop loop;

			// Connect the signal to capture the resolved address
			QString const requestedHostname = hostname;
			QObject::connect(browser, &MdnsBrowser::isFirstAddressResolved, &loop,
							 [&, requestedHostname](const QString& resolvedHostname, const QHostAddress& addr) {
				if (resolvedHostname.compare(requestedHostname, Qt::CaseInsensitive) != 0)
				{
					return;
				}

				hostAddress = addr;
				loop.quit();
			});

			// Call the function asynchronously in MdnsBrowser's thread
			QMetaObject::invokeMethod(browser, "resolveFirstAddress",
									  Qt::QueuedConnection,  // Ensures it runs in the correct thread
									  Q_ARG(QSharedPointer<Logger>, log),
									  Q_ARG(QString, hostname)
									  );

			// Wait for the result
			loop.exec();

			if (!hostAddress.isNull())
			{
				isHostAddressOK = true;
			}
		}
		else
#endif
		{
			// If it's not an mDNS hostname, resolve it normally (synchronously)
			if (hostAddress.setAddress(hostname))
			{
				// An IP-address is not required to be resolved
				isHostAddressOK = true;
			}
			else
			{
				QHostInfo const hostInfo = QHostInfo::fromName(hostname);
				if (hostInfo.error() == QHostInfo::NoError) {
					hostAddress = hostInfo.addresses().at(0);
					Debug(log, "Successfully resolved hostname (%s) to IP-address (%s)", QSTRING_CSTR(hostname), QSTRING_CSTR(hostAddress.toString()));
					isHostAddressOK = true;
				} else {
					QString const errorText = QString("Failed resolving hostname (%1) to IP-address. Error: (%2) %3")
											  .arg(hostname)
											  .arg(hostInfo.error())
											  .arg(hostInfo.errorString());
					Error(log, "%s", QSTRING_CSTR(errorText));
					isHostAddressOK = false;
				}
			}

		}
	}
	return isHostAddressOK;
}

///
/// @brief Resolve a mDNS service name into a nmDNS serice record including mDNS hostname & port for the given service instance
/// @param[in/out] log             The logger of the caller to print
/// @param[in]     serviceInstance The service instance to be resolved
/// @return        A service record
///
#ifdef ENABLE_MDNS
inline QMdnsEngine::Record resolveMDnsServiceRecord(const QByteArray& serviceInstance)
{
	QMdnsEngine::Record serviceRecord;


	if (serviceInstance.endsWith("._tcp.local"))
	{
		qRegisterMetaType<QMdnsEngine::Record>("QMdnsEngine::Record");

		MdnsBrowser* browser = MdnsBrowser::getInstance().get();
		QEventLoop loop;

		// Connect the signal to capture the resolved service record
		QObject::connect(browser, &MdnsBrowser::isServiceRecordResolved, &loop, [&](QMdnsEngine::Record resolvedServiceRecord) {
			serviceRecord = resolvedServiceRecord;
			loop.quit();
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
/// @brief Resolve a hostname(DNS) or mDNS service name into an IP-address. A given IP address will be passed through
/// @param[in/out] log         The logger of the caller to print
/// @param[in]     hostname    The hostname/mDNS service name to be resolved
/// @param[out]    hostAddress The resolved IP-Address
/// @param[in/out] port        The port provided by the mDNS service, if not mDNS the input port is returned
/// @return        True on success else false
///
inline bool resolveHostToAddress(QSharedPointer<Logger> log, const QString& hostname, QHostAddress& hostAddress, int& port)
{
	bool areHostAddressPartOK{ false };
	QString target{ hostname };

#ifdef ENABLE_MDNS
	if (hostname.endsWith("._tcp.local"))
	{
		//Treat hostname as service instance name that requires to be resolved into an mDNS-Hostname first
		QMdnsEngine::Record const service = resolveMDnsServiceRecord(hostname.toUtf8());
		if (!service.target().isEmpty())
		{
			if (!service.target().isEmpty())
			{
				Info(log, "Resolved service [%s] to mDNS hostname [%s], service port [%d]", QSTRING_CSTR(hostname), service.target().constData(), service.port());
				target = service.target();
				port = service.port();
			}
			else
			{
				Error(log, "Failed to resolved service [%s] to an mDNS hostname", QSTRING_CSTR(hostname));
				return false;
			}
		}
		else
		{
			Error(log, "Cannot resolve mDNS hostname for given service [%s]!", QSTRING_CSTR(hostname));
			return false;
		}

		QHostAddress resolvedAddress;
		if (NetUtils::resolveMDnsHostToAddress(log, target, resolvedAddress))
		{
			hostAddress = resolvedAddress;
			if (hostname != hostAddress.toString())
			{
				Info(log, "Resolved hostname (%s) to IP-address (%s)", QSTRING_CSTR(hostname), QSTRING_CSTR(hostAddress.toString()));
			}

			if (NetUtils::isValidPort(log, port, hostAddress.toString()))
			{
				areHostAddressPartOK = true;
			}
		}
	}
	else
#endif
	{
		return false;

	}
	return areHostAddressPartOK;
}

///
/// @brief Resolve a hostname(DNS) or mDNS service name into an IP-address. A given IP address will be passed through
/// @param[in/out] log         The logger of the caller to print
/// @param[in/out] hostname    The hostname/mDNS service name to be resolved, if mDNS the input hostname is replaced with the resolved mDNS hostname
/// @param[in/out] port        The port provided by the mDNS service, if not mDNS the input port is returned
/// @return        True on success else false
///
inline bool resolveMdnsHost(QSharedPointer<Logger> log, QString& hostname, int& port)
{
	bool isResolved{ true };

#ifdef ENABLE_MDNS
	QString target{ hostname };

	if (hostname.endsWith("._tcp.local"))
	{
		//Treat hostname as service instance name that requires to be resolved into an mDNS-Hostname first
		QMdnsEngine::Record const service = resolveMDnsServiceRecord(hostname.toUtf8());
		if (!service.target().isEmpty())
		{
			if (!service.target().isEmpty())
			{
				Info(log, "Resolved service [%s] to mDNS hostname [%s], service port [%d]", QSTRING_CSTR(hostname), service.target().constData(), service.port());
				target = service.target();
				port = service.port();
			}
			else
			{
				Error(log, "Failed to resolved service [%s] to an mDNS hostname", QSTRING_CSTR(hostname));
				return false;
			}
		}
		else
		{
			Error(log, "Cannot resolve mDNS hostname for given service [%s]!", QSTRING_CSTR(hostname));
			return false;
		}

		QHostAddress resolvedAddress;
		if (NetUtils::resolveMDnsHostToAddress(log, target, resolvedAddress))
		{
			QString const resolvedHostname = resolvedAddress.toString();
			if (hostname != resolvedHostname)
			{
				hostname = resolvedHostname;
				Info(log, "Resolved hostname (%s) to IP-address (%s)", QSTRING_CSTR(hostname), QSTRING_CSTR(hostname));

			}

			if (!NetUtils::isValidPort(log, port, hostname))
			{
				isResolved = false;
			}
		}
	}
#endif
	return isResolved;
}

inline bool resolveHostToAddress(QSharedPointer<Logger> log, const QString& hostname, QHostAddress& hostAddress)
{
	int ignoredPort {MAX_PORT};
	return resolveHostToAddress(log, hostname, hostAddress, ignoredPort);
}

}
