// STL includes
#include "hyperion/HyperionIManager.h"
#include <chrono>

// project includes
#include <forwarder/MessageForwarder.h>

// hyperion includes
#include <hyperion/Hyperion.h>

// utils includes
#include <utils/Logger.h>
#include <utils/GlobalSignals.h>
#include <utils/NetUtils.h>
#include <utils/JsonUtils.h>

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
 
Q_LOGGING_CATEGORY(forwarder_write, "forwarder.write");

// Constants
namespace {
	const int DEFAULT_FORWARDER_FLATBUFFFER_PRIORITY = 140;
	constexpr std::chrono::milliseconds JSON_SOCKET_CONNECT_TIMEOUT{ 500 };

} //End of constants

MessageForwarder::MessageForwarder(const QJsonDocument& config)
	:
	_log(Logger::getInstance("NETFORWARDER"))
	, _config(config)
	, _isActive(false)
	, _priority(DEFAULT_FORWARDER_FLATBUFFFER_PRIORITY)
	, _isEnabled(false)
	, _toBeForwardedInstanceID(NO_INSTANCE_ID)
	, _hyperionWeak(nullptr)
	, _muxerWeak(nullptr)
	, _messageForwarderFlatBufHelper(nullptr)
{
	TRACK_SCOPE();
	qRegisterMetaType<TargetHost>("TargetHost");
}

MessageForwarder::~MessageForwarder()
{
	TRACK_SCOPE();
}

void MessageForwarder::init()
{
#ifdef ENABLE_MDNS
	{
		auto mdns = _mdnsBrowser.toStrongRef();
		if (!mdns.isNull())
		{
			QMetaObject::invokeMethod(mdns.get(), "browseForServiceType",
				Qt::QueuedConnection, Q_ARG(QByteArray, MdnsServiceRegister::getServiceType("jsonapi")));
		}
	}
#endif

	handleSettingsUpdate(settings::NETFORWARD, _config);
}

void MessageForwarder::start()
{
	if (_isEnabled)
	{
		handleTargets(true, _config.object());
	}
}

void MessageForwarder::stop()
{
	QSharedPointer<Hyperion> const hyperion = _hyperionWeak.toStrongRef();
	if (!hyperion.isNull())
	{
		disconnectFromInstance(_toBeForwardedInstanceID);

		Info(_log, "Forwarding service stopped");
	}
}

bool MessageForwarder::connectToInstance(quint8 instanceID)
{
	bool isConnected{false};
	if (instanceID == _toBeForwardedInstanceID)
	{
		if (auto mgr = HyperionIManager::getInstanceWeak().toStrongRef())
		{
			if (mgr->isInstanceRunning(_toBeForwardedInstanceID))
			{
				Info(_log, "Connect forwarder to instance [%u]", _toBeForwardedInstanceID);
				QSharedPointer<Hyperion> const hyperion = mgr->getHyperionInstance(_toBeForwardedInstanceID);
				if (hyperion)
				{
					_hyperionWeak = hyperion;
					_muxerWeak = hyperion->getMuxerInstance();

					// component changes
					QObject::connect(hyperion.get(), &Hyperion::compStateChangeRequest, this, &MessageForwarder::handleCompStateChangeRequest);

					// connect with Muxer visible priority changes
					QObject::connect(_muxerWeak.toStrongRef().get(), &PriorityMuxer::visiblePriorityChanged, this, &MessageForwarder::handlePriorityChanges);
#if defined(ENABLE_FLATBUF_SERVER) || defined(ENABLE_PROTOBUF_SERVER)
					QObject::connect(GlobalSignals::getInstance(), &GlobalSignals::setBufferImage, hyperion.get(), &Hyperion::forwardBufferMessage);
#endif
					isConnected = true;
				}
			}
			else
			{
				Debug(_log, "Forwarder not connected as instance [%u] is not running", _toBeForwardedInstanceID);
			}
		}
	}
	return isConnected;
}

