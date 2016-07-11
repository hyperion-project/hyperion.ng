// system includes
#include <stdexcept>

// project includes
#include <jsonserver/JsonServer.h>
#include "JsonClientConnection.h"

JsonServer::JsonServer(uint16_t port) :
	QObject(),
	_hyperion(Hyperion::getInstance()),
	_server(),
	_openConnections(),
	_log(Logger::getInstance("JSONSERVER"))
{
	if (!_server.listen(QHostAddress::Any, port))
	{
		Error(log, "Could not bind to port");
	}

		QList<MessageForwarder::JsonSlaveAddress> list = _hyperion->getForwarder()->getJsonSlaves();
		for ( int i=0; i<list.size(); i++ )
		{
			if ( list.at(i).addr == QHostAddress::LocalHost && list.at(i).port == port ) {
				Error(_log, "Loop between proto server and forwarder detected. Fix your config!");
			}
		}

	// Set trigger for incoming connections
	connect(&_server, SIGNAL(newConnection()), this, SLOT(newConnection()));

	// make sure the resources are loaded (they may be left out after static linking
	Q_INIT_RESOURCE(JsonSchemas);

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
	QTcpSocket * socket = _server.nextPendingConnection();

	if (socket != nullptr)
	{
		Debug(_log, "New connection");
		JsonClientConnection * connection = new JsonClientConnection(socket, _hyperion);
		_openConnections.insert(connection);

		// register slot for cleaning up after the connection closed
		connect(connection, SIGNAL(connectionClosed(JsonClientConnection*)), this, SLOT(closedConnection(JsonClientConnection*)));
	}
}

void JsonServer::closedConnection(JsonClientConnection *connection)
{
	Debug(_log, "Connection closed");
	_openConnections.remove(connection);

	// schedule to delete the connection object
	connection->deleteLater();
}
