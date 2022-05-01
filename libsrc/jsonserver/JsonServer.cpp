// system includes
#include <stdexcept>

// qt includes
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QByteArray>

// project includes
#include "HyperionConfig.h"
#include <jsonserver/JsonServer.h>
#include "JsonClientConnection.h"

#include <utils/NetOrigin.h>

// Constants
namespace {

const char SERVICE_TYPE[] = "jsonapi";

} //End of constants

JsonServer::JsonServer(const QJsonDocument& config)
	: QObject()
	, _server(new QTcpServer(this))
	, _openConnections()
	, _log(Logger::getInstance("JSONSERVER"))
	, _netOrigin(NetOrigin::getInstance())
	, _config(config)
{
	Debug(_log, "Created instance");
}

JsonServer::~JsonServer()
{
	qDeleteAll(_openConnections);
}

void JsonServer::initServer()
{
	// Set trigger for incoming connections
	connect(_server, &QTcpServer::newConnection, this, &JsonServer::newConnection);

	// init
	handleSettingsUpdate(settings::JSONSERVER, _config);
}

void JsonServer::start()
{
	if(!_server->isListening())
	{
		if (!_server->listen(QHostAddress::Any, _port))
		{
			Error(_log,"Could not bind to port '%d', please use an available port", _port);
		}
		else
		{
			Info(_log, "Started on port %d", _port);
			emit publishService(SERVICE_TYPE, _port);
		}
	}
}

void JsonServer::stop()
{
	if(!_server->isListening())
		return;

	_server->close();
	Info(_log, "Stopped");
}

void JsonServer::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type == settings::JSONSERVER)
	{
		QJsonObject obj = config.object();
		if(_port != obj["port"].toInt())
		{
			_port = obj["port"].toInt();
			stop();
			start();
		}
	}
}

uint16_t JsonServer::getPort() const
{
	return _port;
}

void JsonServer::newConnection()
{
	while(_server->hasPendingConnections())
	{
		if (QTcpSocket * socket = _server->nextPendingConnection())
		{
			if(_netOrigin->accessAllowed(socket->peerAddress(), socket->localAddress()))
			{
				Debug(_log, "New connection from: %s",QSTRING_CSTR(socket->peerAddress().toString()));
				JsonClientConnection * connection = new JsonClientConnection(socket, _netOrigin->isLocalAddress(socket->peerAddress(), socket->localAddress()));
				_openConnections.insert(connection);

				// register slot for cleaning up after the connection closed
				connect(connection, &JsonClientConnection::connectionClosed, this, &JsonServer::closedConnection);
			}
			else
				socket->close();
		}
	}
}

void JsonServer::closedConnection()
{
	JsonClientConnection* connection = qobject_cast<JsonClientConnection*>(sender());
	Debug(_log, "Connection closed for %s", QSTRING_CSTR(connection->getClientAddress().toString()));
	_openConnections.remove(connection);

	// schedule to delete the connection object
	connection->deleteLater();
}
