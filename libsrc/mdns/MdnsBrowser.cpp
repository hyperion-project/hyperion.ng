#include <mdns/MdnsBrowser.h>
#include <qmdnsengine/message.h>
#include <qmdnsengine/service.h>

//Qt includes
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

namespace {
	const bool verboseBrowser = false;
} //End of constants

MdnsBrowser::MdnsBrowser(QObject* parent)
	: QObject(parent)
	, _log(Logger::getInstance("MDNS"))
{
	qRegisterMetaType<QHostAddress>("QHostAddress");
}

MdnsBrowser::~MdnsBrowser()
{
	qDeleteAll(_browsedServiceTypes);
}

void MdnsBrowser::browseForServiceType(const QByteArray& serviceType)
{
	if (!_browsedServiceTypes.contains(serviceType))
	{
		DebugIf(verboseBrowser, _log, "Start new mDNS browser for serviceType [%s], Thread: %s", serviceType.constData(), QSTRING_CSTR(QThread::currentThread()->objectName()));
		QMdnsEngine::Browser* newBrowser = new QMdnsEngine::Browser(&_server, serviceType, &_cache);

		QObject::connect(newBrowser, &QMdnsEngine::Browser::serviceAdded, this, &MdnsBrowser::onServiceAdded);
		QObject::connect(newBrowser, &QMdnsEngine::Browser::serviceUpdated, this, &MdnsBrowser::onServiceUpdated);
		QObject::connect(newBrowser, &QMdnsEngine::Browser::serviceRemoved, this, &MdnsBrowser::onServiceRemoved);

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

QHostAddress MdnsBrowser::getHostFirstAddress(const QByteArray& hostname)
{
	DebugIf(verboseBrowser, _log, "for hostname [%s], Thread: %s", hostname.constData(), QSTRING_CSTR(QThread::currentThread()->objectName()));
	QByteArray toBeResolvedHostName {hostname};

	QHostAddress hostAddress;

	if (toBeResolvedHostName.endsWith(".local"))
	{
		toBeResolvedHostName.append('.');
	}
	if (toBeResolvedHostName.endsWith(".local."))
	{
		QList<QMdnsEngine::Record> aRecords;
		if (_cache.lookupRecords(toBeResolvedHostName, QMdnsEngine::A, aRecords))
		{
			foreach(QMdnsEngine::Record record, aRecords)
			{
				// Do not publish link local addresses
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
				if (!record.address().isLinkLocal())
#else
				if (!record.address().toString().startsWith("fe80"))
#endif
				{
					hostAddress = record.address();
					DebugIf(verboseBrowser, _log, "Hostname [%s] translates to IPv4-address [%s]", toBeResolvedHostName.constData(), QSTRING_CSTR(hostAddress.toString()));
					break;
				}
			}
		}
		else
		{
			QList<QMdnsEngine::Record> aaaaRecords;
			if (_cache.lookupRecords(toBeResolvedHostName, QMdnsEngine::AAAA, aaaaRecords))
			{
				foreach(QMdnsEngine::Record record, aaaaRecords)
				{
					// Do not publish link local addresses
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
					if (!record.address().isLinkLocal())
#else
					if (!record.address().toString().startsWith("fe80"))
#endif
					{
						hostAddress = record.address();
						DebugIf(verboseBrowser, _log, "Hostname [%s] translates to IPv6-address [%s]", toBeResolvedHostName.constData(), QSTRING_CSTR(hostAddress.toString()));
						break;
					}
				}
			}
			else
			{
				DebugIf(verboseBrowser, _log, "IP-address for hostname [%s] not yet in cache, start resolver.", toBeResolvedHostName.constData());
				qRegisterMetaType<QMdnsEngine::Message>("Message");
				auto* resolver = new QMdnsEngine::Resolver(&_server, toBeResolvedHostName, &_cache);
				connect(resolver, &QMdnsEngine::Resolver::resolved, this, &MdnsBrowser::onHostNameResolved);
			}
		}
	}
	return hostAddress;
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
		emit addressResolved(address);
	}
}

bool MdnsBrowser::resolveAddress(Logger* log, const QString& hostname, QHostAddress& hostAddress, std::chrono::milliseconds timeout)
{
	DebugIf(verboseBrowser, _log, "Get address for hostname [%s], Thread: %s", QSTRING_CSTR(hostname), QSTRING_CSTR(QThread::currentThread()->objectName()));

	bool isHostAddressOK{ false };
	if (hostname.endsWith(".local") || hostname.endsWith(".local."))
	{
		hostAddress = getHostFirstAddress(hostname.toUtf8());

		if (hostAddress.isNull())
		{
			DebugIf(verboseBrowser, _log, "Wait for resolver on hostname [%s]", QSTRING_CSTR(hostname));

			QEventLoop loop;
			QTimer t;
			QObject::connect(&MdnsBrowser::getInstance(), &MdnsBrowser::addressResolved, &loop, &QEventLoop::quit);

			weakConnect(&MdnsBrowser::getInstance(), &MdnsBrowser::addressResolved,
				[&hostAddress, hostname](const QHostAddress& resolvedAddress) {
					DebugIf(verboseBrowser, Logger::getInstance("MDNS"), "Resolver resolved hostname [%s] to address [%s], Thread: %s", QSTRING_CSTR(hostname), QSTRING_CSTR(resolvedAddress.toString()), QSTRING_CSTR(QThread::currentThread()->objectName()));
					hostAddress = resolvedAddress;
				});

			QTimer::connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
			t.start(static_cast<int>(timeout.count()));
			loop.exec();
		}

		if (!hostAddress.isNull())
		{
			Debug(log, "Resolved mDNS hostname [%s] to address [%s]", QSTRING_CSTR(hostname), QSTRING_CSTR(hostAddress.toString()));
			isHostAddressOK = true;
		}
		else
		{
			Error(log, "Resolved mDNS hostname [%s] timed out", QSTRING_CSTR(hostname));
		}
	}
	else
	{
		Error(log, "Hostname [%s] is not an mDNS hostname.", QSTRING_CSTR(hostname));
		isHostAddressOK = false;
	}
	return isHostAddressOK;
}

QMdnsEngine::Record MdnsBrowser::getServiceInstanceRecord(const QByteArray& serviceInstance, const std::chrono::milliseconds waitTime) const
{
	DebugIf(verboseBrowser, _log, "Get service instance [%s] details, Thread: %s",serviceInstance.constData(), QSTRING_CSTR(QThread::currentThread()->objectName()));

	QByteArray service{ serviceInstance };

	if (!service.endsWith('.'))
	{
		service.append('.');
	}

	QMdnsEngine::Record srvRecord;
	bool found{ false };
	int retries = 5;
	do
	{
		if (_cache.lookupRecord(service, QMdnsEngine::SRV, srvRecord))
		{
			found = true;
		}
		else
		{
			wait(waitTime);
			--retries;
		}

	} while (!found && retries >= 0);

	if (found)
	{
		DebugIf(verboseBrowser, _log, "Service record found for service instance [%s]", service.constData());

	}
	else
	{
		Debug(_log, "No service record found for service instance [%s]", service.constData());
	}
	return srvRecord;
}

QMdnsEngine::Service MdnsBrowser::getFirstService(const QByteArray& serviceType, const QString& filter, const std::chrono::milliseconds waitTime) const
{
	DebugIf(verboseBrowser,_log, "Get first service of type [%s], matching name: [%s]", QSTRING_CSTR(QString(serviceType)), QSTRING_CSTR(filter));

	QMdnsEngine::Service service;

	QRegularExpression regEx(filter);
	if (!regEx.isValid()) {
		QString errorString = regEx.errorString();
		int errorOffset = regEx.patternErrorOffset();

		Error(_log, "Filtering regular expression [%s] error [%d]:[%s]", QSTRING_CSTR(filter), errorOffset, QSTRING_CSTR(errorString));
	}
	else
	{
		QList<QMdnsEngine::Record> ptrRecords;

		bool found {false};
		int retries = 3;
		do
		{
			if (_cache.lookupRecords(serviceType, QMdnsEngine::PTR, ptrRecords))
			{
				for (int ptrCounter = 0; ptrCounter < ptrRecords.size(); ++ptrCounter)
				{
					QByteArray serviceNameFull = ptrRecords.at(ptrCounter).target();

					QRegularExpressionMatch match = regEx.match(serviceNameFull.constData());
					if (match.hasMatch())
					{
						QMdnsEngine::Record srvRecord;
						if (!_cache.lookupRecord(serviceNameFull, QMdnsEngine::SRV, srvRecord))
						{
							DebugIf(verboseBrowser, _log, "No SRV record for [%s] found, skip entry", serviceNameFull.constData());
						}
						else
						{
							if (serviceNameFull.endsWith("." + serviceType))
							{
								service.setName(serviceNameFull.left(serviceNameFull.length() - serviceType.length() - 1));
							}
							else
							{
								service.setName(srvRecord.name());
							}
							service.setPort(srvRecord.port());

							QByteArray hostName = srvRecord.target();
							//Remove trailing dot
							hostName.chop(1);
							service.setHostname(hostName);
							service.setAttributes(srvRecord.attributes());
							found = true;
						}
					}
				}
			}
			else
			{
				wait(waitTime);
				--retries;
			}

		} while (!found && retries >= 0);

		if (found)
		{
			DebugIf(verboseBrowser,_log, "Service of type [%s] found", serviceType.constData());
		}
		else
		{
			Debug(_log, "No service of type [%s] found", serviceType.constData());

		}
	}

	return service;
}

QJsonArray MdnsBrowser::getServicesDiscoveredJson(const QByteArray& serviceType, const QString& filter, const std::chrono::milliseconds waitTime) const
{
	DebugIf(verboseBrowser,_log, "Get services of type [%s], matching name: [%s], Thread: %s", QSTRING_CSTR(QString(serviceType)), QSTRING_CSTR(filter), QSTRING_CSTR(QThread::currentThread()->objectName()));

	QJsonArray result;

	QRegularExpression regEx(filter);
	if (!regEx.isValid()) {
		QString errorString = regEx.errorString();
		int errorOffset = regEx.patternErrorOffset();

		Error(_log, "Filtering regular expression [%s] error [%d]:[%s]", QSTRING_CSTR(filter), errorOffset, QSTRING_CSTR(errorString));
	}
	else
	{
		QList<QMdnsEngine::Record> ptrRecords;

		int retries = 3;
		do
		{
			if (_cache.lookupRecords(serviceType, QMdnsEngine::PTR, ptrRecords))
			{
				for (int ptrCounter = 0; ptrCounter < ptrRecords.size(); ++ptrCounter)
				{
					QByteArray serviceName = ptrRecords.at(ptrCounter).target();

					QRegularExpressionMatch match = regEx.match(serviceName.constData());
					if (match.hasMatch())
					{
						QMdnsEngine::Record srvRecord;
						if (!_cache.lookupRecord(serviceName, QMdnsEngine::SRV, srvRecord))
						{
							Debug(_log, "No SRV record for [%s] found, skip entry", serviceName.constData());
						}
						else
						{
							QJsonObject obj;
							QString domain = "local.";

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

							quint16 port = srvRecord.port();
							obj.insert("port", port);

							QMdnsEngine::Record txtRecord;
							if (_cache.lookupRecord(serviceName, QMdnsEngine::TXT, txtRecord))
							{
								QMap<QByteArray, QByteArray> txtAttributes = txtRecord.attributes();

								QVariantMap txtMap;
								QMapIterator<QByteArray, QByteArray> i(txtAttributes);
								while (i.hasNext()) {
									i.next();
									txtMap.insert(i.key(), i.value());
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
			DebugIf(verboseBrowser,_log, "result: [%s]", QString(QJsonDocument(result).toJson(QJsonDocument::Compact)).toUtf8().constData());
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
	DebugIf(verboseBrowser,_log, "for type: ", QSTRING_CSTR(QMdnsEngine::typeName(type)));
	QList<QMdnsEngine::Record> records;
	if (_cache.lookupRecords(name, type, records))
	{
		qDebug() << "";
		foreach(QMdnsEngine::Record record, records)
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
			}
		}
	}
	else
	{
		DebugIf(verboseBrowser,_log, "Cash is empty for type: ", QSTRING_CSTR(QMdnsEngine::typeName(type)));
	}
}