void MessageForwarder::disconnectFromInstance(quint8 instanceID)
{
	QSharedPointer<Hyperion> const hyperion = _hyperionWeak.toStrongRef();
	if (instanceID == _toBeForwardedInstanceID && !hyperion.isNull())
	{
		Debug(_log, "Disconnect forwarder from instance [%u]", instanceID);

#if defined(ENABLE_FLATBUF_SERVER) || defined(ENABLE_PROTOBUF_SERVER)
		QObject::disconnect(GlobalSignals::getInstance(), &GlobalSignals::setBufferImage, hyperion.get(), &Hyperion::forwardBufferMessage);
#endif

		handleTargets(false, _config.object());
	}
}

void MessageForwarder::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if (type != settings::NETFORWARD) return;

	quint8 const newInstanceID = config["instance"].toInt(NO_INSTANCE_ID);
	if (newInstanceID != _toBeForwardedInstanceID)
	{
		disconnectFromInstance(_toBeForwardedInstanceID);
		_toBeForwardedInstanceID = newInstanceID;
	}

	_config = config;
	_isEnabled = config["enable"].toBool(false);;

	if (_isEnabled && connectToInstance(_toBeForwardedInstanceID))
	{
		start();
	}
	else
	{
		stop();
	}
}

void MessageForwarder::handleCompStateChangeRequest(hyperion::Components component, bool enable)
{
	if (component == hyperion::COMP_FORWARDER && _isActive != enable)
	{
		Info(_log, "Forwarding of instance [%u] is %s", _toBeForwardedInstanceID, (enable ? "active" : "inactive"));

		if (enable)
		{
			if (_hyperionWeak.isNull())
			{
				connectToInstance(_toBeForwardedInstanceID);
			}
			handleTargets(true, _config.object());
		}
		else
		{
			handleTargets(false, _config.object());
		}
	}
}

bool MessageForwarder::isFlatbufferComponent(int priority)
{
	QSharedPointer<Hyperion> const hyperion = _hyperionWeak.toStrongRef();
	if (hyperion.isNull())
	{
		return false;
	}

	bool isFlatbufferComponent{ false };
	hyperion::Components const activeCompId = hyperion->getPriorityInfo(priority).componentId;

		switch (activeCompId) {
		case hyperion::COMP_GRABBER:
		case hyperion::COMP_V4L:
		case hyperion::COMP_AUDIO:
#if defined(ENABLE_FLATBUF_SERVER)
		case hyperion::COMP_FLATBUFSERVER:
#endif
#if defined(ENABLE_PROTOBUF_SERVER)
		case hyperion::COMP_PROTOSERVER:
#endif
#if defined(ENABLE_FLATBUF_SERVER) || defined(ENABLE_PROTOBUF_SERVER)
			isFlatbufferComponent = true;
			break;
#endif
		default:
			break;
		}
	return isFlatbufferComponent;
}

bool MessageForwarder::activateFlatbufferTargets(int priority)
{
	QSharedPointer<Hyperion> const hyperion = _hyperionWeak.toStrongRef();
	if (hyperion.isNull())
	{
		return false;
	}

	int startedFlatbufTargets{ 0 };

	if (priority != PriorityMuxer::LOWEST_PRIORITY)
	{
		if (isFlatbufferComponent(priority))
		{
			startedFlatbufTargets = startFlatbufferTargets(_config.object());
			if (startedFlatbufTargets > 0)
			{
				hyperion::Components const activeCompId = hyperion->getPriorityInfo(priority).componentId;
				switch (activeCompId) {
				case hyperion::COMP_GRABBER:
					QObject::connect(hyperion.get(), &Hyperion::forwardSystemProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage, Qt::UniqueConnection);
					break;
				case hyperion::COMP_V4L:
					QObject::connect(hyperion.get(), &Hyperion::forwardV4lProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage, Qt::UniqueConnection);
					break;
				case hyperion::COMP_AUDIO:
					QObject::connect(hyperion.get(), &Hyperion::forwardAudioProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage, Qt::UniqueConnection);
					break;
#if defined(ENABLE_FLATBUF_SERVER)
				case hyperion::COMP_FLATBUFSERVER:
#endif
#if defined(ENABLE_PROTOBUF_SERVER)
				case hyperion::COMP_PROTOSERVER:
#endif
#if defined(ENABLE_FLATBUF_SERVER) || defined(ENABLE_PROTOBUF_SERVER)

					QObject::connect(hyperion.get(), &Hyperion::forwardBufferMessage, this, &MessageForwarder::forwardFlatbufferMessage, Qt::UniqueConnection);
					break;
#endif
				default:
					break;
				}
			}
		}
	}

	return (startedFlatbufTargets > 0);
}

