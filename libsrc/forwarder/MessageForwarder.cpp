// STL includes
#include <stdexcept>

// project includes
#include <forwarder/MessageForwarder.h>

// hyperion includes
#include <hyperion/Hyperion.h>

// utils includes
#include <utils/Logger.h>
#include <utils/NetUtils.h>

// qt includes
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostInfo>
#include <QNetworkInterface>

#include <flatbufserver/FlatBufferConnection.h>

MessageForwarder::MessageForwarder(Hyperion* hyperion)
	: _hyperion(hyperion)
	, _log(nullptr)
	, _muxer(_hyperion->getMuxerInstance())
	, _forwarder_enabled(true)
	, _priority(140)
{
	QString subComponent = hyperion->property("instance").toString();
	_log= Logger::getInstance("NETFORWARDER", subComponent);

	// get settings updates
	connect(_hyperion, &Hyperion::settingsChanged, this, &MessageForwarder::handleSettingsUpdate);

	// component changes
	connect(_hyperion, &Hyperion::compStateChangeRequest, this, &MessageForwarder::handleCompStateChangeRequest);

	// connect with Muxer visible priority changes
	connect(_muxer, &PriorityMuxer::visiblePriorityChanged, this, &MessageForwarder::handlePriorityChanges);

	// init
	handleSettingsUpdate(settings::NETFORWARD, _hyperion->getSetting(settings::NETFORWARD));
}

MessageForwarder::~MessageForwarder()
{
	while (!_forwardClients.isEmpty())
	{
		delete _forwardClients.takeFirst();
	}
}

void MessageForwarder::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if (type == settings::NETFORWARD)
	{
		// clear the current targets
		_jsonTargets.clear();
		_flatbufferTargets.clear();
		while (!_forwardClients.isEmpty())
		{
			delete _forwardClients.takeFirst();
		}

		// build new one
		const QJsonObject& obj = config.object();
		if (!obj["jsonapi"].isNull())
		{
			const QJsonArray& addr = obj["jsonapi"].toArray();
			for (const auto& entry : addr)
			{
				addJsonTarget(entry.toObject());
			}
		}

		if (!obj["flatbuffer"].isNull())
		{
			const QJsonArray& addr = obj["flatbuffer"].toArray();
			for (const auto& entry : addr)
			{
				addFlatbufferTarget(entry.toObject());
			}
		}

		bool isForwarderEnabledinSettings = obj["enable"].toBool(false);

		if (!_jsonTargets.isEmpty() && isForwarderEnabledinSettings && _forwarder_enabled)
		{
			for (const auto& targetHost : qAsConst(_jsonTargets))
			{
				InfoIf(isForwarderEnabledinSettings, _log, "Forwarding now to JSON-target host: %s port: %u", QSTRING_CSTR(targetHost.host.toString()), targetHost.port);
			}

			connect(_hyperion, &Hyperion::forwardJsonMessage, this, &MessageForwarder::forwardJsonMessage, Qt::UniqueConnection);
		}
		else if (_jsonTargets.isEmpty() || !isForwarderEnabledinSettings || !_forwarder_enabled)
		{
			disconnect(_hyperion, &Hyperion::forwardJsonMessage, nullptr, nullptr);
		}

		if (!_flatbufferTargets.isEmpty() && isForwarderEnabledinSettings && _forwarder_enabled)
		{
			for (const auto& targetHost : qAsConst(_flatbufferTargets))
			{
				InfoIf(isForwarderEnabledinSettings, _log, "Forwarding now to Flatbuffer-target host: %s port: %u", QSTRING_CSTR(targetHost.host.toString()), targetHost.port);
			}
		}
		else if (_flatbufferTargets.isEmpty() || !isForwarderEnabledinSettings || !_forwarder_enabled)
		{
			disconnect(_hyperion, &Hyperion::forwardSystemProtoMessage, nullptr, nullptr);
			disconnect(_hyperion, &Hyperion::forwardV4lProtoMessage, nullptr, nullptr);
		}

		// update comp state
		_hyperion->setNewComponentState(hyperion::COMP_FORWARDER, isForwarderEnabledinSettings);
	}
}

