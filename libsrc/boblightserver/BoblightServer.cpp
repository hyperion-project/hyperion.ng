// system includes
#include <stdexcept>

// project includes
#include <boblightserver/BoblightServer.h>
#include "BoblightClientConnection.h"

BoblightServer::BoblightServer(const int priority, uint16_t port)
	: QObject()
	, _hyperion(Hyperion::getInstance())
	, _server()
	, _openConnections()
	, _priority(priority)
	, _log(Logger::getInstance("BOBLIGHT"))
	, _isActive(false)
	, _port(port)
{
	// Set trigger for incoming connections
	connect(&_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

BoblightServer::~BoblightServer()
{
	stop();
}

void BoblightServer::start()
{
	if ( active() )
		return;
		
	if (!_server.listen(QHostAddress::Any, _port))
	{
		throw std::runtime_error("BOBLIGHT ERROR: server could not bind to port");
	}
	Info(_log, "Boblight server started on port %d", _port);

	_isActive = true;
	emit statusChanged(_isActive);

	_hyperion->registerPriority("Boblight", _priority);

}

void BoblightServer::stop()
{
	if ( ! active() )
		return;
		
	foreach (BoblightClientConnection * connection, _openConnections) {
		delete connection;
	}
	_isActive = false;
	emit statusChanged(_isActive);

	_hyperion->unRegisterPriority("Boblight");

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
		Info(_log, "new connection");
		BoblightClientConnection * connection = new BoblightClientConnection(socket, _priority);
		_openConnections.insert(connection);

		// register slot for cleaning up after the connection closed
		connect(connection, SIGNAL(connectionClosed(BoblightClientConnection*)), this, SLOT(closedConnection(BoblightClientConnection*)));
	}
}

void BoblightServer::closedConnection(BoblightClientConnection *connection)
{
	Debug(_log, "connection closed");
	_openConnections.remove(connection);

	// schedule to delete the connection object
	connection->deleteLater();
}
