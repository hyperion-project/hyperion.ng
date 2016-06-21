// system includes
#include <stdexcept>

// project includes
#include <boblightserver/BoblightServer.h>
#include "BoblightClientConnection.h"

BoblightServer::BoblightServer(const int priority,uint16_t port) :
	QObject(),
	_hyperion(Hyperion::getInstance()),
	_server(),
	_openConnections(),
	_priority(priority)
{
	if (!_server.listen(QHostAddress::Any, port))
	{
		throw std::runtime_error("BOBLIGHT ERROR: server could not bind to port");
	}

	// Set trigger for incoming connections
	connect(&_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

BoblightServer::~BoblightServer()
{
	foreach (BoblightClientConnection * connection, _openConnections) {
		delete connection;
	}
}

uint16_t BoblightServer::getPort() const
{
	return _server.serverPort();
}

void BoblightServer::newConnection()
{
	QTcpSocket * socket = _server.nextPendingConnection();

	if (socket != nullptr)
	{
		std::cout << "BOBLIGHT INFO: new connection" << std::endl;
		BoblightClientConnection * connection = new BoblightClientConnection(socket, _priority, _hyperion);
		_openConnections.insert(connection);

		// register slot for cleaning up after the connection closed
		connect(connection, SIGNAL(connectionClosed(BoblightClientConnection*)), this, SLOT(closedConnection(BoblightClientConnection*)));
	}
}

void BoblightServer::closedConnection(BoblightClientConnection *connection)
{
	std::cout << "BOBLIGHT INFO: connection closed" << std::endl;
	_openConnections.remove(connection);

	// schedule to delete the connection object
	connection->deleteLater();
}
