#include <mdns/MdnsProvider.h>
#include <mdns/MdnsServiceRegister.h>

// Qt includes
#include <QHostInfo>
#include <QThread>

// Utility includes
#include <utils/Logger.h>
#include <HyperionConfig.h>
#include <hyperion/AuthManager.h>

namespace {
	const bool verboseProvider = false;
} // End of constants

MdnsProvider::MdnsProvider(QObject* parent)
	: QObject(parent)
	, _log(Logger::getInstance("MDNS"))
	, _server(nullptr)
	, _hostname(nullptr)
{
}

void MdnsProvider::init()
{
	_server.reset(new QMdnsEngine::Server());
	_hostname.reset(new QMdnsEngine::Hostname(_server.data()));

	connect(_hostname.data(), &QMdnsEngine::Hostname::hostnameChanged, this, &MdnsProvider::onHostnameChanged);
	DebugIf(verboseProvider, _log, "Hostname [%s], isRegistered [%d]", _hostname->hostname().constData(), _hostname->isRegistered());
}

MdnsProvider::~MdnsProvider()
{
	_providedServiceTypes.clear();
}

void MdnsProvider::publishService(const QString& serviceType, quint16 servicePort, const QByteArray& serviceName)
{
	QByteArray type = MdnsServiceRegister::getServiceType(serviceType);

	if (!type.isEmpty())
	{
		DebugIf(verboseProvider, _log, "Publish new mDNS serviceType [%s], Thread: %s", type.constData(), QSTRING_CSTR(QThread::currentThread()->objectName()));

		if (!_providedServiceTypes.contains(type))
		{
			QSharedPointer<QMdnsEngine::Provider> newProvider = QSharedPointer<QMdnsEngine::Provider>::create(_server.data(), _hostname.data());
			_providedServiceTypes.insert(type, newProvider);

		}

		QSharedPointer<QMdnsEngine::Provider> provider = _providedServiceTypes.value(type);
		if (!provider.isNull())
		{
			QMdnsEngine::Service service;
			service.setType(type);
			service.setPort(servicePort);

			QByteArray name(QHostInfo::localHostName().toUtf8());
			if (!serviceName.isEmpty())
			{
				name.prepend(serviceName + "@");
			}
			service.setName(name);

			QByteArray uuid = AuthManager::getInstance()->getID().toUtf8();
			const QMap<QByteArray, QByteArray> attributes = {{"id", uuid}, {"version", HYPERION_VERSION}};
			service.setAttributes(attributes);

			DebugIf(verboseProvider, _log, "[%s], Name: [%s], Port: [%u] ", service.type().constData(), service.name().constData(), service.port());

			provider->update(service);
		}
		else
		{
			Error(_log, "Not able to get hold of mDNS serviceType [%s]",  type.constData());
		}
	}
}

void MdnsProvider::onHostnameChanged(const QByteArray& hostname)
{
	DebugIf(verboseProvider, _log, "mDNS-hostname changed to hostname [%s]", hostname.constData());
}
