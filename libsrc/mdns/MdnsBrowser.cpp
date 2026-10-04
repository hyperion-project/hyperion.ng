#include <mdns/MdnsBrowser.h>
#include <qmdnsengine/message.h>
#include <qmdnsengine/service.h>

// Qt includes
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QHostAddress>
#include <QRegularExpression>

// Utility includes
#include <HyperionConfig.h>
#include <utils/Logger.h>
#include <utils/WaitTime.h>
#include <utils/NetUtils.h>
#include <utils/JsonUtils.h>
#include <utils/MemoryTracker.h>

Q_LOGGING_CATEGORY(mdns_browser, "hyperion.mdns.browser")
Q_LOGGING_CATEGORY(mdns_browser_cache, "hyperion.mdns.browser.cache")

namespace {
const int SERVICE_LOOKUP_RETRIES = 5;
} // End of constants

QSharedPointer<MdnsBrowser> MdnsBrowser::instance;

MdnsBrowser::MdnsBrowser(QObject* parent)
	: QObject(parent)
	, _log(Logger::getInstance("MDNS"))
	, _server(nullptr)
	, _cache(nullptr)
{
	TRACK_SCOPE();
	qRegisterMetaType<QMdnsEngine::Message>("QMdnsEngine::Message");
	qRegisterMetaType<QHostAddress>("QHostAddress");
}

MdnsBrowser::~MdnsBrowser()
{
	TRACK_SCOPE();
}

void MdnsBrowser::initMdns()
{
	if (!_server) // Ensure it only initializes once
	{
		_server.reset(new QMdnsEngine::Server());
		_cache.reset(new QMdnsEngine::Cache());
	}
}

void MdnsBrowser::stop()
{
	qCDebug(mdns_browser) << "Stopping MdnsBrowser";
	_browsedServiceTypes.clear();
	_cache.reset();
	_server.reset();

	Info(_log, "mDNS-Browser stopped");

	emit isStopped();

	qCDebug(mdns_browser) << "MdnsBrowser stopped";
}

QSharedPointer<MdnsBrowser>& MdnsBrowser::getInstance(QThread* externalThread)
{
	if (instance.isNull())
	{
		CREATE_INSTANCE_WITH_TRACKING(instance, MdnsBrowser, nullptr, nullptr);
		if (externalThread != nullptr) // Move to existing thread if provided
		{
			instance->moveToThread(externalThread);
			qCDebug(mdns_browser) << "MdnsBrowser running in thread: " << externalThread->objectName();

			// Ensure _server and _cache are initialized inside externalThread
			QMetaObject::invokeMethod(instance.get(), "initMdns", Qt::QueuedConnection);
		}
		else
		{
			instance->initMdns(); // Run in the same thread
		}
	}

	return instance;
}

void MdnsBrowser::destroyInstance()
{
	if (!instance.isNull())
	{
		qCDebug(mdns_browser) << "Destroying MdnsBrowser";
		instance.clear();
	}
}

void MdnsBrowser::browseForServiceType(const QByteArray& serviceType)
{
	if (!_browsedServiceTypes.contains(serviceType))
	{
		qCDebug(mdns_browser) << "Start new mDNS browser for serviceType:" << serviceType;
		QSharedPointer<QMdnsEngine::Browser> const newBrowser = QSharedPointer<QMdnsEngine::Browser>::create(_server.get(), serviceType, _cache.get());

		QObject::connect(newBrowser.get(), &QMdnsEngine::Browser::serviceAdded, this, &MdnsBrowser::onServiceAdded);
		QObject::connect(newBrowser.get(), &QMdnsEngine::Browser::serviceUpdated, this, &MdnsBrowser::onServiceUpdated);
		QObject::connect(newBrowser.get(), &QMdnsEngine::Browser::serviceRemoved, this, &MdnsBrowser::onServiceRemoved);

		_browsedServiceTypes.insert(serviceType, newBrowser);
	}
	else
	{
		qCDebug(mdns_browser) << "Use existing mDNS browser for serviceType:" << serviceType;
	}
}

