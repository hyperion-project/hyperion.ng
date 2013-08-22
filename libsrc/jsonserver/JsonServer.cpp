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
		std::cout << "New json connection" << std::endl;
		JsonClientConnection * connection = new JsonClientConnection(socket, _hyperion);
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
