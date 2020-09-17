#include <utils/NetOrigin.h>

#include <QJsonObject>

NetOrigin* NetOrigin::instance = nullptr;

NetOrigin::NetOrigin(QObject* parent, Logger* log)
	: QObject(parent)
	, _log(log)
	, _internetAccessAllowed(false)
	, _ipWhitelist()
{
	NetOrigin::instance = this;
}

bool NetOrigin::accessAllowed(const QHostAddress& address, const QHostAddress& local) const
{
	if(_internetAccessAllowed)
		return true;

	if(_ipWhitelist.contains(address)) // v4 and v6
		return true;

	if(!isLocalAddress(address, local))
	{
		Warning(_log,"Client connection with IP address '%s' has been rejected! It's not whitelisted, access denied.",QSTRING_CSTR(address.toString()));
		return false;
	}
	return true;
}

bool NetOrigin::isLocalAddress(const QHostAddress& address, const QHostAddress& local) const
{
	if(address.protocol() == QAbstractSocket::IPv4Protocol)
	{
		if(!address.isInSubnet(local, 24)) // 255.255.255.xxx; IPv4 0-32
		{
			return false;
		}
	}
	else if(address.protocol() == QAbstractSocket::IPv6Protocol)
	{
		if(!address.isInSubnet(local, 64)) // 2001:db8:abcd:0012:XXXX:XXXX:XXXX:XXXX; IPv6 0-128
		{
			return false;
		}
	}
	return true;
}

void NetOrigin::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type == settings::NETWORK)
	{
		const QJsonObject& obj = config.object();
		_internetAccessAllowed = obj["internetAccessAPI"].toBool(false);

		const QJsonArray& arr = obj["ipWhitelist"].toArray();
		_ipWhitelist.clear();

		for(const auto& e : arr)
		{
			const QString& entry = e.toString("");
			if(entry.isEmpty())
				continue;

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
