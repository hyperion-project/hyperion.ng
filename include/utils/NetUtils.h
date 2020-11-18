#pragma once

#include <utils/Logger.h>

#include <QTcpServer>

namespace NetUtils {
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

}
