#include <utils/NetOrigin.h>

#include <QJsonObject>
#include <QNetworkInterface>

NetOrigin* NetOrigin::instance = nullptr;

NetOrigin::NetOrigin(QObject* parent, Logger* log)
	: QObject(parent)
	, _log(log)
{
	NetOrigin::instance = this;
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

