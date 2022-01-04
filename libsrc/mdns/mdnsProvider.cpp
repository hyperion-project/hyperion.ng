#include <mdns/mdnsProvider.h>

//Qt includes
#include <QHostInfo>
#include <QThread>

// Utility includes
#include <utils/Logger.h>
#include <HyperionConfig.h>
#include <hyperion/AuthManager.h>

namespace {
	const bool verboseProvider = false;
} //End of constants

MdnsProvider::MdnsProvider(QObject* parent)
	: QObject(parent)
	, _log(Logger::getInstance("MDNS"))
	, _server(nullptr)
	, _hostname(nullptr)
{
}

void MdnsProvider::init()
{
	_server = new QMdnsEngine::Server();
	_hostname = new QMdnsEngine::Hostname(_server);

	connect(_hostname, &QMdnsEngine::Hostname::hostnameChanged, this, &MdnsProvider::onHostnameChanged);
	DebugIf(verboseProvider, _log, "Hostname [%s], isRegistered [%d]", _hostname->hostname().constData(), _hostname->isRegistered());
}

MdnsProvider::~MdnsProvider()
{
	qDeleteAll(_providedServiceTypes);

	_hostname->deleteLater();
	_server->deleteLater();
}

void MdnsProvider::publishService(const QByteArray& serviceType, quint16 servicePort, const QByteArray& serviceName)
{
	DebugIf(verboseProvider, _log, "Publish new mDNS serviceType [%s], Thread: %s", serviceType.constData(), QSTRING_CSTR(QThread::currentThread()->objectName()));

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

	DebugIf(verboseProvider, _log, "[%s], Name: [%s], Port: [%u] ", service.type().constData(), service.name().constData(), service.port());

	provider->update(service);
}

void MdnsProvider::onHostnameChanged(const QByteArray& hostname)
{
	DebugIf(verboseProvider, _log, "mDNS-hostname changed to hostname [%s]", hostname.constData());
}
