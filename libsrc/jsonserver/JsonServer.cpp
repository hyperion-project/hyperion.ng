// system includes
#include <stdexcept>

// project includes
#include <jsonserver/JsonServer.h>
#include "JsonClientConnection.h"

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

	// connect delay timer and setup
	connect(&_timer, SIGNAL(timeout()), this, SLOT(pushReq()));
	_timer.setSingleShot(true);
	_blockTimer.setSingleShot(true);

	// register for hyprion state changes (bonjour, config, prioritymuxer, register/unregister source)
	connect(_hyperion, SIGNAL(hyperionStateChanged()), this, SLOT(pushReq()));

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
		Debug(_log, "New connection from: %s ",socket->localAddress().toString().toStdString().c_str());
		JsonClientConnection * connection = new JsonClientConnection(socket);
		_openConnections.insert(connection);

		// register for JSONClientConnection events
		connect(connection, SIGNAL(pushReq()), this, SLOT(pushReq()));

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

void JsonServer::pushReq()
{
	if(_blockTimer.isActive())
	{
		_timer.start(250);
	}
	else
	{
		foreach (JsonClientConnection * connection, _openConnections) {
			connection->forceServerInfo();
		}
		_blockTimer.start(250);
	}
}
