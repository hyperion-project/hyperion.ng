#ifndef MDNS_BROWSER_H
#define MDNS_BROWSER_H

#include <chrono>
#include <type_traits>

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

// Utility includes
#include <utils/Logger.h>
#include <utils/WeakConnect.h>

namespace {
	constexpr std::chrono::milliseconds DEFAULT_DISCOVER_TIMEOUT{ 500 };
	constexpr std::chrono::milliseconds DEFAULT_ADDRESS_RESOLVE_TIMEOUT{ 1000 };

} //End of constants

class MdnsBrowser : public QObject
{
	Q_OBJECT

		// Run MdnsBrowser as singleton

private:
	///
	/// @brief Browse for hyperion services in bonjour, constructed from HyperionDaemon
	///        Searching for hyperion http service by default
	///
	// Run MdnsBrowser as singleton
	MdnsBrowser(QObject* parent = nullptr);
	~MdnsBrowser() override;

public:

	static MdnsBrowser& getInstance()
	{
		static MdnsBrowser* instance = new MdnsBrowser();
		return *instance;
	}

	MdnsBrowser(const MdnsBrowser&) = delete;
	MdnsBrowser(MdnsBrowser&&) = delete;
	MdnsBrowser& operator=(const MdnsBrowser&) = delete;
	MdnsBrowser& operator=(MdnsBrowser&&) = delete;

	QMdnsEngine::Service getFirstService(const QByteArray& serviceType, const QString& filter = ".*", const std::chrono::milliseconds waitTime = DEFAULT_DISCOVER_TIMEOUT) const;
	QJsonArray getServicesDiscoveredJson(const QByteArray& serviceType, const QString& filter = ".*", const std::chrono::milliseconds waitTime = std::chrono::milliseconds{ 0 }) const;


	void printCache(const QByteArray& name = QByteArray(), quint16 type = QMdnsEngine::ANY) const;

public slots:

	///
	/// @brief Browse for a service of type
	///
	void browseForServiceType(const QByteArray& serviceType);

	QHostAddress getHostFirstAddress(const QByteArray& hostname);

	void onHostNameResolved(const QHostAddress& address);

	QMdnsEngine::Record getServiceInstanceRecord(const QByteArray& serviceInstance, const std::chrono::milliseconds waitTime = DEFAULT_DISCOVER_TIMEOUT) const;

	bool resolveAddress(Logger* log, const QString& hostname, QHostAddress& hostAddress, std::chrono::milliseconds timeout = DEFAULT_ADDRESS_RESOLVE_TIMEOUT);

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

	void addressResolved(const QHostAddress address);

private slots:

	void onServiceAdded(const QMdnsEngine::Service& service);
	void onServiceUpdated(const QMdnsEngine::Service& service);
	void onServiceRemoved(const QMdnsEngine::Service& service);

private:
	/// The logger instance for mDNS-Service
	Logger* _log;

	QMdnsEngine::Server _server;
	QMdnsEngine::Cache  _cache;

	QMap<QByteArray, QMdnsEngine::Browser*> _browsedServiceTypes;
};

#endif // MDNSBROWSER_H