void MdnsBrowser::onServiceAdded(const QMdnsEngine::Service& service)
{
	qCDebug(mdns_browser) << "Discovered service:" << service.name() << "at host:" << service.hostname() << ", port:" << service.port();
	emit serviceFound(service);
}

void MdnsBrowser::onServiceUpdated(const QMdnsEngine::Service& service) const
{
	qCDebug(mdns_browser) << "Service:" << service.type() << "was updated, name:" << service.name() << ", port:" << service.port();
}

void MdnsBrowser::onServiceRemoved(const QMdnsEngine::Service& service)
{
	qCDebug(mdns_browser) << "Service:" << service.type() << "was removed, name:" << service.name() << ", port:" << service.port();
	emit serviceRemoved(service);
}

void MdnsBrowser::onHostNameResolved(QString hostname, QHostAddress address) const
{
	qCDebug(mdns_browser) << "Resolved mDNS hostname:" << hostname << "to IP-address:" << address;
}

void MdnsBrowser::resolveFirstAddress(QSharedPointer<Logger> log, const QString& hostname, const std::chrono::milliseconds timeout)
{
	qCDebug(mdns_browser) << "Resolve first address for hostname:" << hostname << "with timeout of:" << timeout.count() << "ms";
	resolveFirstAddress(log, hostname, QAbstractSocket::AnyIPProtocol, timeout);
}

void MdnsBrowser::resolveFirstAddress(QSharedPointer<Logger> log, const QString& hostname, QAbstractSocket::NetworkLayerProtocol protocol)
{
	qCDebug(mdns_browser) << "Resolve first address for hostname:" << hostname << "with protocol:" << protocol;
	resolveFirstAddress(log, hostname, protocol, DEFAULT_ADDRESS_RESOLVE_TIMEOUT);
}

void MdnsBrowser::resolveFirstAddress(QSharedPointer<Logger> log, const QString& hostname, QAbstractSocket::NetworkLayerProtocol protocol, const std::chrono::milliseconds timeout)
{
	qRegisterMetaType<QMdnsEngine::Message>("Message");

	QHostAddress resolvedAddress;

	qCDebug(mdns_browser) << "Resolve first address for hostname:" << hostname << "with protocol:" << protocol << "and timeout of:" << timeout.count() << "ms";

	if (!isMdns(hostname))
	{
		qCWarning(mdns_browser) << "Hostname:" << hostname << "is not an mDNS hostname.";
		emit isFirstAddressResolved(hostname, resolvedAddress);
		return;
	}

	QByteArray hostLookupName = hostname.toUtf8();

	if (!hostLookupName.endsWith('.'))
	{
		hostLookupName.append('.');
	}

	QMdnsEngine::Resolver const resolver(_server.get(), hostLookupName, _cache.get());
	qCDebug(mdns_browser) << "Wait for resolver on mDNS hostname:" << hostname;

	QEventLoop loop;
	QTimer timer;

	timer.setSingleShot(true);
	connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

	connect(&resolver, &QMdnsEngine::Resolver::resolved, &loop, [hostname, protocol, &loop, &resolvedAddress](const QHostAddress &address)
	{
		// Ignore link-local addresses
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
		if (address.isLinkLocal())
#else
		if (address.toString().startsWith("fe80"))
#endif
		{
			return;
		}

		if (protocol == QAbstractSocket::AnyIPProtocol || address.protocol() == protocol)
		{
			qCDebug(mdns_browser) << "Resolved mDNS hostname:" << hostname << "matches the protocol:" << protocol << ", resolved IP-address:" << address;
			resolvedAddress = address;
			loop.quit();
		}
		else
		{
			qCDebug(mdns_browser) << "Ignoring address" << address << "for" << hostname << "- protocol mismatch. Requested:" << protocol << "Got:" << address.protocol();
		} 
	});

	timer.start(timeout);
	loop.exec();

	if (resolvedAddress.isNull())
	{
		Error(log, "Failed to resolve mDNS hostname: \"%s\"", QSTRING_CSTR(hostname));
	}
	else
	{
		qCDebug(mdns_browser) << "Resolved mDNS hostname:" << hostname << "to IP-address:" << resolvedAddress;
	}

	emit isFirstAddressResolved(hostname, resolvedAddress);
}

