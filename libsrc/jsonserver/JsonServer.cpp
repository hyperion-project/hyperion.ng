// system includes
#include <stdexcept>

// project includes
#include <jsonserver/JsonServer.h>
#include "JsonClientConnection.h"

JsonServer::JsonServer(Hyperion *hyperion, uint16_t port) :
	QObject(),
	_hyperion(hyperion),
	_server(),
	_openConnections()
{
	if (!_server.listen(QHostAddress::Any, port))
	{
		throw std::runtime_error("Json server could not bind to port");
	}

	// Set trigger for incoming connections
	connect(&_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
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
	std::cout << "New incoming json connection" << std::endl;
	QTcpSocket * socket = _server.nextPendingConnection();

	if (socket != nullptr)
	{
		JsonClientConnection * connection = new JsonClientConnection(socket);
		_openConnections.insert(connection);

		// register slot for cleaning up after the connection closed
		connect(connection, SIGNAL(connectionClosed(JsonClientConnection*)), this, SLOT(closedConnection(JsonClientConnection*)));
	}
}

void JsonServer::closedConnection(JsonClientConnection *connection)
{
	std::cout << "Json connection closed" << std::endl;
	_openConnections.remove(connection);

	// schedule to delete the connection object
	connection->deleteLater();
}
