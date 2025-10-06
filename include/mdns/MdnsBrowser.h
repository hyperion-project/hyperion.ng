#ifndef MDNS_BROWSER_H
#define MDNS_BROWSER_H

#include <chrono>

#include <qmdnsengine/server.h>
#include <qmdnsengine/service.h>

#include <qmdnsengine/browser.h>
#include <qmdnsengine/cache.h>
#include <qmdnsengine/resolver.h>
#include <qmdnsengine/dns.h>
#include <qmdnsengine/record.h>

// Qt includes
#include <QObject>
#include <QByteArray>
#include <QMap>
#include <QJsonArray>
#include <QSharedPointer>
#include <QScopedPointer>
#include <QLoggingCategory>

// Utility includes
#include <utils/Logger.h>

Q_DECLARE_LOGGING_CATEGORY(mdns_browser)

namespace {
	constexpr std::chrono::milliseconds DEFAULT_DISCOVER_TIMEOUT{ 500 };
	constexpr std::chrono::milliseconds DEFAULT_ADDRESS_RESOLVE_TIMEOUT{ 1000 };
} // End of constants

class MdnsBrowser : public QObject
{
	Q_OBJECT

private:
	///
	/// @brief Browse for hyperion services in bonjour, constructed from HyperionDaemon
	///        Searching for hyperion http service by default
	///
	// Run MdnsBrowser as singleton
	explicit MdnsBrowser(QObject* parent = nullptr);
	MdnsBrowser(const MdnsBrowser&) = delete;
	MdnsBrowser(MdnsBrowser&&) = delete;
	MdnsBrowser& operator=(const MdnsBrowser&) = delete;
	MdnsBrowser& operator=(MdnsBrowser&&) = delete;

	static QSharedPointer<MdnsBrowser> instance;

public:
	 static QSharedPointer<MdnsBrowser>& getInstance(QThread* externalThread = nullptr);

	QMdnsEngine::Service getFirstService(const QByteArray& serviceType, const QString& filter = ".*", std::chrono::milliseconds waitTime = DEFAULT_DISCOVER_TIMEOUT) const;
	QJsonArray getServicesDiscoveredJson(const QByteArray& serviceType, const QString& filter = ".*", std::chrono::milliseconds waitTime = std::chrono::milliseconds{ 0 }) const;

	///
	/// @brief Check if the passed name is an MDNS service- or hostname
	/// @param[in]     mdnsName    The name to be checked
	/// @return        True on success else false
	///
	static inline bool isMdns(const QString &mdnsName)
	{
		return mdnsName.endsWith(".local") || mdnsName.endsWith(".local.");
	}

	///
	/// @brief Check if the passed name is an MDNS service name
	/// @param[in]     mdnsServiceName    The name to be checked
	/// @return        True on success else false
	///
	static inline bool isMdnsService(const QString &mdnsServiceName)
	{
		return mdnsServiceName.endsWith("._tcp.local") || mdnsServiceName.endsWith("._tcp.local.");
	}

	void printCache(const QByteArray &name = QByteArray(), quint16 type = QMdnsEngine::ANY) const;

public slots:

	void stop(); // Stop _server and _cache in the right thread

	///
	/// @brief Browse for a service of type
	///
	void browseForServiceType(const QByteArray& serviceType);

	void resolveServiceInstance(const QByteArray& serviceInstance, std::chrono::milliseconds waitTime = DEFAULT_DISCOVER_TIMEOUT) const;

	void resolveFirstAddress(Logger* log, const QString& hostname, std::chrono::milliseconds timeout = DEFAULT_ADDRESS_RESOLVE_TIMEOUT);

Q_SIGNALS:

	/**
	 * @brief Indicate that the specified service was updated
	 *
	 * This signal is emitted when the SRV record for a service (identified by
	 * its name and type) or a TXT record has changed.
	 */
	void serviceFound(const QMdnsEngine::Service& service);

	/**
	 * @brief Indicate that the specified service was removed
	 *
	 * This signal is emitted when an essential record (PTR or SRV) is
	 * expiring from the cache. This will also occur when an updated PTR or
	 * SRV record is received with a TTL of 0.
	 */
	void serviceRemoved(const QMdnsEngine::Service& service);

	void isAddressResolved(const QString& hostname, const QHostAddress& address);
	void isFirstAddressResolved(const QString& hostname, const QHostAddress& address);

	void isServiceRecordResolved(const QByteArray& serviceInstance, const QMdnsEngine::Record& serviceRecord) const;

private slots:

	void initMdns();

	void onServiceAdded(const QMdnsEngine::Service& service);
	void onServiceUpdated(const QMdnsEngine::Service& service);
	void onServiceRemoved(const QMdnsEngine::Service& service);

	void onHostNameResolved(const QString& hostname, const QHostAddress& address);

private:
	/// The logger instance for mDNS-Service
	Logger* _log;

	QScopedPointer<QMdnsEngine::Server, QScopedPointerDeleteLater> _server;
	QScopedPointer<QMdnsEngine::Cache, QScopedPointerDeleteLater> _cache;

	QMap<QByteArray, QSharedPointer<QMdnsEngine::Browser>> _browsedServiceTypes;
};

#endif // MDNSBROWSER_H