void MdnsBrowser::resolveServiceInstance(const QByteArray& serviceInstance, const std::chrono::milliseconds waitTime) const
{
	qCDebug(mdns_browser) << "Get details for service instance:" << serviceInstance;

	if (_cache.isNull())
	{
		emit isServiceRecordResolved(serviceInstance, QMdnsEngine::Record());
		return;
	}

	QByteArray serviceLookupName = serviceInstance;

	if (!serviceLookupName.endsWith('.'))
	{
		serviceLookupName.append('.');
	}

	QMdnsEngine::Record srvRecord;
	bool found { false };
	int retries { SERVICE_LOOKUP_RETRIES };

	while (!found && retries >= 0)
	{
		if (!_cache.isNull() && _cache->lookupRecord(serviceLookupName, QMdnsEngine::SRV, srvRecord))
		{
			found = true;
		}
		else
		{
			wait(waitTime);
			--retries;
		}
	}

	printCache(nullptr, QMdnsEngine::ANY);

	if (!_server.isNull())
	{
		if (found)
		{
			qCDebug(mdns_browser) << "Service record found for service instance:" << serviceLookupName;

		}
		else
		{
			qCWarning(mdns_browser) << "No service record found for service instance:" << serviceLookupName;
		}
	}
	emit isServiceRecordResolved(serviceInstance, srvRecord);
}

QMdnsEngine::Service MdnsBrowser::getFirstService(const QByteArray& serviceType, const QString& filter, const std::chrono::milliseconds waitTime) const
{
	qCDebug(mdns_browser) << "Get first service of type:" << serviceType << ", matching name:" << filter;

	if (_cache.isNull())
	{
		return {};
	}

	QMdnsEngine::Service service;
	QRegularExpression const regEx(filter);

	if (!regEx.isValid()) {
		QString errorString = regEx.errorString();
		qCWarning(mdns_browser) << "Filtering regular expression:" << filter << "error:" << regEx.patternErrorOffset() << ":" << errorString;
		return service;
	}

	bool found { false };
	int retries = 3;
	QList<QMdnsEngine::Record> ptrRecords;

	while (!found && retries >= 0)
	{
		if (!_cache.isNull() && _cache->lookupRecords(serviceType, QMdnsEngine::PTR, ptrRecords))
		{
			for (const auto& ptrRecord : std::as_const(ptrRecords))
			{
				QByteArray const serviceNameFull = ptrRecord.target();

				if (regEx.match(serviceNameFull.constData()).hasMatch())
				{
					QMdnsEngine::Record srvRecord;
					if (_cache->lookupRecord(serviceNameFull, QMdnsEngine::SRV, srvRecord))
					{
						service.setName(serviceNameFull.endsWith("." + serviceType)
										? serviceNameFull.left(serviceNameFull.length() - serviceType.length() - 1)
										: srvRecord.name());

						service.setPort(srvRecord.port());

						QByteArray hostName = srvRecord.target();
						hostName.chop(1);  // Remove trailing dot
						service.setHostname(hostName);
						service.setAttributes(srvRecord.attributes());
						found = true;
					}
				}
			}
		}

		if (!found)
		{
			wait(waitTime); // Uses your existing wait function with event loop
			--retries;
		}
	}

	if (found)
	{
		qCDebug(mdns_browser) << "Service of type:" << serviceType << "found";
	}
	else
	{
		qCWarning(mdns_browser) << "No service of type:" << serviceType << "found";
	}

	return service;
}

