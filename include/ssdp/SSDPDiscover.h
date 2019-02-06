#pragma once

#include <utils/Logger.h>
#include <QHostAddress>

class QUdpSocket;

enum searchType{
	STY_WEBSERVER,
	STY_FLATBUFSERVER
};

///
/// @brief Search for SSDP sessions, used by standalone capture binaries
///
class SSDPDiscover : public QObject
{
	Q_OBJECT

public:
	SSDPDiscover(QObject* parent = nullptr);

	///
	/// @brief Search for specified service, results will be returned by signal newService(). Calling this method again will reset all found usns and search again
	/// @param st The service to search for
	///
	void searchForService(const QString& st = "urn:hyperion-project.org:device:basic:1");

	///
	/// @brief Search for specified searchTarget, the method will block until a server has been found or a timeout happend
	/// @param type        The address type one of struct searchType
	/// @param st          The service to search for
	/// @param timeout_ms  The timeout in ms
	/// @return The address+port of webserver or empty if timed out
	///
	const QString getFirstService(const searchType& type = STY_WEBSERVER,const QString& st = "urn:hyperion-project.org:device:basic:1", const int& timeout_ms = 3000);

signals:
	///
	/// @brief Emits whenever a new service has ben found, search started with searchForService()
	/// @param webServer The address+port of webserver "192.168.0.10:8090"
	///
	void newService(const QString webServer);

private slots:
	void readPendingDatagrams();

private:
	void sendSearch(const QString& st);

private:
	Logger* _log;
	QUdpSocket* _udpSocket;
	QString _searchTarget;
	QStringList _usnList;
};
