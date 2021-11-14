#pragma once

#include <utils/Logger.h>

#include <QTcpServer>
#include <QUrl>
#include <QHostAddress>
#include <QHostInfo>

namespace NetUtils {

	const int MAX_PORT = 65535;

	///
	/// @brief Check if the port is available for listening
	/// @param[in/out] port  The port to test, will be incremented if port is in use
	/// @param         log   The logger of the caller to print
	/// @return        True on success else false
	///
	static bool portAvailable(quint16& port, Logger* log)
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
	static bool isValidPort(Logger* log, int port, const QString& host)
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
	static bool resolveHostPort(const QString& address, QString& host, quint16& port)
	{
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
	/// @brief Check if the port is in the valid range
	/// @param     log   The logger of the caller to print
	/// @param[in] address  The port to be tested
	/// @param[out] hostAddress  A hostname to make reference to during logging 
	/// @return          True on success else false
	///

	static bool resolveHostAddress(Logger* log, const QString& address, QHostAddress& hostAddress)
	{
		bool isHostAddressOK{ false };


		if (hostAddress.setAddress(address))
		{
			Debug(log, "Successfully parsed %s as an IP-address.", QSTRING_CSTR(hostAddress.toString()));
			isHostAddressOK = true;
		}
		else
		{
			QHostInfo hostInfo = QHostInfo::fromName(address);
			if (hostInfo.error() == QHostInfo::NoError)
			{
				hostAddress = hostInfo.addresses().first();
				Debug(log, "Successfully resolved IP-address (%s) for hostname (%s).", QSTRING_CSTR(hostAddress.toString()), QSTRING_CSTR(address));
				isHostAddressOK = true;
			}
			else
			{
				QString errortext = QString("Failed resolving IP-address for [%1], (%2) %3").arg(address).arg(hostInfo.error()).arg(hostInfo.errorString());
				Error(log, "%s", QSTRING_CSTR(errortext));
				isHostAddressOK = false;
			}
		}
		return isHostAddressOK;
	}



}
