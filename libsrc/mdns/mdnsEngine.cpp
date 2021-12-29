#include <mdns/mdnsEngine.h>
#include <qmdnsengine/resolver.h>
#include <qmdnsengine/record.h>
#include <qmdnsengine/message.h>

//Qt includes
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QHostInfo>
#include <QThread>

// Utility includes
#include <utils/Logger.h>
#include <HyperionConfig.h>
#include <hyperion/AuthManager.h>


namespace {
	const bool verbose = true;
	const bool verboseProvider = true;
} //End of constants

//MdnsEngine* MdnsEngine::instance = nullptr;

MdnsEngine::MdnsEngine(QObject* parent)
	: QObject(parent)
	, _log(Logger::getInstance("MDNS"))
	, _server(nullptr)
	, _hostname(nullptr)
	, _provider(nullptr)
	, _cache(nullptr)
{
	//MdnsEngine::instance = this;

	qRegisterMetaType<QMdnsEngine::Service>("Service");
	qRegisterMetaType<QMdnsEngine::Message>("Message");
}

void MdnsEngine::initEngine()
{
	Debug(_log, "");
	_server = new QMdnsEngine::Server();
	_cache = new QMdnsEngine::Cache();
	_hostname = new QMdnsEngine::Hostname(_server);

	connect(_hostname, &QMdnsEngine::Hostname::hostnameChanged, this, &MdnsEngine::onHostnameChanged);
	DebugIf(verboseProvider, _log, "Hostname [%s], isRegistered [%d]", QSTRING_CSTR(QString(_hostname->hostname())), _hostname->isRegistered());

	browseForServiceType("_hyperiond-flatbuf._tcp.local.");
	browseForServiceType("_hyperiond-json._tcp.local.");
	browseForServiceType("_http._tcp.local.");
	browseForServiceType("_https._tcp.local.");
}

MdnsEngine::~MdnsEngine()
{
	QMapIterator<QByteArray, QMdnsEngine::Browser*> b(_browsedServiceTypes);
	while (b.hasNext()) {
		b.next();
		QMdnsEngine::Browser* browserPtr = b.value();
		browserPtr->disconnect();
		delete browserPtr;
	}

	QMapIterator<QByteArray, QMdnsEngine::Provider*> p(_providedServiceTypes);
	while (p.hasNext()) {
		p.next();
		QMdnsEngine::Provider* providerPtr = p.value();
		delete providerPtr;
	}

	_hostname->disconnect();

	delete _cache;
	delete _provider;
	delete _hostname;
	delete _server;
}

void MdnsEngine::provideServiceType(const QByteArray& serviceType, quint16 servicePort, const QByteArray& serviceName)
{
	DebugIf(verbose, _log, "Start new Provider for serviceType [%s]", QSTRING_CSTR(QString(serviceType)));
	qDebug() << "\nMdnsEngine::provideServiceType" << QThread::currentThread();

	QMdnsEngine::Provider* provider(nullptr);

	if (!_providedServiceTypes.contains(serviceType))
	{
		provider = new QMdnsEngine::Provider(_server, _hostname);
		_providedServiceTypes.insert(serviceType, provider);
	}
	else
	{
		provider = _providedServiceTypes[serviceType];
	}

	QMdnsEngine::Service service;
	service.setType(serviceType);
	service.setPort(servicePort);

	QByteArray name("hyperion");
	if (!serviceName.isEmpty())
	{
		name += "-" + serviceName;
	}
	name += "@" + QHostInfo::localHostName().toUtf8();
	service.setName(name);

	QByteArray id = AuthManager::getInstance()->getID().toUtf8();
	const QMap<QByteArray, QByteArray> attributes = { {"id", id}, {"version", HYPERION_VERSION} };
	service.setAttributes(attributes);

	DebugIf(verboseProvider, _log, "[%s] Name: [%s], Hostname[%s], Port: [%u] ",
		QSTRING_CSTR(QString(service.type())),
		QSTRING_CSTR(QString(service.name())),
		QSTRING_CSTR(QString(service.hostname())), service.port());
		
	provider->update(service);
}

