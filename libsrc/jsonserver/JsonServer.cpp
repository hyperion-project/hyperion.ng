// system includes
#include <stdexcept>

// project includes
#include <jsonserver/JsonServer.h>
#include "JsonClientConnection.h"

// hyperion include
#include <hyperion/MessageForwarder.h>

// qt includes
#include <QTcpSocket>
#include <QJsonDocument>
#include <QByteArray>

JsonServer::JsonServer(uint16_t port)
	: QObject()
	, _server()
	, _hyperion(Hyperion::getInstance())
	, _openConnections()
	, _log(Logger::getInstance("JSONSERVER"))
{
	if (!_server.listen(QHostAddress::Any, port))
	{
		throw std::runtime_error("JSONSERVER ERROR: could not bind to port");
	}

		QList<MessageForwarder::JsonSlaveAddress> list = Hyperion::getInstance()->getForwarder()->getJsonSlaves();
		for ( int i=0; i<list.size(); i++ )
		{
			if ( list.at(i).addr == QHostAddress::LocalHost && list.at(i).port == port ) {
				throw std::runtime_error("JSONSERVER ERROR: Loop between proto server and forwarder detected. Fix your config!");
			}
		}

	// Set trigger for incoming connections
	connect(&_server, SIGNAL(newConnection()), this, SLOT(newConnection()));

	// receive state of forwarder
	connect(_hyperion, &Hyperion::componentStateChanged, this, &JsonServer::componentStateChanged);

	// initial connect TODO get initial state from config to stop messing
	connect(_hyperion, &Hyperion::forwardJsonMessage, this, &JsonServer::forwardJsonMessage);
}

JsonServer::~JsonServer()
{
	foreach (JsonClientConnection * connection, _openConnections) {
		delete connection;
	}
}

uint16_t JsonServer::getPort() const
{
	return _server.serverPort();
}

void JsonServer::newConnection()
{
	while(_server.hasPendingConnections())
	{
		if (QTcpSocket * socket = _server.nextPendingConnection())
		{
			Debug(_log, "New connection from: %s ",socket->localAddress().toString().toStdString().c_str());
			JsonClientConnection * connection = new JsonClientConnection(socket);
			_openConnections.insert(connection);

			// register slot for cleaning up after the connection closed
			connect(connection, &JsonClientConnection::connectionClosed, this, &JsonServer::closedConnection);
		}
	}
}

void JsonServer::closedConnection(void)
{
	JsonClientConnection* connection = qobject_cast<JsonClientConnection*>(sender());
	Debug(_log, "Connection closed");
	_openConnections.remove(connection);

	// schedule to delete the connection object
	connection->deleteLater();
}

void JsonServer::componentStateChanged(const hyperion::Components component, bool enable)
{
	if (component == hyperion::COMP_FORWARDER && _forwarder_enabled != enable)
	{
		_forwarder_enabled = enable;
		Info(_log, "forwarder change state to %s", (enable ? "enabled" : "disabled") );
		if(_forwarder_enabled)
		{
			connect(_hyperion, &Hyperion::forwardJsonMessage, this, &JsonServer::forwardJsonMessage);
		}
		else
		{
			disconnect(_hyperion, &Hyperion::forwardJsonMessage, this, &JsonServer::forwardJsonMessage);
		}
	}
}

void JsonServer::forwardJsonMessage(const QJsonObject &message)
{
	QTcpSocket client;
	QList<MessageForwarder::JsonSlaveAddress> list = _hyperion->getForwarder()->getJsonSlaves();

	for ( int i=0; i<list.size(); i++ )
	{
		client.connectToHost(list.at(i).addr, list.at(i).port);
		if ( client.waitForConnected(500) )
		{
			sendMessage(message,&client);
			client.close();
		}
	}
}

void JsonServer::sendMessage(const QJsonObject & message, QTcpSocket * socket)
{
	// serialize message
	QJsonDocument writer(message);
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
	QJsonDocument reply = QJsonDocument::fromJson(serializedReply ,&error);

	if (error.error != QJsonParseError::NoError)
	{
		Error(_log, "Error while parsing reply: invalid json");
		return;
	}

}
