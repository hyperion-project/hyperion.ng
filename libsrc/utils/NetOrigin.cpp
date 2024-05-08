#include <utils/NetOrigin.h>

#include <QJsonObject>
#include <QNetworkInterface>

NetOrigin* NetOrigin::instance = nullptr;

NetOrigin::NetOrigin(QObject* parent, Logger* log)
	: QObject(parent)
	, _log(log)
	, _isInternetAccessAllowed(false)
	, _isInternetAccessRestricted(false)
	, _ipWhitelist()
{
	NetOrigin::instance = this;
}

bool NetOrigin::accessAllowed(const QHostAddress& address, const QHostAddress& local) const
{
	bool isAllowed {false};

	if(isLocalAddress(address, local))
	{
		isAllowed = true;
	}
	else
	{
		if(_isInternetAccessAllowed)
		{
			if (!_isInternetAccessRestricted)
			{
				isAllowed = true;
			}
			else
			{
				for (const QHostAddress &listAddress : _ipWhitelist)
				{
					if (address.isEqual(listAddress))
					{
						isAllowed = true;
						break;
					}
				}
				WarningIf(!isAllowed, _log,"Client connection from IP address '%s' has been rejected! It's not whitelisted.",QSTRING_CSTR(address.toString()));
			}
		}
	}
	return isAllowed;
}


bool NetOrigin::isLocalAddress(const QHostAddress& ipAddress, const QHostAddress& /*local*/) const
{
	QHostAddress address = ipAddress;

	if (address.isLoopback() || address.isLinkLocal())
	{
		return true;
	}

	//Convert to IPv4 to check, if an IPv6 address is an IPv4 mapped address
	QHostAddress ipv4Address(address.toIPv4Address());
	if (ipv4Address != QHostAddress::AnyIPv4) // ipv4Address is not "0.0.0.0"
	{
		address = ipv4Address;
	}

	QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();
	for (const QNetworkInterface &networkInterface : allInterfaces) {
		QList<QNetworkAddressEntry> addressEntries = networkInterface.addressEntries();
		for (const QNetworkAddressEntry &localNetworkAddressEntry : addressEntries) {
			QHostAddress localIP = localNetworkAddressEntry.ip();

			if(localIP.protocol() != QAbstractSocket::NetworkLayerProtocol::IPv4Protocol)
			{
				continue;
			}

			bool isInSubnet = address.isInSubnet(localIP, localNetworkAddressEntry.prefixLength());
			if (isInSubnet)
			{
				return true;
			}
		}
	}
	return false;
}

void NetOrigin::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type == settings::NETWORK)
	{
		const QJsonObject& obj = config.object();
		_isInternetAccessAllowed = obj["internetAccessAPI"].toBool(false);
		_isInternetAccessRestricted = obj["restirctedInternetAccessAPI"].toBool(false);
		const QJsonArray arr = obj["ipWhitelist"].toArray();

        _ipWhitelist.clear();

        for(const auto& item : std::as_const(arr))
		{
			const QString& entry = item.toString("");
			if(entry.isEmpty())
			{
				continue;
			}

			QHostAddress host(entry);
			if(host.isNull())
			{
				Warning(_log,"The whitelisted IP address '%s' isn't valid! Skipped",QSTRING_CSTR(entry));
				continue;
			}
			_ipWhitelist << host;
		}
	}
}
