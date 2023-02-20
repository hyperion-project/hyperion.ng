// STL includes
#include <chrono>

// project includes
#include <forwarder/MessageForwarder.h>

// hyperion includes
#include <hyperion/Hyperion.h>

// utils includes
#include <utils/Logger.h>
#include <utils/NetUtils.h>

// qt includes
#include <QTcpSocket>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QThread>

#include <flatbufserver/FlatBufferConnection.h>

// mDNS discover
#ifdef ENABLE_MDNS
#include <mdns/MdnsBrowser.h>
#include <mdns/MdnsServiceRegister.h>
#endif

// Constants
namespace {

const int DEFAULT_FORWARDER_FLATBUFFFER_PRIORITY = 140;

constexpr std::chrono::milliseconds CONNECT_TIMEOUT{500};		 // JSON-socket connect timeout in ms

} //End of constants

MessageForwarder::MessageForwarder(Hyperion* hyperion)
	: _hyperion(hyperion)
	  , _log(nullptr)
	  , _muxer(_hyperion->getMuxerInstance())
	  , _forwarder_enabled(false)
	  , _priority(DEFAULT_FORWARDER_FLATBUFFFER_PRIORITY)
	  , _messageForwarderFlatBufHelper(nullptr)
{
	QString subComponent = hyperion->property("instance").toString();
	_log= Logger::getInstance("NETFORWARDER", subComponent);

	qRegisterMetaType<TargetHost>("TargetHost");

#ifdef ENABLE_MDNS
	QMetaObject::invokeMethod(&MdnsBrowser::getInstance(), "browseForServiceType",
							   Qt::QueuedConnection, Q_ARG(QByteArray, MdnsServiceRegister::getServiceType("jsonapi")));
#endif

	// get settings updates
	connect(_hyperion, &Hyperion::settingsChanged, this, &MessageForwarder::handleSettingsUpdate);

	// component changes
	connect(_hyperion, &Hyperion::compStateChangeRequest, this, &MessageForwarder::handleCompStateChangeRequest);

	// connect with Muxer visible priority changes
	connect(_muxer, &PriorityMuxer::visiblePriorityChanged, this, &MessageForwarder::handlePriorityChanges);
}

MessageForwarder::~MessageForwarder()
{
	stopJsonTargets();
	stopFlatbufferTargets();
}


void MessageForwarder::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if (type == settings::NETFORWARD)
	{
		const QJsonObject& obj = config.object();

		bool isForwarderEnabledinSettings = obj["enable"].toBool(false);
		enableTargets(isForwarderEnabledinSettings, obj);
	}
}

void MessageForwarder::handleCompStateChangeRequest(hyperion::Components component, bool enable)
{
	if (component == hyperion::COMP_FORWARDER && _forwarder_enabled != enable)
	{
		Info(_log, "Forwarder is %s", (enable ? "enabled" : "disabled"));
		QJsonDocument config {_hyperion->getSetting(settings::type::NETFORWARD)};
		enableTargets(enable, config.object());
	}
}

void MessageForwarder::enableTargets(bool enable, const QJsonObject& config)
{
	if (!enable)
	{
		_forwarder_enabled = false;
		stopJsonTargets();
		stopFlatbufferTargets();

	}
	else
	{
		int jsonTargetNum = startJsonTargets(config);
		int flatbufTargetNum = startFlatbufferTargets(config);

		if (flatbufTargetNum > 0)
		{
			hyperion::Components activeCompId = _hyperion->getPriorityInfo(_hyperion->getCurrentPriority()).componentId;

			switch (activeCompId) {
			case hyperion::COMP_GRABBER:
				connect(_hyperion, &Hyperion::forwardSystemProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage, Qt::UniqueConnection);
				break;
			case hyperion::COMP_V4L:
				connect(_hyperion, &Hyperion::forwardV4lProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage, Qt::UniqueConnection);
				break;
			case hyperion::COMP_AUDIO:
				connect(_hyperion, &Hyperion::forwardAudioProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage, Qt::UniqueConnection);
				break;
#if defined(ENABLE_FLATBUF_SERVER)
			case hyperion::COMP_FLATBUFSERVER:
#endif
#if defined(ENABLE_PROTOBUF_SERVER)
			case hyperion::COMP_PROTOSERVER:
#endif
#if defined(ENABLE_FLATBUF_SERVER) || defined(ENABLE_PROTOBUF_SERVER)

				connect(_hyperion, &Hyperion::forwardBufferMessage, this, &MessageForwarder::forwardFlatbufferMessage, Qt::UniqueConnection);
				break;
#endif
			default:
				break;
			}
		}

		if (jsonTargetNum > 0 || flatbufTargetNum > 0)
		{
			_forwarder_enabled = true;
		}
		else
		{
			_forwarder_enabled = false;
			Warning(_log,"No JSON- nor Flatbuffer-Forwarder configured -> Forwarding disabled");
		}
	}
	_hyperion->setNewComponentState(hyperion::COMP_FORWARDER, _forwarder_enabled);
}

