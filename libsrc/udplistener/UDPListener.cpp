// system includes
#include <stdexcept>

// project includes
#include <udplistener/UDPListener.h>
#include "UDPClientConnection.h"

UDPListener::UDPListener(Hyperion *hyperion, const int priority,uint16_t port) :
	QObject(),
	_hyperion(hyperion),
	_server(),
	_openConnections(),
	_priority(priority)
{
	if (!_server.listen(QHostAddress::Any, port))
	{
		throw std::runtime_error("UDPLISTENER ERROR: server could not bind to port");
	}

	// Set trigger for incoming connections
	connect(&_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

UDPListener::~UDPListener()
{
	foreach (UDPClientConnection * connection, _openConnections) {
		delete connection;
	}
}

uint16_t UDPListener::getPort() const
{
	return _server.serverPort();
}

void UDPListener::newConnection()
{
	QTcpSocket * socket = _server.nextPendingConnection();

	if (socket != nullptr)
	{
		std::cout << "UDPLISTENER INFO: new connection" << std::endl;
		UDPClientConnection * connection = new UDPClientConnection(socket, _priority, _hyperion);
		_openConnections.insert(connection);

		// register slot for cleaning up after the connection closed
		connect(connection, SIGNAL(connectionClosed(UDPClientConnection*)), this, SLOT(closedConnection(UDPClientConnection*)));
	}
}

void UDPListener::closedConnection(UDPClientConnection *connection)
{
	std::cout << "UDPLISTENER INFO: connection closed" << std::endl;
	_openConnections.remove(connection);

	// schedule to delete the connection object
	connection->deleteLater();
}
