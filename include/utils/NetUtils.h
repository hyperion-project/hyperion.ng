#pragma once

#include <QTcpServer>
#include <QUrl>
#include <QHostAddress>
#include <QHostInfo>

#include <HyperionConfig.h>
#include <utils/Logger.h>

#ifdef ENABLE_MDNS
#include <mdns/mdnsBrowser.h>
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
			Warning(log, "The requested Port '%d' was already in use, will use Port '%d' instead", prevPort, port);
			return false;
		}
		return true;
	}

	///
	/// @brief Check if the port is in the valid range
	/// @param     log   The logger of the caller to print/// 
	/// @param[in] port  The port to be tested
	/// @param[in] host  A hostname/IP-address to make reference to during logging
	/// @return          True on success else false
	///
	inline bool isValidPort(Logger* log, int port, const QString& host)
	{
		if (port <= 0 || port > MAX_PORT)
		{
			Error(log, "Invalid port [%d] for host: %s!", port, QSTRING_CSTR(host));
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
	inline bool resolveHostPort(const QString& address, QString& host, quint16& port)
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
	/// @param[in]     context     The QObject context of the call
	/// @param[in/out] log         The logger of the caller to print
	/// @param[in]     hostname    The hostname to be resolved
	/// @param[in/out] hostAddress A hostname to make reference to during logging
	/// @return        True on success else false
	///
	inline bool resolveHostAddress(const QObject* context, Logger* log, const QString& hostname, QHostAddress& hostAddress)
	{
		bool isHostAddressOK{ false };

		if (!hostname.isEmpty())
		{
			#ifdef ENABLE_MDNS
			if (hostname.endsWith(".local"))
			{
				isHostAddressOK = MdnsBrowser::getInstance().resolveAddress(context, log, hostname, hostAddress);
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
						hostAddress = hostInfo.addresses().first();
						Debug(log, "Successfully resolved IP-address (%s) for hostname (%s).", QSTRING_CSTR(hostAddress.toString()), QSTRING_CSTR(hostname));
						isHostAddressOK = true;
					}
					else
					{
						QString errortext = QString("Failed resolving IP-address for [%1], (%2) %3").arg(hostname).arg(hostInfo.error()).arg(hostInfo.errorString());
						Error(log, "%s", QSTRING_CSTR(errortext));
						isHostAddressOK = false;
					}
				}
			}
		}
		return isHostAddressOK;
	}
}