void MessageForwarder::handlePriorityChanges(int priority)
{
	if (priority != 0 && _forwarder_enabled)
	{
		hyperion::Components activeCompId = _hyperion->getPriorityInfo(priority).componentId;

		switch (activeCompId) {
		case hyperion::COMP_GRABBER:
			disconnect(_hyperion, &Hyperion::forwardV4lProtoMessage, nullptr, nullptr);
			disconnect(_hyperion, &Hyperion::forwardAudioProtoMessage, nullptr, nullptr);
#if defined(ENABLE_FLATBUF_SERVER) || defined(ENABLE_PROTOBUF_SERVER)
			disconnect(_hyperion, &Hyperion::forwardBufferMessage, nullptr, nullptr);
#endif
			connect(_hyperion, &Hyperion::forwardSystemProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage, Qt::UniqueConnection);
			break;
		case hyperion::COMP_V4L:
			disconnect(_hyperion, &Hyperion::forwardSystemProtoMessage, nullptr, nullptr);
			disconnect(_hyperion, &Hyperion::forwardAudioProtoMessage, nullptr, nullptr);
#if defined(ENABLE_FLATBUF_SERVER) || defined(ENABLE_PROTOBUF_SERVER)
			disconnect(_hyperion, &Hyperion::forwardBufferMessage, nullptr, nullptr);
#endif
			connect(_hyperion, &Hyperion::forwardV4lProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage, Qt::UniqueConnection);
			break;
		case hyperion::COMP_AUDIO:
			disconnect(_hyperion, &Hyperion::forwardSystemProtoMessage, nullptr, nullptr);
			disconnect(_hyperion, &Hyperion::forwardV4lProtoMessage, nullptr, nullptr);
#if defined(ENABLE_FLATBUF_SERVER) || defined(ENABLE_PROTOBUF_SERVER)
			disconnect(_hyperion, &Hyperion::forwardBufferMessage, nullptr, nullptr);
#endif
			connect(_hyperion, &Hyperion::forwardAudioProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage, Qt::UniqueConnection);
			break;
#if defined(ENABLE_FLATBUF_SERVER)
		case hyperion::COMP_FLATBUFSERVER:
#endif
#if defined(ENABLE_PROTOBUF_SERVER)
		case hyperion::COMP_PROTOSERVER:
#endif
#if defined(ENABLE_FLATBUF_SERVER) || defined(ENABLE_PROTOBUF_SERVER)
			disconnect(_hyperion, &Hyperion::forwardAudioProtoMessage, nullptr, nullptr);
			disconnect(_hyperion, &Hyperion::forwardSystemProtoMessage, nullptr, nullptr);
			disconnect(_hyperion, &Hyperion::forwardV4lProtoMessage, nullptr, nullptr);
			connect(_hyperion, &Hyperion::forwardBufferMessage, this, &MessageForwarder::forwardFlatbufferMessage, Qt::UniqueConnection);
			break;
#endif
		default:
			disconnect(_hyperion, &Hyperion::forwardSystemProtoMessage, nullptr, nullptr);
			disconnect(_hyperion, &Hyperion::forwardV4lProtoMessage, nullptr, nullptr);
			disconnect(_hyperion, &Hyperion::forwardAudioProtoMessage, nullptr, nullptr);
#if defined(ENABLE_FLATBUF_SERVER) || defined(ENABLE_PROTOBUF_SERVER)
			disconnect(_hyperion, &Hyperion::forwardBufferMessage, nullptr, nullptr);
#endif
			break;
		}
	}
}

