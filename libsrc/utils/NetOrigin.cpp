#include <utils/NetOrigin.h>
#include <utils/MemoryTracker.h>

#include <QJsonObject>
#include <QNetworkInterface>

QSharedPointer<NetOrigin> NetOrigin::_instance;

NetOrigin::NetOrigin(QObject* parent, QSharedPointer<Logger> log)
	: QObject(parent)
	, _log(log)
{}

void NetOrigin::createInstance(QObject* parent)
{
	CREATE_INSTANCE_WITH_TRACKING(_instance, NetOrigin, parent, nullptr);
}

QSharedPointer<NetOrigin> NetOrigin::getInstance()
{
	return _instance;
}

bool NetOrigin::isValid()
{
	return !_instance.isNull();
}

void NetOrigin::destroyInstance()
{
	_instance.reset();
}


bool NetOrigin::isLocalAddress(const QHostAddress& ipAddress, const QHostAddress& /*local*/) const
{
	QHostAddress address = ipAddress;

	if (address.isLoopback() || address.isLinkLocal())
	{
		return true;
	}

	// Convert IPv4-mapped IPv6 to pure IPv4
	QHostAddress const ipv4Address(address.toIPv4Address());
	if (ipv4Address != QHostAddress::AnyIPv4)
	{
		address = ipv4Address;
	}

	QList<QNetworkInterface> const allInterfaces = QNetworkInterface::allInterfaces();
	for (const QNetworkInterface &networkInterface : allInterfaces)
	{
		QList<QNetworkAddressEntry> const addressEntries = networkInterface.addressEntries();
		for (const QNetworkAddressEntry &localNetworkAddressEntry : addressEntries)
		{
			QHostAddress const localIP = localNetworkAddressEntry.ip();

			// Skip protocol mismatch
			if (localIP.protocol() != address.protocol())
			{
				continue;
			}

			if (address.isInSubnet(localIP, localNetworkAddressEntry.prefixLength()))
			{
				return true;
			}
		}
	}

	return false;
}

