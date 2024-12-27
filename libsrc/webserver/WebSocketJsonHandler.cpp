#include "WebSocketJsonHandler.h"

#include <api/JsonAPI.h>
#include <api/JsonCallbacks.h>
#include <utils/JsonUtils.h>
#include <utils/NetOrigin.h>

WebSocketJsonHandler::WebSocketJsonHandler(QWebSocket* websocket, QObject* parent)
	: QObject(parent)
	, _websocket(websocket)
	, _log(Logger::getInstance("WEBSOCKET"))
{
	connect(_websocket, &QWebSocket::textMessageReceived, this, &WebSocketJsonHandler::onTextMessageReceived);
	connect(_websocket, &QWebSocket::disconnected, this, &WebSocketJsonHandler::onDisconnected);

	const QString client = _websocket->peerAddress().toString();
	Debug(_log, "New WebSocket connection from %s", QSTRING_CSTR(client));

	bool localConnection = NetOrigin::getInstance()->isLocalAddress(_websocket->peerAddress(), _websocket->localAddress());

	// Json processor
	_jsonAPI.reset(new JsonAPI(client, _log, localConnection, this));

	connect(_jsonAPI.get(), &JsonAPI::callbackReady, this, &WebSocketJsonHandler::sendMessage);
	connect(_jsonAPI->getCallBack().get(), &JsonCallbacks::callbackReady, this, &WebSocketJsonHandler::sendMessage);

	// Init JsonAPI
	_jsonAPI->initialize();
}

void WebSocketJsonHandler::onTextMessageReceived(const QString& message)
{
	qDebug() << "WebSocket message received:" << message;
	_jsonAPI.get()->handleMessage(message);
}

qint64 WebSocketJsonHandler::sendMessage(QJsonObject obj)
{
	QString const message = JsonUtils::jsonValueToQString(obj);
	qDebug() << "WebSocket send message: " << message;
	return _websocket->sendTextMessage(message);
}

void WebSocketJsonHandler::onDisconnected()
{
	qDebug() << "WebSocket disconnected";
}


