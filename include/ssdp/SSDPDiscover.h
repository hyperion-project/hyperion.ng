#pragma once

#include <utils/Logger.h>
#include <QHostAddress>

class QUdpSocket;

enum class searchType{
	STY_WEBSERVER,
	STY_FLATBUFSERVER,
	STY_JSONSERVER
};

///
/// @brief Search for SSDP sessions, used by stand-alone capture binaries
///
class SSDPDiscover : public QObject
{
	Q_OBJECT

public:

	SSDPDiscover(QObject* parent = nullptr);

	///
	/// @brief Search for specified service, results will be returned by signal newService(). Calling this method again will reset all found urns and search again
	/// @param st The service to search for
	///
	void searchForService(const QString &st = "urn:hyperion-project.org:device:basic:1");

	///
	/// @brief Search for specified searchTarget, the method will block until a server has been found or a timeout happened
	/// @param type        The address type one of struct searchType
	/// @param st          The service to search for
	/// @param timeout_ms  The timeout in ms
	/// @return The address+port of web-server or empty if timed out
	///
	const QString getFirstService(const searchType &type = searchType::STY_WEBSERVER,const QString &st = "urn:hyperion-project.org:device:basic:1", const int &timeout_ms = 3000);

signals:
	///
	/// @brief Emits whenever a new service has been found, search started with searchForService()
	///
	/// @param webServer The address+port of web-server "192.168.0.10:8090"
	///
	void newService(const QString &webServer);

private slots:
	void readPendingDatagrams();

private:
	///
	/// @brief Execute ssdp discovery request
	///
	/// @param[in] st Search Target
	///
	void sendSearch(const QString &st);

private:

	Logger* _log;
	QUdpSocket* _udpSocket;
	QString _searchTarget;
	QStringList _usnList;
};
