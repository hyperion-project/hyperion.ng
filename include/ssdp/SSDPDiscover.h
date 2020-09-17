#ifndef SSDPDISCOVER_H
#define SSDPDISCOVER_H

#include <QHostAddress>
#include <QMultiMap>
#include <QUrl>
#include <QRegularExpression>

#include <chrono>

class Logger;
class QUdpSocket;

enum class searchType{
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

// Default values
static const char	DEFAULT_SEARCH_ADDRESS[] = "239.255.255.250";
static const int	DEFAULT_SEARCH_PORT = 1900;
static const char	DEFAULT_FILTER[] = ".*";
static const char	DEFAULT_FILTER_HEADER[] = "ST";

constexpr std::chrono::milliseconds DEFAULT_SSDP_TIMEOUT{5000}; // timeout in ms

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
	QString getFirstService(const searchType &type = searchType::STY_WEBSERVER,const QString &st = "urn:hyperion-project.org:device:basic:1", int timeout_ms = 3000);

	///
	/// @brief Discover services via ssdp.
	///
	/// Records meeting the search target and filter criteria ( setSearchFilter() ) are stored in a map using the given element as a key.
	///
	/// The search result can be accessed via getServicesDiscoveredJson() or getServicesDiscovered()
	///
	/// Usage sample:
	/// @code
	///
	/// SSDPDiscover discover;
	///
	/// discover.skipDuplicateKeys(true);
	/// QString searchTargetFilter = "(.*)IpBridge(.*)";
	/// discover.setSearchFilter(searchTargetFilter, "SERVER");
	/// QString searchTarget = "upnp:rootdevice";
	///
	/// if ( discover.discoverServices(searchTarget) > 0 )
	/// 	deviceList = discover.getServicesDiscoveredJson();
	///
	///@endcode
	///
	/// @param[in] searchTarget The ssdp discovery search target (ST)
	/// @param[in] key Element used as key for the result map
	///
	/// @return Number of service records found (meeting the search & filter criteria)
	///
	int discoverServices(const QString &searchTarget="ssdp:all", const QString &key="LOCATION");

	///
	/// @brief Get services discovered during discoverServices().
	///
	/// Hostname and domain are resolved from IP-address and stored in extra elements
	///
	/// Sample result:
	/// @code
	///
	/// [{
	/// "cache-control": "max-age=100",
	/// "domain": "fritz.box",
	/// "hostname": "ubuntu1910",
	/// "id": "http://192.168.2.152:8081/description.xml",
	/// "ip": "192.168.2.152",
	/// "location": "http://192.168.2.152:8081/description.xml",
	/// "other": { "ext": "", "host": "239.255.255.250:1900", "hue-bridgeid": "000C29FFFED8D52D"},
	/// "port": 8081,
	/// "server": "Linux/3.14.0 UPnP/1.0 IpBridge/1.19.0",
	/// "st": "upnp:rootdevice",
	/// "usn": "uuid:2f402f80-da50-11e1-9b23-000c29d8d52d::upnp:rootdevice"
	/// }]
	///
	///@endcode
	///
	/// @return Discovered services as JSON-document
	///
	QJsonArray getServicesDiscoveredJson() const;

	///
	/// @brief Set the ssdp discovery address (HOST)
	///
	/// @param[in] IP-address used during discovery
	///
	void setAddress ( const QString &address) { _ssdpAddr = QHostAddress(address); }

	///
	/// @brief Set the ssdp discovery port (HOST)
	///
	/// @param[in] port used during discovery
	///
	void setPort ( quint16 port) { _ssdpPort = port; }

	///
	/// @brief Set the ssdp discovery max wait time (MX)
	///
	/// @param[in] maxWaitResponseTime
	///
	void setMaxWaitResponseTime ( int maxWaitResponseTime) { _ssdpMaxWaitResponseTime = maxWaitResponseTime; }

	///
	/// @brief Set the ssdp discovery search target (ST)
	///
	/// @param[in] searchTarget
	///
	void setSearchTarget ( const QString &searchTarget) { _searchTarget = searchTarget; }

	///
	/// @brief Set the ssdp discovery search target filter
	///
	/// @param[in] filter as regular expression
	/// @param[in] filterHeader Header element the filter is applied to
	///
	/// @return True, if valid regular expression
	///
	bool setSearchFilter ( const QString &filter=DEFAULT_FILTER, const QString &filterHeader="ST");

	///
	/// @brief Set the ssdp discovery search target and filter to default values
	///
	void clearSearchFilter () { _filter=DEFAULT_FILTER; _filterHeader="ST"; }

	///
	/// @brief Skip duplicate records with the same key-value
	///
	/// @param[in] skip True: skip records with duplicate key-values, False: Allow duplicate key-values
	///
	void skipDuplicateKeys( bool skip ) { _skipDupKeys = skip; }

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
	QHostAddress _ssdpAddr;
	quint16 _ssdpPort;

	int _ssdpMaxWaitResponseTime;
	int	_ssdpTimeout;

	QMultiMap<QString, SSDPService> _services;

	QStringList _usnList;
	QString _searchTarget;

	QString _filter;
	QString _filterHeader;
	QRegularExpression _regExFilter;
	bool _skipDupKeys;
};

#endif // SSDPDISCOVER_H
