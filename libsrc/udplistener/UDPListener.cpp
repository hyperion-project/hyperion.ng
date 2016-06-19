// system includes
#include <stdexcept>

// project includes
#include <udplistener/UDPListener.h>
#include "UDPClientConnection.h"

UDPListener::UDPListener(Hyperion *hyperion, const int priority, uint16_t port) :
	QObject(),
	_hyperion(hyperion),
	_server(),
	_openConnections(),
	_priority(priority)
{
	_server = new QUdpSocket(this);
	if (!_server->bind(QHostAddress::Any, port))
	{
		throw std::runtime_error("UDPLISTENER ERROR: server could not bind to port");
	}

	// Set trigger for incoming connections
	connect(_server, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
}

UDPListener::~UDPListener()
{
	foreach (UDPClientConnection * connection, _openConnections) {
		delete connection;
	}
}


uint16_t UDPListener::getPort() const
{
	return _server->localPort();
}


void UDPListener::readPendingDatagrams()
{
	while (_server->hasPendingDatagrams()) {
		QByteArray datagram;
		datagram.resize(_server->pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;

		_server->readDatagram(datagram.data(), datagram.size(),
					&sender, &senderPort);

//		processTheDatagram(datagram);

		std::cout << "UDPLISTENER INFO: new packet from " << std::endl;

//		UDPClientConnection * connection = new UDPClientConnection(& datagram, _priority, _hyperion);
//		_openConnections.insert(connection);

		// register slot for cleaning up after the connection closed
//		connect(connection, SIGNAL(connectionClosed(UDPClientConnection*)), this, SLOT(closedConnection(UDPClientConnection*)));
	}
}

/*
void UDPListener::closedConnection(UDPClientConnection *connection)
{
	std::cout << "UDPLISTENER INFO: connection closed" << std::endl;
	_openConnections.remove(connection);

	// schedule to delete the connection object
	connection->deleteLater();
}
*/