void MdnsEngine::browseForServiceType(const QByteArray& serviceType)
{
	qDebug() << "\nMdnsEngine::browseForServiceType" << QThread::currentThread();
	if (!_browsedServiceTypes.contains(serviceType))
	{
		DebugIf(verbose, _log, "Start new Browser for serviceType [%s]", QSTRING_CSTR(QString(serviceType)));
		QMdnsEngine::Browser* newBrowser = new QMdnsEngine::Browser(_server, serviceType, _cache);

		QObject::connect(newBrowser, &QMdnsEngine::Browser::serviceAdded, this, &MdnsEngine::onServiceAdded);
		QObject::connect(newBrowser, &QMdnsEngine::Browser::serviceUpdated, this, &MdnsEngine::onServiceUpdated);
		QObject::connect(newBrowser, &QMdnsEngine::Browser::serviceRemoved, this, &MdnsEngine::onServiceRemoved);

		_browsedServiceTypes.insert(serviceType, newBrowser);
	}
}

void MdnsEngine::onHostnameChanged(const QByteArray& hostname)
{
	DebugIf(verboseProvider, _log, "Hostname changed to Hostname [%s]", QSTRING_CSTR(QString(hostname)));
}

void MdnsEngine::onServiceAdded(const QMdnsEngine::Service& service)
{
	DebugIf(verbose, _log, "[%s] Name: [%s], Hostname[%s], Port: [%u] ",
		QSTRING_CSTR(QString(service.type())),
		QSTRING_CSTR(QString(service.name())),
		QSTRING_CSTR(QString(service.hostname())), service.port());
	resolveService(service);
}

void MdnsEngine::onServiceUpdated(const QMdnsEngine::Service& service)
{
	DebugIf(verbose, _log, "[%s] Name: [%s], Hostname[%s], Port: [%u] ",
		QSTRING_CSTR(QString(service.type())),
		QSTRING_CSTR(QString(service.name())),
		QSTRING_CSTR(QString(service.hostname())), service.port());
	resolveService(service);
}

void MdnsEngine::resolveService(const QMdnsEngine::Service& service)
{
	DebugIf(verbose, _log, "[%s] Name: [%s], Hostname[%s], Port: [%u] ",
		QSTRING_CSTR(QString(service.type())),
		QSTRING_CSTR(QString(service.name())),
		QSTRING_CSTR(QString(service.hostname())), service.port());

	emit resolveHostName(service.hostname());
}

void MdnsEngine::resolveHostName(const QByteArray& hostName)
{
	DebugIf(verbose, _log, "Hostname[%s]", QSTRING_CSTR(QString(hostName)));

	qRegisterMetaType<QMdnsEngine::Message>("Message");
	auto* resolver = new QMdnsEngine::Resolver(_server, hostName, _cache);
	connect(resolver, &QMdnsEngine::Resolver::resolved, this, &MdnsEngine::onHostNameResolved);

	//qRegisterMetaType<QMdnsEngine::Message>("Message");
	//QMdnsEngine::Resolver resolver(_server, hostName, _cache);
	//connect(&resolver, &QMdnsEngine::Resolver::resolved, [](const QHostAddress& hostAddress) {
	//	qDebug() << "\nMdnsEngine::getHostAddress" << QThread::currentThread();
	//	qDebug() << "\nMdnsEngine::hostAddress" << hostAddress;
	//});
}

void MdnsEngine::onHostNameResolved(const QHostAddress& address)
{
	switch (address.protocol()) {
	case QAbstractSocket::IPv4Protocol:
		DebugIf(verbose, _log, "resolved to IP4 [%s]", QSTRING_CSTR(address.toString()));
		break;
	case QAbstractSocket::IPv6Protocol:
		DebugIf(verbose, _log, "resolved to IP6 [%s]", QSTRING_CSTR(address.toString()));
		break;
	default:
		break;
	}
}

void MdnsEngine::onServiceRemoved(const QMdnsEngine::Service& service)
{
	DebugIf(verbose, _log, "[%s] Name: [%s], Hostname[%s], Port: [%u] ",
		QSTRING_CSTR(QString(service.type())),
		QSTRING_CSTR(QString(service.name())),
		QSTRING_CSTR(QString(service.hostname())), service.port());
}

QHostAddress MdnsEngine::getHostAddress(const QString& hostName)
{
	return getHostAddress(hostName.toUtf8());
}