/// @brief Queries the local mDNS cache for services of a given type, filters
///        them by name, and returns the matching entries as a JSON array.
///
/// The function looks up PTR records for @p serviceType in the in-memory mDNS
/// cache.  For each PTR record whose target name matches the @p filter regular
/// expression, it resolves the companion SRV record to obtain the host name and
/// port, and optionally the TXT record for additional service metadata.  The
/// results are assembled into JSON objects and appended to the returned array.
///
/// @par Retry strategy
/// If the cache returns no matches on the first attempt, the function waits
/// @p waitTime and retries up to 3 times total before giving up.  This allows
/// the mDNS browser time to populate the cache with freshly received multicast
/// responses.
///
/// @par Returned JSON schema (one element of the array)
/// @code{.json}
/// {
///   "id":       "MyHyperion._tcp.local.",  // raw PTR target (includes trailing dot)
///   "service":  "MyHyperion._tcp.local",   // PTR target with trailing dot removed
///   "type":     "_hyperiond-http._tcp.local.",
///   "name":     "MyHyperion",              // human-readable service label
///   "hostname": "raspberrypi",             // SRV target host (trailing dot removed)
///   "domain":   "local.",
///   "sameHost": true,                      // true when the service runs on this machine
///   "port":     8090,
///   "txt": {                               // optional — only present when a TXT record exists
///     "version": "2.0.16"
///   }
/// }
/// @endcode
///
/// @param serviceType The mDNS service type to look up
///                    (e.g. @c "_hyperiond-http._tcp.local.").
/// @param filter      A @c QRegularExpression pattern applied to each PTR record's
///                    target name.  Pass an empty string or @c ".*" to match all.
/// @param waitTime    How long to wait between retry attempts when the cache is
///                    empty or no records match.
/// @return A @c QJsonArray of matching service descriptors.  Returns an empty
///         array when the cache is null, the @p filter regex is invalid, or no
///         matching services are found within the retry budget.
QJsonArray MdnsBrowser::getServicesDiscoveredJson(const QByteArray& serviceType, const QString& filter, const std::chrono::milliseconds waitTime) const
{
	qCDebug(mdns_browser) << "Get services of type:" << serviceType << ", matching name:" << filter;

	// Nothing to search if the cache has not been initialised yet.
	if (_cache.isNull())
	{
		return {};
	}

	QJsonArray result;

	// Validate the caller-supplied filter pattern before using it.
	QRegularExpression const regEx(filter);
	if (!regEx.isValid()) {
		QString const errorString = regEx.errorString();
		qsizetype const errorOffset = regEx.patternErrorOffset();

		qCWarning(mdns_browser) << "Filtering regular expression:" << filter << "error:" << errorOffset << ":" << errorString;
	}
	else
	{
		QList<QMdnsEngine::Record> ptrRecords;

		// Retry loop: wait for the cache to be populated if no records are found yet.
		int retries = 3;
		do
		{
			if (!_cache.isNull() && _cache->lookupRecords(serviceType, QMdnsEngine::PTR, ptrRecords))
			{
				for (const auto& ptrRecord : std::as_const(ptrRecords))
				{
					QByteArray const serviceName = ptrRecord.target();
					// Apply the name filter before doing any further record lookups.
					if (regEx.match(serviceName.constData()).hasMatch())
					{
						// Resolve the SRV record to get the host name and port.
						QMdnsEngine::Record srvRecord;
						if (!_cache->lookupRecord(serviceName, QMdnsEngine::SRV, srvRecord))
						{
							// A PTR record without a corresponding SRV record is incomplete — skip it.
							qCWarning(mdns_browser) << "No SRV record for:" << serviceName << "found, skip entry";
						}
						else
						{
							QJsonObject obj;
							QString const domain = "local.";

							obj.insert("id", serviceName.constData());

							// Strip the trailing dot that mDNS appends to fully-qualified names.
							QString service = serviceName;
							service.chop(1);
							obj.insert("service", service);
							obj.insert("type", serviceType.constData());

							// Extract the human-readable service label by removing the
							// service-type suffix from the full service name.
							QString name;
							if (serviceName.endsWith("." + serviceType))
							{
								name = serviceName.left(serviceName.length() - serviceType.length() - 1);
								obj.insert("name", QString(name));
							}

							QByteArray hostName = srvRecord.target();
							//Remove trailing dot
							hostName.chop(1);

							obj.insert("hostname", QString(hostName));
							obj.insert("domain", domain);

							// Tag records where the service is provided by this host so
							// the UI can highlight local instances.
							QByteArray localHostname = QHostInfo::localHostName().toUtf8();
							localHostname = localHostname.replace('.', '-');

							bool isSameHost {false};
							if ( name == localHostname )
							{
								isSameHost = true;
							}
							obj.insert("sameHost", isSameHost);

							quint16 const port = srvRecord.port();
							obj.insert("port", port);

							// Append TXT record attributes when available (e.g. version info).
							QMdnsEngine::Record txtRecord;
							if (_cache->lookupRecord(serviceName, QMdnsEngine::TXT, txtRecord))
							{
								QMap<QByteArray, QByteArray> const txtAttributes = txtRecord.attributes();

								QVariantMap txtMap;
								QMapIterator iterator(txtAttributes);
								while (iterator.hasNext()) {
									iterator.next();
									txtMap.insert(iterator.key(), iterator.value());
								}
								obj.insert("txt", QJsonObject::fromVariantMap(txtMap));
							}
							result << obj;
						}
					}
				}
			}

			// Back off and retry when the cache has not delivered any results yet.
			if ( result.isEmpty())
			{
				wait(waitTime);
				--retries;
			}

		} while (result.isEmpty() && retries >= 0);

		if (!result.isEmpty())
		{
			qCDebug(mdns_browser).noquote() << "result:" << JsonUtils::toCompact(result);
		}
		else
		{
			qCWarning(mdns_browser) << "No service of type:" << serviceType << "found";
		}
	}

	return result;
}