void MessageForwarder::addJsonTarget(const QJsonObject& targetConfig)
{
	TargetHost targetHost;

	QString hostName = targetConfig["host"].toString();
	int port = targetConfig["port"].toInt();

	if (!hostName.isEmpty())
	{
		if (NetUtils::resolveHostToAddress(_log, hostName, targetHost.host, port))
		{
			QString address = targetHost.host.toString();
			if (hostName != address)
			{
				Info(_log, "Resolved hostname [%s] to address [%s]",  QSTRING_CSTR(hostName), QSTRING_CSTR(address));
			}

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
					if (_jsonTargets.indexOf(targetHost) == -1)
					{
						Debug(_log, "JSON-Forwarder settings: Adding target host: %s port: %u", QSTRING_CSTR(targetHost.host.toString()), targetHost.port);
						_jsonTargets << targetHost;
					}
					else
					{
						Warning(_log, "JSON-Forwarder settings: Duplicate target host configuration! Configuration for host: %s, port: %d is ignored.", QSTRING_CSTR(targetHost.host.toString()), targetHost.port);
					}
				}
			}
		}
	}
}

int MessageForwarder::startJsonTargets(const QJsonObject& config)
{
	if (!config["jsonapi"].isNull())
	{
		_jsonTargets.clear();
		const QJsonArray& addr = config["jsonapi"].toArray();

#ifdef ENABLE_MDNS
		if (!addr.isEmpty())
		{
			QMetaObject::invokeMethod(&MdnsBrowser::getInstance(), "browseForServiceType",
									   Qt::QueuedConnection, Q_ARG(QByteArray, MdnsServiceRegister::getServiceType("jsonapi")));
		}
#endif

		for (const auto& entry : addr)
		{
			addJsonTarget(entry.toObject());
		}

		if (!_jsonTargets.isEmpty())
		{
			for (const auto& targetHost : qAsConst(_jsonTargets))
			{
				Info(_log, "Forwarding now to JSON-target host: %s port: %u", QSTRING_CSTR(targetHost.host.toString()), targetHost.port);
			}

			connect(_hyperion, &Hyperion::forwardJsonMessage, this, &MessageForwarder::forwardJsonMessage, Qt::UniqueConnection);
		}
	}
	return _jsonTargets.size();
}


void MessageForwarder::stopJsonTargets()
{
	if (!_jsonTargets.isEmpty())
	{
		disconnect(_hyperion, &Hyperion::forwardJsonMessage, nullptr, nullptr);
		for (const auto& targetHost : qAsConst(_jsonTargets))
		{
			Info(_log, "Stopped forwarding to JSON-target host: %s port: %u", QSTRING_CSTR(targetHost.host.toString()), targetHost.port);
		}
		_jsonTargets.clear();
	}
}

