#include "WebSocketJsonHandler.h"

#include <api/JsonAPI.h>
#include <api/JsonCallbacks.h>
#include <utils/JsonUtils.h>
#include <utils/NetOrigin.h>
#include <hyperion/AuthManager.h>

Q_LOGGING_CATEGORY(comm_websocket_receive, "hyperion.comm.websocket.receive");
Q_LOGGING_CATEGORY(comm_websocket_send, "hyperion.comm.websocket.send");

WebSocketJsonHandler::WebSocketJsonHandler(QWebSocket* websocket, QObject* parent)
	: QObject(parent)
	, _websocket(websocket)
	, _log(Logger::getInstance("WEBSOCKET"))
{
	connect(_websocket, &QWebSocket::textMessageReceived, this, &WebSocketJsonHandler::onTextMessageReceived);
	connect(_websocket, &QWebSocket::binaryMessageReceived, this, &WebSocketJsonHandler::onBinaryMessageReceived);
	connect(_websocket, &QWebSocket::disconnected, this, &WebSocketJsonHandler::onDisconnected);

	_peerAddress = _websocket->peerAddress().toString();
	_origin = websocket->origin();
	Debug(_log, "New WebSocket connection from %s initiated via: %s", QSTRING_CSTR(_peerAddress), QSTRING_CSTR(_origin));

	bool localConnection = false;
	if (auto origin = NetOrigin::getInstanceWeak().toStrongRef())
	{
		localConnection = origin->isLocalAddress(_websocket->peerAddress(), _websocket->localAddress());
	}

	if (!localConnection && AuthManager::getInstance() && AuthManager::getInstance()->isDefaultUserPassword())
	{
		Warning(_log, "Non local network WebSocket connect attempt from %s initiated via %s identified, but default Hyperion password is set! - Reject connection.", QSTRING_CSTR(_peerAddress), QSTRING_CSTR(_origin));
		 _websocket->close(QWebSocketProtocol::CloseCodePolicyViolated, "Remote connections are not allowed to use WebSocket API");
		return;
	}
	{
		Debug(_log, "WebSocket connection from %s identified as local connection", QSTRING_CSTR(_peerAddress));
	}

	// Json processor
	_jsonAPI.reset(new JsonAPI(_peerAddress, _log, localConnection));

	connect(_jsonAPI.get(), &JsonAPI::callbackReady, this, &WebSocketJsonHandler::sendMessage);
	connect(_jsonAPI->getCallBack().get(), &JsonCallbacks::callbackReady, this, &WebSocketJsonHandler::sendMessage);

	// Init JsonAPI
	_jsonAPI->initialize();
}

void WebSocketJsonHandler::onTextMessageReceived(const QString& message)
{
	qCDebug(comm_websocket_receive) << "[" << _peerAddress << "] WebSocket message received:" << message;
	_jsonAPI->handleMessage(message);
}

void WebSocketJsonHandler::onBinaryMessageReceived(const QByteArray& message)
{
	qCDebug(comm_websocket_receive) << "[" << _peerAddress << "] WebSocket message received:" << message.toHex();
	Warning(_log,"Unexpected binary message received");
}

qint64 WebSocketJsonHandler::sendMessage(QJsonObject obj)
{
	qCDebug(comm_websocket_send) << "[" << _peerAddress << "] WebSocket send message: " << obj;
	return _websocket->sendTextMessage(JsonUtils::jsonValueToQString(obj));
}

void WebSocketJsonHandler::onDisconnected()
{
	Debug(_log, "WebSocket disconnected from %s initiated via: %s", QSTRING_CSTR(_peerAddress), QSTRING_CSTR(_origin));
}