void MessageForwarder::handleCompStateChangeRequest(hyperion::Components component, bool enable)
{
	if (component == hyperion::COMP_FORWARDER && _forwarder_enabled != enable)
	{
		_forwarder_enabled = enable;
		handleSettingsUpdate(settings::NETFORWARD, _hyperion->getSetting(settings::NETFORWARD));
		Info(_log, "Forwarder change state to %s", (_forwarder_enabled ? "enabled" : "disabled"));
		_hyperion->setNewComponentState(component, _forwarder_enabled);
	}
}

void MessageForwarder::handlePriorityChanges(quint8 priority)
{
	const QJsonObject obj = _hyperion->getSetting(settings::NETFORWARD).object();
	if (priority != 0 && _forwarder_enabled && obj["enable"].toBool())
	{
		hyperion::Components activeCompId = _hyperion->getPriorityInfo(priority).componentId;
		if (activeCompId == hyperion::COMP_GRABBER || activeCompId == hyperion::COMP_V4L)
		{
			if (!obj["flatbuffer"].isNull())
			{
				const QJsonArray& addr = obj["flatbuffer"].toArray();
				for (const auto& entry : addr)
				{
					addFlatbufferTarget(entry.toObject());
				}
			}

			switch (activeCompId)
			{
			case hyperion::COMP_GRABBER:
			{
				disconnect(_hyperion, &Hyperion::forwardV4lProtoMessage, nullptr, nullptr);
				connect(_hyperion, &Hyperion::forwardSystemProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage, Qt::UniqueConnection);
			}
			break;
			case hyperion::COMP_V4L:
			{
				disconnect(_hyperion, &Hyperion::forwardSystemProtoMessage, nullptr, nullptr);
				connect(_hyperion, &Hyperion::forwardV4lProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage, Qt::UniqueConnection);
			}
			break;
			default:
			{
				disconnect(_hyperion, &Hyperion::forwardSystemProtoMessage, nullptr, nullptr);
				disconnect(_hyperion, &Hyperion::forwardV4lProtoMessage, nullptr, nullptr);
			}
			}
		}
		else
		{
			disconnect(_hyperion, &Hyperion::forwardSystemProtoMessage, nullptr, nullptr);
			disconnect(_hyperion, &Hyperion::forwardV4lProtoMessage, nullptr, nullptr);
		}
	}
}

void MessageForwarder::addJsonTarget(const QJsonObject& targetConfig)
{
	TargetHost targetHost;

	QString hostName = targetConfig["host"].toString();
	int port = targetConfig["port"].toInt();

#if 0
//#ifdef ENABLE_MDNS
		if (hostName.endsWith("._tcp.local"))
		{
			Debug(_log, "Service      : %s", QSTRING_CSTR(hostName));
			//Treat hostname as service instance name that requires to be resolved into an mDNS-Hostname first
			QMdnsEngine::Record service = MdnsBrowser::getInstance().getServiceInstanceRecord(hostName.toUtf8());

			if (!service.target().isEmpty())
			{
				hostName = service.target();
				port = service.port();
			}
			else
			{
				Error(_log, "Cannot resolve host for given service [%s]!", QSTRING_CSTR(hostName));
			}
		}
#endif

		if (!hostName.isEmpty())
		{
			if (NetUtils::resolveHostAddress(_hyperion->parent(), _log, hostName, targetHost.host))
			{

				if (NetUtils::isValidPort(_log, port, targetHost.host.toString()))
				{
					targetHost.port = static_cast<quint16>(port);

					// verify loop with JSON-server
					const QJsonObject& obj = _hyperion->getSetting(settings::JSONSERVER).object();
					if ((QNetworkInterface::allAddresses().indexOf(targetHost.host) != -1) && targetHost.port == static_cast<quint16>(obj["port"].toInt()))
					{
						Error(_log, "Loop between JSON-Server and Forwarder! Configuration for host: %s, port: %d is ignored.", QSTRING_CSTR(targetHost.host.toString()), port);
					}
					else
					{
						if (_forwarder_enabled)
						{
							if (_jsonTargets.indexOf(targetHost) == -1)
							{
								Info(_log, "JSON-Forwarder settings: Adding target host: %s port: %u", QSTRING_CSTR(targetHost.host.toString()), targetHost.port);
								_jsonTargets << targetHost;
							}
							else
							{
								Warning(_log, "JSON Forwarder settings: Duplicate target host configuration! Configuration for host: %s, port: %d is ignored.", QSTRING_CSTR(targetHost.host.toString()), targetHost.port);
							}
						}

					}
				}
			}
		}
}

