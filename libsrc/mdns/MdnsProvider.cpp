#include <mdns/MdnsProvider.h>
#include <mdns/MdnsServiceRegister.h>

// Qt includes
#include <QHostInfo>
#include <QThread>

// Utility includes
#include <utils/Logger.h>
#include <HyperionConfig.h>
#include <hyperion/AuthManager.h>

Q_LOGGING_CATEGORY(mdns_provider, "hyperion.mdns.provider")

MdnsProvider::MdnsProvider(QObject* parent)
	: QObject(parent)
	, _log(Logger::getInstance("MDNS"))
	, _server(nullptr)
	, _hostname(nullptr)
{
	TRACK_SCOPE();
}

MdnsProvider::~MdnsProvider()
{
	TRACK_SCOPE();
}

void MdnsProvider::init()
{
	_server.reset(new QMdnsEngine::Server());
	_hostname.reset(new QMdnsEngine::Hostname(_server.data()));

	connect(_hostname.data(), &QMdnsEngine::Hostname::hostnameChanged, this, &MdnsProvider::onHostnameChanged);
	qCDebug(mdns_provider) << "Hostname:" << _hostname->hostname() << ", isRegistered:" << _hostname->isRegistered();
}

void MdnsProvider::stop()
{
	_providedServiceTypes.clear();
	_server.reset();
	_hostname.reset();

	Info(_log, "mDNS info service stopped");

	emit isStopped();
}

void MdnsProvider::publishService(const QString& serviceType, quint16 servicePort, const QByteArray& serviceName)
{
	QByteArray type = MdnsServiceRegister::getServiceType(serviceType);

	if (type.isEmpty())
	{
		Error(_log, "Not able to publish mDNS serviceType [%s], invalid type",  serviceType.constData());
		return;
	}

	qCDebug(mdns_provider) << "Publish new mDNS serviceType:" << type;

	if (!_providedServiceTypes.contains(type))
	{
		QSharedPointer<QMdnsEngine::Provider> newProvider = QSharedPointer<QMdnsEngine::Provider>::create(_server.data(), _hostname.data());
		_providedServiceTypes.insert(type, newProvider);
	}

	QSharedPointer<QMdnsEngine::Provider> provider = _providedServiceTypes.value(type);
	if (provider.isNull())
	{
		Error(_log, "Not able to get hold of mDNS serviceType [%s]",  type.constData());
		return;
	}

	QMdnsEngine::Service service;
	service.setType(type);
	service.setPort(servicePort);

	QByteArray name(QHostInfo::localHostName().toUtf8());
	if (!serviceName.isEmpty())
	{
		name.prepend(serviceName + "@");
	}
	service.setName(name);

	QByteArray uuid;
	if (auto auth = AuthManager::getInstanceWeak().toStrongRef())
	{
		uuid = auth->getID().toUtf8();
	}
	const QMap<QByteArray, QByteArray> attributes = {{"id", uuid}, {"version", HYPERION_VERSION}};
	service.setAttributes(attributes);

	qCDebug(mdns_provider) << "Successfully published mDNS serviceType:" << service.type() << ", name:" << service.name() << ", port:" << service.port();

	provider->update(service);
}

void MdnsProvider::onHostnameChanged(const QByteArray& hostname) const
{
	qCDebug(mdns_provider) << "Updated mDNS hostname to:" << hostname;
}
