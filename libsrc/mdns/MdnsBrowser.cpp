#include <mdns/MdnsBrowser.h>
#include <qmdnsengine/message.h>
#include <qmdnsengine/service.h>

// Qt includes
#include <QThread>
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

namespace {
const bool verboseBrowser = false;
const int SERVICE_LOOKUP_RETRIES = 5;
} // End of constants

QSharedPointer<MdnsBrowser> MdnsBrowser::instance;

MdnsBrowser::MdnsBrowser(QObject* parent)
	: QObject(parent)
	, _log(Logger::getInstance("MDNS"))
	, _server(nullptr)
	, _cache(nullptr)
{
	qRegisterMetaType<QMdnsEngine::Message>("Message");
	qRegisterMetaType<QHostAddress>("QHostAddress");
}

MdnsBrowser::~MdnsBrowser()
{
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
	_browsedServiceTypes.clear();
	_cache.reset();
	_server.reset();
}

QSharedPointer<MdnsBrowser>& MdnsBrowser::getInstance(QThread* externalThread)
{
	static QSharedPointer<MdnsBrowser> instance;

	if (instance.isNull())
	{
		instance.reset(new MdnsBrowser());

		if (externalThread != nullptr) // Move to existing thread if provided
		{
			instance->moveToThread(externalThread);

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

void MdnsBrowser::browseForServiceType(const QByteArray& serviceType)
{
	if (!_browsedServiceTypes.contains(serviceType))
	{
		DebugIf(verboseBrowser, _log, "Start new mDNS browser for serviceType [%s], Thread: %s", serviceType.constData(), QSTRING_CSTR(QThread::currentThread()->objectName()));
		QSharedPointer<QMdnsEngine::Browser> const newBrowser = QSharedPointer<QMdnsEngine::Browser>::create(_server.get(), serviceType, _cache.get());

		QObject::connect(newBrowser.get(), &QMdnsEngine::Browser::serviceAdded, this, &MdnsBrowser::onServiceAdded);
		QObject::connect(newBrowser.get(), &QMdnsEngine::Browser::serviceUpdated, this, &MdnsBrowser::onServiceUpdated);
		QObject::connect(newBrowser.get(), &QMdnsEngine::Browser::serviceRemoved, this, &MdnsBrowser::onServiceRemoved);

		_browsedServiceTypes.insert(serviceType, newBrowser);
	}
	else
	{
		DebugIf(verboseBrowser, _log, "Use existing mDNS browser for serviceType [%s], Thread: %s", serviceType.constData(), QSTRING_CSTR(QThread::currentThread()->objectName()));
	}
}

void MdnsBrowser::onServiceAdded(const QMdnsEngine::Service& service)
{
	DebugIf(verboseBrowser, _log, "Discovered service [%s] at host: %s, port: %u, Thread: %s", service.name().constData(), service.hostname().constData(), service.port(), QSTRING_CSTR(QThread::currentThread()->objectName()));
	emit serviceFound(service);
}

void MdnsBrowser::onServiceUpdated(const QMdnsEngine::Service& service)
{
	DebugIf(verboseBrowser, _log, "[%s], Name: [%s], Port: [%u], Thread: %s", service.type().constData(), service.name().constData(), service.port(), QSTRING_CSTR(QThread::currentThread()->objectName()));
}

void MdnsBrowser::onServiceRemoved(const QMdnsEngine::Service& service)
{
	DebugIf(verboseBrowser, _log, "[%s], Name: [%s], Port: [%u], Thread: %s", service.type().constData(), service.name().constData(), service.port(), QSTRING_CSTR(QThread::currentThread()->objectName()));
	emit serviceRemoved(service);
}

void MdnsBrowser::onHostNameResolved(const QHostAddress& address)
{
	DebugIf(verboseBrowser, _log, "for address [%s], Thread: %s", QSTRING_CSTR(address.toString()), QSTRING_CSTR(QThread::currentThread()->objectName()));

	// Do not publish link local addresses
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
	if (!address.isLinkLocal())
#else
	if (!address.toString().startsWith("fe80"))
#endif
	{
		emit isAddressResolved(address);
	}
}

void MdnsBrowser::resolveFirstAddress(Logger* log, const QString& hostname, std::chrono::milliseconds timeout)
{
	qRegisterMetaType<QMdnsEngine::Message>("Message");

	QHostAddress resolvedAddress;

	if (hostname.endsWith(".local") || hostname.endsWith(".local."))
	{
		QMdnsEngine::Resolver const resolver (_server.get(), hostname.toUtf8(), _cache.get());
		connect(&resolver, &QMdnsEngine::Resolver::resolved, this, &MdnsBrowser::onHostNameResolved);

		DebugIf(verboseBrowser, log, "Wait for resolver on hostname [%s]", QSTRING_CSTR(hostname));

		QEventLoop loop;
		QTimer timer;

		timer.setSingleShot(true);
		connect(&timer, &QTimer::timeout, this, [&loop]() {
			loop.quit();  // Stop waiting if timeout occurs
		});

		std::unique_ptr<QObject> context{new QObject};
		QObject* pcontext = context.get();
		connect(this, &MdnsBrowser::isAddressResolved, pcontext, [ &loop, &resolvedAddress, context = std::move(context)](const QHostAddress &address) mutable {
			resolvedAddress = address;
			loop.quit();
			context.reset();
		});

		timer.start(timeout);
		loop.exec();

		if (!resolvedAddress.isNull())
		{
			Debug(log, "Resolved mDNS hostname [%s] to address [%s]", QSTRING_CSTR(hostname), QSTRING_CSTR(resolvedAddress.toString()));
		}
		else
		{
			Error(log, "Failed to resolve mDNS hostname [%s]", QSTRING_CSTR(hostname));
		}
	}
	else
	{
		Error(log, "Hostname [%s] is not an mDNS hostname.", QSTRING_CSTR(hostname));
	}

	emit isFirstAddressResolved(resolvedAddress);
}

void MdnsBrowser::resolveServiceInstance(const QByteArray& serviceInstance, const std::chrono::milliseconds waitTime) const
{
	DebugIf(verboseBrowser, _log, "Get service instance [%s] details, Thread: %s",serviceInstance.constData(), QSTRING_CSTR(QThread::currentThread()->objectName()));

	if (_cache.isNull())
	{
		return;
	}

	QByteArray service{ serviceInstance };

	if (!service.endsWith('.'))
	{
		service.append('.');
	}

	QMdnsEngine::Record srvRecord;
	bool found { false };
	int retries { SERVICE_LOOKUP_RETRIES };

	while (!found && retries >= 0)
	{
		if (!_cache.isNull() && _cache->lookupRecord(service, QMdnsEngine::SRV, srvRecord))
		{
			found = true;
		}
		else
		{
			wait(waitTime);
			--retries;
		}
	}

	if (!_server.isNull())
	{
		if (found)
		{
			DebugIf(verboseBrowser, _log, "Service record found for service instance [%s]", service.constData());

		}
		else
		{
			Debug(_log, "No service record found for service instance [%s]", service.constData());
		}
	}
	emit isServiceRecordResolved(srvRecord);
}

QMdnsEngine::Service MdnsBrowser::getFirstService(const QByteArray& serviceType, const QString& filter, const std::chrono::milliseconds waitTime) const
{
	DebugIf(verboseBrowser, _log, "Get first service of type [%s], matching name: [%s]", QSTRING_CSTR(QString(serviceType)), QSTRING_CSTR(filter));

	if (_cache.isNull())
	{
		return {};
	}

	QMdnsEngine::Service service;
	QRegularExpression const regEx(filter);

	if (!regEx.isValid()) {
		QString errorString = regEx.errorString();
		qsizetype const errorOffset = regEx.patternErrorOffset();

		Error(_log, "Filtering regular expression [%s] error [%d]:[%s]", QSTRING_CSTR(filter), errorOffset, QSTRING_CSTR(errorString));
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
		DebugIf(verboseBrowser, _log, "Service of type [%s] found", serviceType.constData());
	}
	else
	{
		Debug(_log, "No service of type [%s] found", serviceType.constData());
	}

	return service;
}

QJsonArray MdnsBrowser::getServicesDiscoveredJson(const QByteArray& serviceType, const QString& filter, const std::chrono::milliseconds waitTime) const
{
	DebugIf(verboseBrowser,_log, "Get services of type [%s], matching name: [%s], Thread: %s", QSTRING_CSTR(QString(serviceType)), QSTRING_CSTR(filter), QSTRING_CSTR(QThread::currentThread()->objectName()));

	if (_cache.isNull())
	{
		return {};
	}

	QJsonArray result;

	QRegularExpression const regEx(filter);
	if (!regEx.isValid()) {
		QString const errorString = regEx.errorString();
		qsizetype const errorOffset = regEx.patternErrorOffset();

		Error(_log, "Filtering regular expression [%s] error [%d]:[%s]", QSTRING_CSTR(filter), errorOffset, QSTRING_CSTR(errorString));
	}
	else
	{
		QList<QMdnsEngine::Record> ptrRecords;

		int retries = 3;
		do
		{
			if (!_cache.isNull() && _cache->lookupRecords(serviceType, QMdnsEngine::PTR, ptrRecords))
			{
				for (const auto& ptrRecord : std::as_const(ptrRecords))
				{
					QByteArray const serviceName = ptrRecord.target();
					if (regEx.match(serviceName.constData()).hasMatch())
					{
						QMdnsEngine::Record srvRecord;
						if (!_cache->lookupRecord(serviceName, QMdnsEngine::SRV, srvRecord))
						{
							Debug(_log, "No SRV record for [%s] found, skip entry", serviceName.constData());
						}
						else
						{
							QJsonObject obj;
							QString const domain = "local.";

							obj.insert("id", serviceName.constData());

							QString service = serviceName;
							service.chop(1);
							obj.insert("service", service);
							obj.insert("type", serviceType.constData());

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

							//Tag records where the service is provided by this host
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

							QMdnsEngine::Record txtRecord;
							if (_cache->lookupRecord(serviceName, QMdnsEngine::TXT, txtRecord))
							{
								QMap<QByteArray, QByteArray> const txtAttributes = txtRecord.attributes();

								QVariantMap txtMap;
								QMapIterator<QByteArray, QByteArray> iterator(txtAttributes);
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

			if ( result.isEmpty())
			{
				wait(waitTime);
				--retries;
			}

		} while (result.isEmpty() && retries >= 0);

		if (!result.isEmpty())
		{
			DebugIf(verboseBrowser,_log, "result: [%s]", QSTRING_CSTR(JsonUtils::jsonValueToQString(result)));
		}
		else
		{
			Debug(_log, "No service of type [%s] found", serviceType.constData());

		}
	}

	return result;
}

void MdnsBrowser::printCache(const QByteArray& name, quint16 type) const
{
	DebugIf(verboseBrowser,_log, "for type: %s", QSTRING_CSTR(QMdnsEngine::typeName(type)));
	QList<QMdnsEngine::Record> records;
	if (!_cache.isNull() && _cache->lookupRecords(name, type, records))
	{
		foreach(QMdnsEngine::Record const record, records)
		{
			qDebug() << QMdnsEngine::typeName(record.type()) << "," << record.name() << "], ttl       : " << record.ttl();

			switch (record.type()) {
			case QMdnsEngine::PTR:
				qDebug() << QMdnsEngine::typeName(record.type()) << "," << record.name() << ", target    : " << record.target();
			break;

			case QMdnsEngine::SRV:
				qDebug() << QMdnsEngine::typeName(record.type()) << "," << record.name() << ", target    : " << record.target();
				qDebug() << QMdnsEngine::typeName(record.type()) << "," << record.name() << ", port      : " << record.port();
				qDebug() << QMdnsEngine::typeName(record.type()) << "," << record.name() << ", priority  : " << record.priority();
				qDebug() << QMdnsEngine::typeName(record.type()) << "," << record.name() << ", weight    : " << record.weight();
			break;
			case QMdnsEngine::TXT:
				qDebug() << QMdnsEngine::typeName(record.type()) << "," << record.name() << ", attributes: " << record.attributes();
			break;

			case QMdnsEngine::NSEC:
				qDebug() << QMdnsEngine::typeName(record.type()) << "," << record.name() << ", nextDomNam: " << record.nextDomainName();
			break;

			case QMdnsEngine::A:
			case QMdnsEngine::AAAA:
				qDebug() << QMdnsEngine::typeName(record.type()) << "," << record.name() << ", address   : " << record.address();
			break;
			default:
			break;
			}
		}
	}
	else
	{
		DebugIf(verboseBrowser,_log, "Cash is empty for type: %s", QSTRING_CSTR(QMdnsEngine::typeName(type)));
	}
}
