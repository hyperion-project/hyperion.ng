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
	, _server(nullptr)
	, _openConnections()
	, _log(Logger::getInstance("JSONSERVER"))
	, _netOriginWeak(NetOrigin::getInstance())
	, _config(config)
{
	TRACK_SCOPE();
}

JsonServer::~JsonServer()
{
	TRACK_SCOPE();
}

void JsonServer::initServer()
{
	Debug(_log, "Initialize JSON API server");
	if (_server.isNull())
	{
		_server.reset(new QTcpServer());
	}

	// Set trigger for incoming connections
	connect(_server.get(), &QTcpServer::newConnection, this, &JsonServer::newConnection);

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
	// Close the server if available
	if (!_server.isNull())
	{
		if (_server->isListening())
		{
			_server->close();
		}
	}

	// Ensure all open connections are deleted from the owning thread
	qDeleteAll(_openConnections);
	_openConnections.clear();

	Info(_log, "JSON-Server stopped");

	emit isStopped();
}

void JsonServer::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type == settings::JSONSERVER)
	{
		QJsonObject obj = config.object();
		auto port = static_cast<uint16_t>(obj["port"].toInt());
		if(_port != port)
		{
			_port = port;
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
		if (auto* socket = _server->nextPendingConnection())
		{
			Debug(_log, "New connection from: %s",QSTRING_CSTR(socket->peerAddress().toString()));
			bool isLocal = false;
			if (auto origin = _netOriginWeak.toStrongRef())
			{
				isLocal = origin->isLocalAddress(socket->peerAddress(), socket->localAddress());
			}
			auto * connection = new JsonClientConnection(socket, isLocal);
			_openConnections.insert(connection);

			// register slot for cleaning up after the connection closed
			connect(connection, &JsonClientConnection::connectionClosed, this, &JsonServer::closedConnection);
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