void MessageForwarder::addFlatbufferTarget(const QJsonObject& targetConfig)
{
	TargetHost targetHost;

	QString hostName = targetConfig["host"].toString();
	int port = targetConfig["port"].toInt();

#if 0
//#ifdef ENABLE_MDNS
	if (hostName.endsWith("._tcp.local"))
	{
		Debug(_log, "Service      : %s", QSTRING_CSTR(hostName));
		//Treat hostname as service instance name that requires to be resolved into an mDNS-Hostname first
		QMdnsEngine::Record service = MdnsBrowser::getInstance().getServiceInstanceRecord(hostName.toUtf8());

		if (!service.target().isEmpty())
		{
			hostName = service.target();
			port = service.port();
		}
		else
		{
			Error(_log, "Cannot resolve host for given service [%s]!", QSTRING_CSTR(hostName));
		}
	}
#endif

	if (!hostName.isEmpty())
	{
		if (NetUtils::resolveHostAddress(&MdnsBrowser::getInstance(), _log, hostName, targetHost.host))
		{

			if (NetUtils::isValidPort(_log, port, targetHost.host.toString()))
			{
				targetHost.port = static_cast<quint16>(port);

				// verify loop with Flatbuffer-server
				const QJsonObject& obj = _hyperion->getSetting(settings::FLATBUFSERVER).object();
				if ((QNetworkInterface::allAddresses().indexOf(targetHost.host) != -1) && targetHost.port == static_cast<quint16>(obj["port"].toInt()))
				{
					Error(_log, "Loop between Flatbuffer-Server and Forwarder! Configuration for host: %s, port: %d is ignored.", QSTRING_CSTR(targetHost.host.toString()), port);
				}
				else
				{
					if (_forwarder_enabled)
					{
						if (_flatbufferTargets.indexOf(targetHost) == -1)
						{
							Info(_log, "Flatbuffer-Forwarder settings: Adding target host: %s port: %u", QSTRING_CSTR(targetHost.host.toString()), targetHost.port);
							_flatbufferTargets << targetHost;

							FlatBufferConnection* flatbuf = new FlatBufferConnection("Forwarder", targetHost.host.toString(), _priority, false, targetHost.port);
							_forwardClients << flatbuf;
						}
						else
						{
							Warning(_log, "Flatbuffer Forwarder settings: Duplicate target host configuration! Configuration for host: %s, port: %d is ignored.", QSTRING_CSTR(targetHost.host.toString()), targetHost.port);
						}
					}
				}
			}
		}
	}
}

void MessageForwarder::forwardJsonMessage(const QJsonObject& message)
{
	if (_forwarder_enabled)
	{
		QTcpSocket client;
		for (const auto& targetHost : qAsConst(_jsonTargets))
		{
			client.connectToHost(targetHost.host, targetHost.port);
			if (client.waitForConnected(500))
			{
				sendJsonMessage(message, &client);
				client.close();
			}
		}
	}
}

void MessageForwarder::forwardFlatbufferMessage(const QString& /*name*/, const Image<ColorRgb>& image)
{
	if (_forwarder_enabled)
	{
		for (int i = 0; i < _forwardClients.size(); i++)
		{
			_forwardClients.at(i)->setImage(image);
		}
	}
}

void MessageForwarder::sendJsonMessage(const QJsonObject& message, QTcpSocket* socket)
{
	// for hyperion classic compatibility
	QJsonObject jsonMessage = message;
	if (jsonMessage.contains("tan") && jsonMessage["tan"].isNull())
	{
		jsonMessage["tan"] = 100;
	}

	// serialize message
	QJsonDocument writer(jsonMessage);
	QByteArray serializedMessage = writer.toJson(QJsonDocument::Compact) + "\n";

	// write message
	socket->write(serializedMessage);
	if (!socket->waitForBytesWritten())
	{
		Debug(_log, "Error while writing data to host");
		return;
	}

	// read reply data
	QByteArray serializedReply;
	while (!serializedReply.contains('\n'))
	{
		// receive reply
		if (!socket->waitForReadyRead())
		{
			Debug(_log, "Error while writing data from host");
			return;
		}

		serializedReply += socket->readAll();
	}

	// parse reply data
	QJsonParseError error;
	QJsonDocument::fromJson(serializedReply, &error);

	if (error.error != QJsonParseError::NoError)
	{
		Error(_log, "Error while parsing reply: invalid json");
		return;
	}
}

