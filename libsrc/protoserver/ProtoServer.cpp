// system includes
#include <stdexcept>

// project includes
#include <protoserver/ProtoServer.h>
#include "ProtoClientConnection.h"

ProtoServer::ProtoServer(Hyperion *hyperion, uint16_t port) :
	QObject(),
	_hyperion(hyperion),
	_server(),
	_openConnections()
{
	if (!_server.listen(QHostAddress::Any, port))
	{
		throw std::runtime_error("Proto server could not bind to port");
	}

	// Set trigger for incoming connections
	connect(&_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

ProtoServer::~ProtoServer()
{
	foreach (ProtoClientConnection * connection, _openConnections) {
		delete connection;
	}
}

uint16_t ProtoServer::getPort() const
{
	return _server.serverPort();
}

void ProtoServer::newConnection()
{
	QTcpSocket * socket = _server.nextPendingConnection();

	if (socket != nullptr)
	{
		std::cout << "New proto connection" << std::endl;
		ProtoClientConnection * connection = new ProtoClientConnection(socket, _hyperion);
		_openConnections.insert(connection);

		// register slot for cleaning up after the connection closed
		connect(connection, SIGNAL(connectionClosed(ProtoClientConnection*)), this, SLOT(closedConnection(ProtoClientConnection*)));
	}
}

void ProtoServer::closedConnection(ProtoClientConnection *connection)
{
	std::cout << "Proto connection closed" << std::endl;
	_openConnections.remove(connection);

	// schedule to delete the connection object
	connection->deleteLater();
}
