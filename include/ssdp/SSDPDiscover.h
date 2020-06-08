#pragma once

#include <utils/Logger.h>
#include <QHostAddress>
#include <QUrl>
#include <QRegularExpression>

class QUdpSocket;

enum searchType{
	STY_WEBSERVER,
	STY_FLATBUFSERVER,
	STY_JSONSERVER
};

struct SSDPService {
	QString cacheControl;
	QUrl location;
	QString server;
	QString searchTarget;
	QString uniqueServiceName;
	QMap <QString,QString> otherHeaders;
};

static const char	DEFAULT_SEARCH_ADDRESS[] = "239.255.255.250";
static const int	DEFAULT_SEARCH_PORT = 1900;
static const char	DEFAULT_FILTER[] = ".*";
static const char	DEFAULT_FILTER_HEADER[] = "ST";

const int DEFAULT_SSDP_TIMEOUT = 5000; // timout in ms

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

	int discoverServices(const QString& searchTarget="ssdp:all", const QString& key="LOCATION");
	const QMap<QString, SSDPService> getServicesDiscovered () { return _services; }
	QJsonArray getServicesDiscoveredJson();

	void setAddress ( const QString& address) { _ssdpAddr = address; }
	void setPort ( quint16 port) { _ssdpPort = port; }
	void setMaxWaitResponseTime ( int maxWaitResponseTime) { _ssdpMaxWaitResponseTime = maxWaitResponseTime; }

	void setSearchTarget ( const QString& searchTarget) { _searchTarget = searchTarget; }
	bool setSearchFilter ( const QString& filter=DEFAULT_FILTER, const QString& filterHeader="ST");
	void clearSearchFilter () { _filter=DEFAULT_FILTER; _filterHeader="ST"; }
	void skipDuplicateKeys( bool skip ) { _skipDupKeys = skip; }

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
	QHostAddress _ssdpAddr;
	quint16 _ssdpPort;

	int _ssdpMaxWaitResponseTime;
	int	_ssdpTimeout;

	QMap<QString, SSDPService> _services;

	QStringList _usnList;
	QString _searchTarget;

	QString _filter;
	QString _filterHeader;
	QRegularExpression _regExFilter;
	bool _skipDupKeys;
};