QHostAddress MdnsEngine::getHostAddress(const QByteArray& hostName)
{
	Debug(_log, "Resolve IP-address for hostname [%s].", QSTRING_CSTR(QString(hostName)));

	qDebug() << "\nMdnsEngine::getHostAddress" << QThread::currentThread();

	QHostAddress hostAddress;

	QMdnsEngine::Record aRecord;
	if (!_cache->lookupRecord(hostName, QMdnsEngine::A, aRecord))
	{
		DebugIf(verbose, _log, "IP-address for hostname [%s] not yet in cache, start resolver.", QSTRING_CSTR(QString(hostName)));
		//emit resolveHostName(hostName);
	}
	else
	{
		hostAddress = aRecord.address();
		Debug(_log, "Hostname [%s] translates to IP-address [%s]", QSTRING_CSTR(QString(hostName)), QSTRING_CSTR(hostAddress.toString()));
	}
	return hostAddress;
}

QString MdnsEngine::getHostByService(const QByteArray& serviceName)
{
	QString hostAddress;
	Debug(_log, "Resolve IP-address for service [%s].", QSTRING_CSTR(QString(serviceName)));

	qDebug() << "\nMdnsEngine::getHostAddress" << QThread::currentThread();

	QMdnsEngine::Record srvRecord;
	if (!_cache->lookupRecord(serviceName, QMdnsEngine::SRV, srvRecord))
	{
		Debug(_log, "No SRV record for [%s] found, skip entry", QSTRING_CSTR(QString(serviceName)));
	}
	else
	{
		QByteArray hostName = srvRecord.target();
		quint16 port = srvRecord.port();

		QMdnsEngine::Record aRecord;
		if (!_cache->lookupRecord(hostName, QMdnsEngine::A, aRecord))
		{
			DebugIf(verbose, _log, "IP-address for hostname [%s] not yet in cache, start resolver.", QSTRING_CSTR(QString(hostName)));
		}
		else
		{
			hostAddress = QString("%1:%2").arg(aRecord.address().toString()).arg(port);
			Debug(_log, "Service [%s] translates to [%s]", QSTRING_CSTR(QString(serviceName)), QSTRING_CSTR(hostAddress));
		}
	}
	return hostAddress;
}

QVariantList MdnsEngine::getServicesDiscoveredJson(const QByteArray& serviceType, const QString& filter) const
{
	DebugIf(verbose,_log, "Get services of type [%s], matching name: [%s]", QSTRING_CSTR(QString(serviceType)), QSTRING_CSTR(filter));

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

		if (_cache->lookupRecords(serviceType, QMdnsEngine::PTR, ptrRecords))
		{
			for (int ptrCounter = 0; ptrCounter < ptrRecords.size(); ++ptrCounter)
			{
				QByteArray serviceNameFull = ptrRecords.at(ptrCounter).target();

				QRegularExpressionMatch match = regEx.match(serviceNameFull);
				if (match.hasMatch())
				{
					QMdnsEngine::Record srvRecord;
					if (!_cache->lookupRecord(serviceNameFull, QMdnsEngine::SRV, srvRecord))
					{
						Debug(_log, "No SRV record for [%s] found, skip entry", QSTRING_CSTR(QString(serviceNameFull)));
					}
					else
					{
						QJsonObject obj;
						QString domain = "local.";

						obj.insert("id", QString(serviceNameFull));
						obj.insert("nameFull", QString(serviceNameFull));
						obj.insert("type", QString(serviceType.left(serviceType.size()-domain.size())));

						if (serviceNameFull.endsWith("." + serviceType))
						{
							QString serviceName = serviceNameFull.left(serviceNameFull.length() - serviceType.length() - 1);
							obj.insert("name", QString(serviceName));
						}

						QByteArray hostName = srvRecord.target();
						obj.insert("hostname", QString(hostName));
						obj.insert("domain", domain);

						quint16 port = srvRecord.port();
						obj.insert("port", port);

						QMdnsEngine::Record txtRecord;
						if (_cache->lookupRecord(serviceNameFull, QMdnsEngine::TXT, txtRecord))
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

						QMdnsEngine::Record aRecord;
						if (_cache->lookupRecord(hostName, QMdnsEngine::A, aRecord))
						{
							QHostAddress hostAddress = aRecord.address();
							obj.insert("address", hostAddress.toString());
						}
						result << obj;
					}
				}
			}
			DebugIf(verbose,_log, "result: [%s]", QString(QJsonDocument(result).toJson(QJsonDocument::Compact)).toUtf8().constData());
		}
		else
		{
			Debug(_log, "No service of type [%s] found", QSTRING_CSTR(QString(serviceType)));
		}
	}

	return result.toVariantList();
}

void MdnsEngine::printCache(const QByteArray& name, quint16 type) const
{
	QList<QMdnsEngine::Record> records;
	if (_cache->lookupRecords(name, type, records))
	{
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
}