void MessageForwarder::handleTargets(bool start, const QJsonObject& config)
{
	_isActive = false;
	stopJsonTargets();
	stopFlatbufferTargets();

	QSharedPointer<Hyperion> const hyperion = _hyperionWeak.toStrongRef();
	if (start)
	{
		if (!hyperion.isNull())
		{
			int const jsonTargetNum = startJsonTargets(config);

			int const currentPriority = hyperion->getCurrentPriority();
			bool const isActiveFlatbufferTarget = activateFlatbufferTargets(currentPriority);

			if (jsonTargetNum > 0 || isActiveFlatbufferTarget)
			{
				_isActive = true;
			}
			else
			{
				_isActive = false;
				Warning(_log, "No JSON- nor Flatbuffer targets configured/active -> Forwarding deactivated");
			}
		}
	}

	if (!hyperion.isNull())
	{
		hyperion->setNewComponentState(hyperion::COMP_FORWARDER, _isActive);
	}
}

void MessageForwarder::disconnectFlatBufferComponents(int priority) const
{
	QSharedPointer<Hyperion> const hyperion = _hyperionWeak.toStrongRef();
	if (hyperion.isNull())
	{
		return;
	}

	hyperion::Components const activeCompId = hyperion->getPriorityInfo(priority).componentId;

	switch (activeCompId) {
	case hyperion::COMP_GRABBER:
		QObject::disconnect(hyperion.get(), &Hyperion::forwardV4lProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage);
		QObject::disconnect(hyperion.get(), &Hyperion::forwardAudioProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage);
#if defined(ENABLE_FLATBUF_SERVER) || defined(ENABLE_PROTOBUF_SERVER)
		QObject::disconnect(hyperion.get(), &Hyperion::forwardBufferMessage, this, &MessageForwarder::forwardFlatbufferMessage);
#endif
		break;
	case hyperion::COMP_V4L:
		QObject::disconnect(hyperion.get(), &Hyperion::forwardSystemProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage);
		QObject::disconnect(hyperion.get(), &Hyperion::forwardAudioProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage);
#if defined(ENABLE_FLATBUF_SERVER) || defined(ENABLE_PROTOBUF_SERVER)
		QObject::disconnect(hyperion.get(), &Hyperion::forwardBufferMessage, this, &MessageForwarder::forwardFlatbufferMessage);
#endif
		break;
	case hyperion::COMP_AUDIO:
		QObject::disconnect(hyperion.get(), &Hyperion::forwardSystemProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage);
		QObject::disconnect(hyperion.get(), &Hyperion::forwardV4lProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage);
#if defined(ENABLE_FLATBUF_SERVER) || defined(ENABLE_PROTOBUF_SERVER)
		QObject::disconnect(hyperion.get(), &Hyperion::forwardBufferMessage, this, &MessageForwarder::forwardFlatbufferMessage);
#endif
		break;
#if defined(ENABLE_FLATBUF_SERVER)
	case hyperion::COMP_FLATBUFSERVER:
#endif
#if defined(ENABLE_PROTOBUF_SERVER)
	case hyperion::COMP_PROTOSERVER:
#endif
#if defined(ENABLE_FLATBUF_SERVER) || defined(ENABLE_PROTOBUF_SERVER)
		QObject::disconnect(hyperion.get(), &Hyperion::forwardAudioProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage);
		QObject::disconnect(hyperion.get(), &Hyperion::forwardSystemProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage);
		QObject::disconnect(hyperion.get(), &Hyperion::forwardV4lProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage);
		break;
#endif
	default:
		QObject::disconnect(hyperion.get(), &Hyperion::forwardSystemProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage);
		QObject::disconnect(hyperion.get(), &Hyperion::forwardV4lProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage);
		QObject::disconnect(hyperion.get(), &Hyperion::forwardAudioProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage);
#if defined(ENABLE_FLATBUF_SERVER) || defined(ENABLE_PROTOBUF_SERVER)
		QObject::disconnect(hyperion.get(), &Hyperion::forwardBufferMessage, this, &MessageForwarder::forwardFlatbufferMessage);
#endif
		break;
	}
}

