// project includes
#include "JsonClientConnection.h"
#include <api/JsonAPI.h>
#include <api/JsonCallbacks.h>

// qt inc
#include <QTcpSocket>
#include <QHostAddress>

JsonClientConnection::JsonClientConnection(QTcpSocket *socket, bool localConnection)
	: QObject()
	, _socket(socket)
	, _receiveBuffer()
	, _log(Logger::getInstance("JSONCLIENTCONNECTION"))
{
	connect(_socket, &QTcpSocket::disconnected, this, &JsonClientConnection::disconnected);
	connect(_socket, &QTcpSocket::readyRead, this, &JsonClientConnection::readRequest);
	// create a new instance of JsonAPI
	_jsonAPI = new JsonAPI(socket->peerAddress().toString(), _log, localConnection, this);
	// get the callback messages from JsonAPI and send it to the client
	connect(_jsonAPI, &JsonAPI::callbackReady, this , &JsonClientConnection::sendMessage);
	connect(_jsonAPI, &JsonAPI::isForbidden, this , &JsonClientConnection::onForbidden);

	connect(_jsonAPI->getCallBack().get(), &JsonCallbacks::callbackReady, this, &JsonClientConnection::sendMessage);

	_jsonAPI->initialize();
}

JsonClientConnection::~JsonClientConnection()
{
	if (_socket)
	{
		_socket->disconnectFromHost();
		_socket->deleteLater();
	}

	delete _jsonAPI;
}

void JsonClientConnection::readRequest()
{
	_receiveBuffer += _socket->readAll();
	// raw socket data, handling as usual
	auto bytes = _receiveBuffer.indexOf('\n') + 1;
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

void JsonClientConnection::onForbidden()
{
	qCDebug(api_msg_reply_error) << "Connection is forbidden, closing connection";

	QJsonObject response;
	response["success"] = false;
	response["error"] = "password change required";
	response["errorData"] = QJsonArray{QJsonObject({{"description", "Default password must be changed before accessing the API"}})};

	sendMessage(response);

	_socket->close();
}

qint64 JsonClientConnection::sendMessage(QJsonObject message)
{
	if (!_socket || (_socket->state() != QAbstractSocket::ConnectedState))
	{
		return 0;
	}

	QJsonDocument writer(message);
	QByteArray data = writer.toJson(QJsonDocument::Compact);
	data.append('\n');

	return _socket->write(data.data(), data.size());
}

void JsonClientConnection::disconnected()
{
	emit connectionClosed();
}

QHostAddress JsonClientConnection::getClientAddress() const
{
	return _socket->peerAddress();
}