void MessageForwarder::addFlatbufferTarget(const QJsonObject& targetConfig)
{
	TargetHost targetHost;

	QString hostName = targetConfig["host"].toString();
	int port = targetConfig["port"].toInt();

	if (!hostName.isEmpty())
	{
		if (NetUtils::resolveHostToAddress(_log, hostName, targetHost.host, port))
		{
			QString address = targetHost.host.toString();
			if (hostName != address)
			{
				Info(_log, "Resolved hostname [%s] to address [%s]",  QSTRING_CSTR(hostName), QSTRING_CSTR(address));
			}

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
					if (_flatbufferTargets.indexOf(targetHost) == -1)
					{
						Debug(_log, "Flatbuffer-Forwarder settings: Adding target host: %s port: %u", QSTRING_CSTR(targetHost.host.toString()), targetHost.port);
						_flatbufferTargets << targetHost;

						if (_messageForwarderFlatBufHelper != nullptr)
						{
							emit _messageForwarderFlatBufHelper->addClient("Forwarder", targetHost, _priority, false);
						}
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

int MessageForwarder::startFlatbufferTargets(const QJsonObject& config)
{
	if (!config["flatbuffer"].isNull())
	{
		if (_messageForwarderFlatBufHelper == nullptr)
		{
			_messageForwarderFlatBufHelper = new MessageForwarderFlatbufferClientsHelper();
		}
		else
		{
			emit _messageForwarderFlatBufHelper->clearClients();
		}
		_flatbufferTargets.clear();

		const QJsonArray& addr = config["flatbuffer"].toArray();

#ifdef ENABLE_MDNS
		if (!addr.isEmpty())
		{
			QMetaObject::invokeMethod(&MdnsBrowser::getInstance(), "browseForServiceType",
									   Qt::QueuedConnection, Q_ARG(QByteArray, MdnsServiceRegister::getServiceType("flatbuffer")));
		}
#endif
		for (const auto& entry : addr)
		{
			addFlatbufferTarget(entry.toObject());
		}

		if (!_flatbufferTargets.isEmpty())
		{
			for (const auto& targetHost : qAsConst(_flatbufferTargets))
			{
				Info(_log, "Forwarding now to Flatbuffer-target host: %s port: %u", QSTRING_CSTR(targetHost.host.toString()), targetHost.port);
			}
		}
	}
	return _flatbufferTargets.size();
}

void MessageForwarder::stopFlatbufferTargets()
{
	if (!_flatbufferTargets.isEmpty())
	{
		disconnect(_hyperion, &Hyperion::forwardSystemProtoMessage, nullptr, nullptr);
		disconnect(_hyperion, &Hyperion::forwardV4lProtoMessage, nullptr, nullptr);
		disconnect(_hyperion, &Hyperion::forwardAudioProtoMessage, nullptr, nullptr);
#if defined(ENABLE_FLATBUF_SERVER) || defined(ENABLE_PROTOBUF_SERVER)
		disconnect(_hyperion, &Hyperion::forwardBufferMessage, nullptr, nullptr);
#endif

		if (_messageForwarderFlatBufHelper != nullptr)
		{
			delete _messageForwarderFlatBufHelper;
			_messageForwarderFlatBufHelper = nullptr;
		}

		for (const auto& targetHost : qAsConst(_flatbufferTargets))
		{
			Info(_log, "Stopped forwarding to Flatbuffer-target host: %s port: %u", QSTRING_CSTR(targetHost.host.toString()), targetHost.port);
		}
		_flatbufferTargets.clear();
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
			if (client.waitForConnected(CONNECT_TIMEOUT.count()))
			{
				sendJsonMessage(message, &client);
				client.close();
			}
		}
	}
}

void MessageForwarder::forwardFlatbufferMessage(const QString& /*name*/, const Image<ColorRgb>& image)
{
	if (_messageForwarderFlatBufHelper != nullptr)
	{
		bool isfree = _messageForwarderFlatBufHelper->isFree();

		if (isfree && _forwarder_enabled)
		{
			QMetaObject::invokeMethod(_messageForwarderFlatBufHelper, "forwardImage", Qt::QueuedConnection, Q_ARG(Image<ColorRgb>, image));
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
	/* QJsonDocument reply = */ QJsonDocument::fromJson(serializedReply, &error);

	if (error.error != QJsonParseError::NoError)
	{
		Error(_log, "Error while parsing reply: invalid JSON");
		return;
	}
}

MessageForwarderFlatbufferClientsHelper::MessageForwarderFlatbufferClientsHelper()
{
	QThread* mainThread = new QThread();
	mainThread->setObjectName("ForwarderHelperThread");
	this->moveToThread(mainThread);
	mainThread->start();

	_free = true;
	connect(this, &MessageForwarderFlatbufferClientsHelper::addClient, this, &MessageForwarderFlatbufferClientsHelper::addClientHandler);
	connect(this, &MessageForwarderFlatbufferClientsHelper::clearClients, this, &MessageForwarderFlatbufferClientsHelper::clearClientsHandler);
}

MessageForwarderFlatbufferClientsHelper::~MessageForwarderFlatbufferClientsHelper()
{
	_free=false;
	while (!_forwardClients.isEmpty())
	{
		_forwardClients.takeFirst()->deleteLater();
	}


	QThread* oldThread = this->thread();
	disconnect(oldThread, nullptr, nullptr, nullptr);
	oldThread->quit();
	oldThread->wait();
	delete oldThread;
}

void MessageForwarderFlatbufferClientsHelper::addClientHandler(const QString& origin, const TargetHost& targetHost, int priority, bool skipReply)
{
	FlatBufferConnection* flatbuf = new FlatBufferConnection(origin, targetHost.host.toString(), priority, skipReply, targetHost.port);
	_forwardClients << flatbuf;
	_free = true;
}

void MessageForwarderFlatbufferClientsHelper::clearClientsHandler()
{
	while (!_forwardClients.isEmpty())
	{
		delete _forwardClients.takeFirst();
	}
	_free = false;
}

bool MessageForwarderFlatbufferClientsHelper::isFree() const
{
	return _free;
}

void MessageForwarderFlatbufferClientsHelper::forwardImage(const Image<ColorRgb>& image)
{
	_free = false;

	for (int i = 0; i < _forwardClients.size(); i++)
	{
		_forwardClients.at(i)->setImage(image);
	}

	_free = true;
}