void MessageForwarder::handlePriorityChanges(int priority)
{
	if (priority != 0)
	{
		disconnectFlatBufferComponents(priority);
		if (!isFlatbufferComponent(priority) || priority == PriorityMuxer::LOWEST_PRIORITY)
		{
			stopFlatbufferTargets();
		}
		else
		{
			if (_isActive)
			{
				activateFlatbufferTargets(priority);
			}
			else
			{
				start();
			}
		}
	}
}

void MessageForwarder::addJsonTarget(const QJsonObject& targetConfig)
{
	TargetHost targetHost;

	QString const hostName = targetConfig["host"].toString();
	int port = targetConfig["port"].toInt();

	if (!hostName.isEmpty())
	{
		if (_hyperionWeak.isNull())
		{
			return;
		}

		if (NetUtils::resolveHostToAddress(_log, hostName, targetHost.host, port))
		{
			QString const address = targetHost.host.toString();
			if (hostName != address)
			{
				Debug(_log, "Resolved hostname [%s] to address [%s]", QSTRING_CSTR(hostName), QSTRING_CSTR(address));
			}

			if (NetUtils::isValidPort(_log, port, targetHost.host.toString()))
			{
				targetHost.port = static_cast<quint16>(port);

				// check for loop with JSON-server
				const QJsonObject& obj = _settings.getSettings(settings::JSONSERVER);
				if ((QNetworkInterface::allAddresses().indexOf(targetHost.host) != -1) && targetHost.port == static_cast<quint16>(obj["port"].toInt()))
				{
					Error(_log, "Loop between JSON-Server and Forwarder! Configuration for host: %s, port: %d is ignored.", QSTRING_CSTR(targetHost.host.toString()), port);
				}
				else
				{
					if (_jsonTargets.indexOf(targetHost) == -1)
					{
						QJsonArray const targetInstanceIds = targetConfig["instanceIds"].toArray();

						if (targetInstanceIds.contains(255))
						{
							targetHost.instanceIds = { "all" };
						}
						else
						{
							targetHost.instanceIds = targetInstanceIds;
						}

						Debug(_log, "JSON-Forwarder settings: Adding target host: %s port: %u, instance-IDs: %s", QSTRING_CSTR(targetHost.host.toString()), targetHost.port, QSTRING_CSTR(JsonUtils::jsonValueToQString(targetHost.instanceIds)));
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
			NetUtils::discoverMdnsServices("jsonapi");
		}
#endif

		for (const auto& entry : addr)
		{
			addJsonTarget(entry.toObject());
		}

		if (!_jsonTargets.isEmpty())
		{
			for (const auto& targetHost : std::as_const(_jsonTargets))
			{
				Info(_log, "Forwarding instance [%u] now to JSON-target host: %s port: %u", _toBeForwardedInstanceID, QSTRING_CSTR(targetHost.host.toString()), targetHost.port);
			}
			QObject::connect(GlobalSignals::getInstance(), &GlobalSignals::forwardJsonMessage, this, &MessageForwarder::forwardJsonMessage, Qt::UniqueConnection);
		}
	}
	return _jsonTargets.size();
}


void MessageForwarder::stopJsonTargets()
{
	if (!_jsonTargets.isEmpty())
	{
		QObject::disconnect(GlobalSignals::getInstance(), &GlobalSignals::forwardJsonMessage, this, &MessageForwarder::forwardJsonMessage);
		for (const auto& targetHost : std::as_const(_jsonTargets))
		{
			Info(_log, "Stopped instance [%u] forwarding to JSON-target host: %s port: %u", _toBeForwardedInstanceID, QSTRING_CSTR(targetHost.host.toString()), targetHost.port);
		}
		_jsonTargets.clear();
	}
}

void MessageForwarder::addFlatbufferTarget(const QJsonObject& targetConfig)
{
	TargetHost targetHost;

	QString const hostName = targetConfig["host"].toString();
	int port = targetConfig["port"].toInt();

	if (!hostName.isEmpty())
	{
		if (_hyperionWeak.isNull())
		{
			return;
		}

		if (NetUtils::resolveHostToAddress(_log, hostName, targetHost.host, port))
		{
			QString const address = targetHost.host.toString();
			if (hostName != address)
			{
				Debug(_log, "Resolved hostname [%s] to address [%s]", QSTRING_CSTR(hostName), QSTRING_CSTR(address));
			}

			if (NetUtils::isValidPort(_log, port, targetHost.host.toString()))
			{
				targetHost.port = static_cast<quint16>(port);

				// check for loop with Flatbuffer-server
				const QJsonObject& obj = _settings.getSettings(settings::FLATBUFSERVER);
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
		if (_messageForwarderFlatBufHelper.isNull())
		{
			_messageForwarderFlatBufHelper = QSharedPointer<MessageForwarderFlatbufferClientsHelper>::create();
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
			NetUtils::discoverMdnsServices("flatbuffer");
		}
#endif
		for (const auto& entry : addr)
		{
			addFlatbufferTarget(entry.toObject());
		}

		if (!_flatbufferTargets.isEmpty())
		{
			for (const auto& targetHost : std::as_const(_flatbufferTargets))
			{
				Info(_log, "Forwarding instance [%u] now to Flatbuffer-target host: %s port: %u", _toBeForwardedInstanceID, QSTRING_CSTR(targetHost.host.toString()), targetHost.port);
			}
		}
	}
	return _flatbufferTargets.size();
}

void MessageForwarder::stopFlatbufferTargets()
{
	if (!_flatbufferTargets.isEmpty())
	{
		QSharedPointer<Hyperion> const hyperion = _hyperionWeak.toStrongRef();
		if (!hyperion.isNull())
		{
			QObject::disconnect(hyperion.get(), &Hyperion::forwardSystemProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage);
			QObject::disconnect(hyperion.get(), &Hyperion::forwardV4lProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage);
			QObject::disconnect(hyperion.get(), &Hyperion::forwardAudioProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage);
#if defined(ENABLE_FLATBUF_SERVER) || defined(ENABLE_PROTOBUF_SERVER)
			QObject::disconnect(hyperion.get(), &Hyperion::forwardBufferMessage, this, &MessageForwarder::forwardFlatbufferMessage);
#endif
		}

		emit _messageForwarderFlatBufHelper->clearClients();
		for (const auto& targetHost : std::as_const(_flatbufferTargets))
		{
			Info(_log, "Stopped instance [%u] forwarding to Flatbuffer-target host: %s port: %u", _toBeForwardedInstanceID, QSTRING_CSTR(targetHost.host.toString()), targetHost.port);
		}
		_flatbufferTargets.clear();
	}
}

void MessageForwarder::forwardJsonMessage(const QJsonObject& message, quint8 instanceId)
{
	if (!_isActive)
	{
		return;
	}
	if (instanceId == _toBeForwardedInstanceID)
	{
		QTcpSocket client;
		for (const auto& targetHost : std::as_const(_jsonTargets))
		{
			client.connectToHost(targetHost.host, targetHost.port);
			if (client.waitForConnected(JSON_SOCKET_CONNECT_TIMEOUT.count()))
			{
				sendJsonMessage(message, &client, targetHost.instanceIds);
				client.close();
			}
		}
	}
}

void MessageForwarder::forwardFlatbufferMessage(const QString& /*name*/, const Image<ColorRgb>& image) const
{
	if (_messageForwarderFlatBufHelper)
	{
		bool const isfree = _messageForwarderFlatBufHelper->isFree();

		if (isfree && _isActive)
		{
			QMetaObject::invokeMethod(_messageForwarderFlatBufHelper.get(), "forwardImage", Qt::QueuedConnection, Q_ARG(Image<ColorRgb>, image));
		}
	}
}

void MessageForwarder::sendJsonMessage(const QJsonObject& message, QTcpSocket* socket, const QJsonArray& targetInstanceIds)
{
	// for hyperion classic compatibility
	QJsonObject jsonMessage = message;
	if (jsonMessage.contains("tan") && jsonMessage["tan"].isNull())
	{
		jsonMessage["tan"] = 100;
	}

	if (!targetInstanceIds.empty())
	{
		jsonMessage["instance"] = targetInstanceIds;
	}

	// serialize message
	QJsonDocument const writer(jsonMessage);
	qCDebug(forwarder_write) << "Source instance [" << _toBeForwardedInstanceID << "], JSON-Request: [" << writer.toJson(QJsonDocument::Compact).constData()<< "]";

	QByteArray const serializedMessage = writer.toJson(QJsonDocument::Compact) + "\n";

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
			Debug(_log, "Error while reading data from host");
			return;
		}

		serializedReply += socket->readAll();
	}
	QList const replies = serializedReply.trimmed().split('\n');

	// parse reply data
	QJsonObject response;
	const QString ident = "JsonForwarderTarget@" + socket->peerAddress().toString();
	bool isParsingError{ false };
	QList<QString> errorList;

	for (const QByteArray& reply : replies)
	{
		auto const [isParsed, errorMessages] = JsonUtils::parse(ident, reply, response, _log);
		if (!isParsed)
		{
			qCDebug(forwarder_write) << "Response: [" << JsonUtils::toCompact(response) << "]";
			isParsingError = true;
			errorList.append(errorMessages);
		}
		else
		{
			QString reason = "No error info";
			bool const success = response["success"].toBool(false);
			if (!success)
			{
				Debug(_log, "Source instance [%u], JSON-Request: [%s]", _toBeForwardedInstanceID, writer.toJson(QJsonDocument::Compact).constData());
				reason = response["error"].toString(reason);
				Error(_log, "Request to %s failed with error: %s", QSTRING_CSTR(ident), QSTRING_CSTR(reason));
			}
		}
	}

	if (isParsingError)
	{
		QString const errorText = errorList.join(";");;
		Error(_log, "Error parsing response(s. Errors: %s", QSTRING_CSTR(errorText));
	}
}

MessageForwarderFlatbufferClientsHelper::MessageForwarderFlatbufferClientsHelper()
{
	QThread* mainThread = new QThread();
	mainThread->setObjectName("ForwarderHelperThread");
	this->moveToThread(mainThread);
	mainThread->start();

	_isFree = true;

	QObject::connect(this, &MessageForwarderFlatbufferClientsHelper::addClient, this, &MessageForwarderFlatbufferClientsHelper::addClientHandler);
	QObject::connect(this, &MessageForwarderFlatbufferClientsHelper::clearClients, this, &MessageForwarderFlatbufferClientsHelper::clearClientsHandler);

}

MessageForwarderFlatbufferClientsHelper::~MessageForwarderFlatbufferClientsHelper()
{
	clearClientsHandler();

	QThread* oldThread = this->thread();
	QObject::disconnect(oldThread, nullptr, nullptr, nullptr);
	oldThread->quit();
	oldThread->wait();
	delete oldThread;
}

void MessageForwarderFlatbufferClientsHelper::addClientHandler(const QString& origin, const TargetHost& targetHost, int priority, bool skipReply)
{
	QSharedPointer<FlatBufferConnection> flatbufClient = QSharedPointer<FlatBufferConnection>::create(origin, targetHost.host, priority, skipReply, targetHost.port);
	_forwardClients.append(flatbufClient);
	_isFree = true;
}

void MessageForwarderFlatbufferClientsHelper::clearClientsHandler()
{
	_forwardClients.clear();
	_isFree = false;
}

bool MessageForwarderFlatbufferClientsHelper::isFree() const
{
	return _isFree;
}

void MessageForwarderFlatbufferClientsHelper::forwardImage(const Image<ColorRgb>& image)
{
	_isFree = false;

	for (const auto& client : _forwardClients)
	{
		if (client->isClientRegistered())
		{
			client->setImage(image);
		}
	}

	_isFree = true;
}
