// STL includes
#include <stdexcept>

#include <hyperion/MessageForwarder.h>

#include <utils/Logger.h>
#include <hyperion/Hyperion.h>

MessageForwarder::MessageForwarder(Hyperion* hyperion, const QJsonDocument & config)
	: QObject()
	, _hyperion(hyperion)
	, _log(Logger::getInstance("NETFORWARDER"))
{
	handleSettingsUpdate(settings::NETFORWARD, config);
	// get settings updates
	connect(_hyperion, &Hyperion::settingsChanged, this, &MessageForwarder::handleSettingsUpdate);
}

MessageForwarder::~MessageForwarder()
{
}

void MessageForwarder::handleSettingsUpdate(const settings::type& type, const QJsonDocument& config)
{
	if(type == settings::NETFORWARD)
	{
		// clear the current targets
		_jsonSlaves.clear();
		_protoSlaves.clear();
		// build new one
		const QJsonObject &obj = config.object();
		if ( !obj["json"].isNull() )
		{
			const QJsonArray & addr = obj["json"].toArray();
			for (const auto& entry : addr)
			{
				addJsonSlave(entry.toString());
			}
		}

		if ( !obj["proto"].isNull() )
		{
			const QJsonArray & addr = obj["proto"].toArray();
			for (const auto& entry : addr)
			{
				addProtoSlave(entry.toString());
			}
		}
		InfoIf(obj["enable"].toBool(true), _log, "Forward now to json targets '%s' and proto targets '%s'", QSTRING_CSTR(_jsonSlaves.join(", ")), QSTRING_CSTR(_protoSlaves.join(", ")))
		// update comp state
		_hyperion->getComponentRegister().componentStateChanged(hyperion::COMP_FORWARDER, obj["enable"].toBool(true));
	}
}

void MessageForwarder::addJsonSlave(QString slave)
{
	QStringList parts = slave.split(":");
	if (parts.size() != 2)
	{
		Error(_log, "Unable to parse address (%s)",QSTRING_CSTR(slave));
		return;
	}

	bool ok;
	parts[1].toUShort(&ok);
	if (!ok)
	{
		Error(_log, "Unable to parse port number (%s)",QSTRING_CSTR(parts[1]));
		return;
	}

	// verify loop with jsonserver
	const QJsonObject& obj = _hyperion->getSetting(settings::JSONSERVER).object();
	if(QHostAddress(parts[0]) == QHostAddress::LocalHost && parts[1].toInt() == obj["port"].toInt())
	{
		Error(_log, "Loop between JsonServer and Forwarder! (%s)",QSTRING_CSTR(slave));
		return;
	}

	_jsonSlaves << slave;
}

void MessageForwarder::addProtoSlave(QString slave)
{
	QStringList parts = slave.split(":");
	if (parts.size() != 2)
	{
		Error(_log, "Unable to parse address (%s)",QSTRING_CSTR(slave));
		return;
	}

	bool ok;
	parts[1].toUShort(&ok);
	if (!ok)
	{
		Error(_log, "Unable to parse port number (%s)",QSTRING_CSTR(parts[1]));
		return;
	}

	// verify loop with protoserver
	const QJsonObject& obj = _hyperion->getSetting(settings::PROTOSERVER).object();
	if(QHostAddress(parts[0]) == QHostAddress::LocalHost && parts[1].toInt() == obj["port"].toInt())
	{
		Error(_log, "Loop between ProtoServer and Forwarder! (%s)",QSTRING_CSTR(slave));
		return;
	}
	_protoSlaves << slave;
}

bool MessageForwarder::protoForwardingEnabled()
{
	return ! _protoSlaves.empty();
}

bool MessageForwarder::jsonForwardingEnabled()
{
	return ! _jsonSlaves.empty();
}