void MdnsBrowser::printCache(const QByteArray& name, quint16 type) const
{
	qCDebug(mdns_browser_cache) << "mDNS Browser Cache for type:" << QMdnsEngine::typeName(type);
	QList<QMdnsEngine::Record> records;
	if (!_cache.isNull() && _cache->lookupRecords(name, type, records))
	{
		foreach(QMdnsEngine::Record const record, records)
		{
			qCDebug(mdns_browser_cache) << QMdnsEngine::typeName(record.type()) << "," << record.name() << ", ttl       :" << record.ttl();

			switch (record.type()) {
			case QMdnsEngine::PTR:
				qCDebug(mdns_browser_cache) << QMdnsEngine::typeName(record.type()) << "," << record.name() << ", target    :" << record.target();
			break;

			case QMdnsEngine::SRV:
				qCDebug(mdns_browser_cache) << QMdnsEngine::typeName(record.type()) << "," << record.name() << ", target    :" << record.target();
				qCDebug(mdns_browser_cache) << QMdnsEngine::typeName(record.type()) << "," << record.name() << ", port      :" << record.port();
				qCDebug(mdns_browser_cache) << QMdnsEngine::typeName(record.type()) << "," << record.name() << ", priority  :" << record.priority();
				qCDebug(mdns_browser_cache) << QMdnsEngine::typeName(record.type()) << "," << record.name() << ", weight    :" << record.weight();
			break;
			case QMdnsEngine::TXT:
				qCDebug(mdns_browser_cache) << QMdnsEngine::typeName(record.type()) << "," << record.name() << ", attributes:" << record.attributes();
			break;

			case QMdnsEngine::NSEC:
				qCDebug(mdns_browser_cache) << QMdnsEngine::typeName(record.type()) << "," << record.name() << ", nextDomNam:" << record.nextDomainName();
			break;

			case QMdnsEngine::A:
			case QMdnsEngine::AAAA:
				qCDebug(mdns_browser_cache) << QMdnsEngine::typeName(record.type()) << "," << record.name() << ", address   :" << record.address();
			break;
			default:
			break;
			}
		}
	}
	else
	{
		qCDebug(mdns_browser) << "Cache is empty for type:" << QMdnsEngine::typeName(type);
	}
}
