#pragma once

#include <QTcpServer>
#include <QUrl>
#include <QHostAddress>
#include <QHostInfo>

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
			Error(log, "Invalid port [%d] for host: (%s)! - Port must be in range [0 - %d]", port, QSTRING_CSTR(host), MAX_PORT);
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

		QUrl url(testUrl);
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
	inline bool resolveMdDnsHostToAddress(Logger* log, const QString& hostname, QHostAddress& hostAddress)
	{
		bool isHostAddressOK{ false };
		if (!hostname.isEmpty())
		{
			#ifdef ENABLE_MDNS
			if (hostname.endsWith(".local") || hostname.endsWith(".local."))
			{
				QHostAddress resolvedAddress;
				QMetaObject::invokeMethod(&MdnsBrowser::getInstance(), "resolveAddress",
										   Qt::BlockingQueuedConnection,
										   Q_RETURN_ARG(bool, isHostAddressOK),
										   Q_ARG(Logger*, log), Q_ARG(QString, hostname), Q_ARG(QHostAddress&, resolvedAddress));
				hostAddress = resolvedAddress;
			}
			else
			#endif
			{
				if (hostAddress.setAddress(hostname))
				{
					//Debug(log, "IP-address (%s) not required to be resolved.", QSTRING_CSTR(hostAddress.toString()));
					isHostAddressOK = true;
				}
				else
				{
					QHostInfo hostInfo = QHostInfo::fromName(hostname);
					if (hostInfo.error() == QHostInfo::NoError)
					{
						hostAddress = hostInfo.addresses().at(0);
						Debug(log, "Successfully resolved hostname (%s) to IP-address (%s)", QSTRING_CSTR(hostname), QSTRING_CSTR(hostAddress.toString()));
						isHostAddressOK = true;
					}
					else
					{
						QString errortext = QString("Failed resolving hostname (%1) to IP-address. Error: (%2) %3").arg(hostname).arg(hostInfo.error()).arg(hostInfo.errorString());
						Error(log, "%s", QSTRING_CSTR(errortext));
						isHostAddressOK = false;
					}
				}
			}
		}
		return isHostAddressOK;
	}

	///
	/// @brief Resolve a hostname(DNS) or mDNS service name into an IP-address. A given IP address will be passed through
	/// @param[in/out] log         The logger of the caller to print
	/// @param[in]     hostname    The hostname/mDNS service name to be resolved
	/// @param[out]    hostAddress The resolved IP-Address
	/// @param[in/out] port        The port provided by the mDNS service, if not mDNS the input port is returned
	/// @return        True on success else false
	///
	inline bool resolveHostToAddress(Logger* log, const QString& hostname, QHostAddress& hostAddress, int& port)
	{
		bool areHostAddressPartOK{ false };
		QString target {hostname};

		#ifdef ENABLE_MDNS
		if (hostname.endsWith("._tcp.local"))
		{
			//Treat hostname as service instance name that requires to be resolved into an mDNS-Hostname first
			QMdnsEngine::Record service = MdnsBrowser::getInstance().getServiceInstanceRecord(hostname.toUtf8());
			if (!service.target().isEmpty())
			{
				Info(log, "Resolved service [%s] to mDNS hostname [%s], service port [%d]", QSTRING_CSTR(hostname), service.target().constData(), service.port());
				target = service.target();
				port = service.port();
			}
			else
			{
				Error(log, "Cannot resolve mDNS hostname for given service [%s]!", QSTRING_CSTR(hostname));
				return areHostAddressPartOK;
			}
		}
		#endif

		QHostAddress resolvedAddress;
		if (NetUtils::resolveMdDnsHostToAddress(log, target, resolvedAddress))
		{
			hostAddress = resolvedAddress;
			if (hostname != hostAddress.toString())
			{
				Info(log, "Resolved hostname (%s) to IP-address (%s)",  QSTRING_CSTR(hostname), QSTRING_CSTR(hostAddress.toString()));
			}

			if (NetUtils::isValidPort(log, port, hostAddress.toString()))
			{
				areHostAddressPartOK = true;
			}
		}
		return areHostAddressPartOK;
	}

	inline bool resolveHostToAddress(Logger* log, const QString& hostname, QHostAddress& hostAddress)
	{
		int ignoredPort {MAX_PORT};
		return resolveHostToAddress(log, hostname, hostAddress, ignoredPort);
	}

	}
