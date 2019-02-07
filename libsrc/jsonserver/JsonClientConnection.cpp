// project includes
#include "JsonClientConnection.h"
#include <api/JsonAPI.h>

// qt inc
#include <QTcpSocket>
#include <QHostAddress>

// websocket includes
#include "webserver/WebSocketClient.h"

JsonClientConnection::JsonClientConnection(QTcpSocket *socket)
	: QObject()
	, _socket(socket)
	, _websocketClient(nullptr)
	, _receiveBuffer()
	, _log(Logger::getInstance("JSONCLIENTCONNECTION"))
{
	connect(_socket, &QTcpSocket::disconnected, this, &JsonClientConnection::disconnected);
	connect(_socket, &QTcpSocket::readyRead, this, &JsonClientConnection::readRequest);
	// create a new instance of JsonAPI
	_jsonAPI = new JsonAPI(socket->peerAddress().toString(), _log, this);
	// get the callback messages from JsonAPI and send it to the client
	connect(_jsonAPI,SIGNAL(callbackMessage(QJsonObject)),this,SLOT(sendMessage(QJsonObject)));
}

void JsonClientConnection::readRequest()
{
	_receiveBuffer += _socket->readAll();

	// might be an old hyperion classic handshake request or raw socket data
	if(_receiveBuffer.contains("Upgrade: websocket"))
	{
		if(_websocketClient == Q_NULLPTR)
		{
			// disconnect this slot from socket for further requests
			disconnect(_socket, &QTcpSocket::readyRead, this, &JsonClientConnection::readRequest);
			int start = _receiveBuffer.indexOf("Sec-WebSocket-Key") + 19;
			QByteArray header(_receiveBuffer.mid(start, _receiveBuffer.indexOf("\r\n", start) - start).data());
			_websocketClient = new WebSocketClient(header, _socket, this);
		}
	}
	else
	{
		// raw socket data, handling as usual
		int bytes = _receiveBuffer.indexOf('\n') + 1;
		while(bytes > 0)
		{
			// create message string
			QString message(QByteArray(_receiveBuffer.data(), bytes));

			// remove message data from buffer
			_receiveBuffer = _receiveBuffer.mid(bytes);

			// handle message
			_jsonAPI->handleMessage(message);

			// try too look up '\n' again
			bytes = _receiveBuffer.indexOf('\n') + 1;
		}
	}
}

qint64 JsonClientConnection::sendMessage(QJsonObject message)
{
	QJsonDocument writer(message);
	QByteArray data = writer.toJson(QJsonDocument::Compact) + "\n";

	if (!_socket || (_socket->state() != QAbstractSocket::ConnectedState)) return 0;
	return _socket->write(data.data(), data.size());
}

void JsonClientConnection::disconnected(void)
{
	emit connectionClosed();
}
