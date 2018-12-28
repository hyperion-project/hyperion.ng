// system includes
#include <stdexcept>

// qt incl
#include <QTcpServer>
#include <QJsonObject>

// project includes
#include <protoserver/ProtoServer.h>
#include "protoserver/ProtoConnection.h"
#include "ProtoClientConnection.h"
#include <bonjour/bonjourserviceregister.h>
#include <hyperion/ComponentRegister.h>

ProtoServer::ProtoServer(const QJsonDocument& config)
	: QObject()
	, _server(new QTcpServer(this))
	, _openConnections()
	, _log(Logger::getInstance("PROTOSERVER"))
{
	Debug(_log,"Instance created");
	connect( _server, SIGNAL(newConnection()), this, SLOT(newConnection()));
	handleSettingsUpdate(settings::PROTOSERVER, config);
}

ProtoServer::~ProtoServer()
{
	foreach (ProtoClientConnection * connection, _openConnections) {
		delete connection;
	}
}

void ProtoServer::start()
{
	if(_server->isListening())
		return;

	if (!_server->listen(QHostAddress::Any, _port))
	{
		Error(_log,"Could not bind to port '%d', please use an available port",_port);
		return;
	}
	Info(_log, "Started on port %d", _port);

	if(_serviceRegister == nullptr)
	{
		_serviceRegister = new BonjourServiceRegister(this);
		_serviceRegister->registerService("_hyperiond-proto._tcp", _port);
	}
	else if( _serviceRegister->getPort() != _port)
	{
		delete _serviceRegister;
		_serviceRegister = new BonjourServiceRegister(this);
		_serviceRegister->registerService("_hyperiond-proto._tcp", _port);
	}
}

void ProtoServer::stop()
{
	if(!_server->isListening())
		return;

	_server->close();
	Info(_log, "Stopped");
}

void ProtoServer::handleSettingsUpdate(const settings::type& type, const QJsonDocument& config)
{
	if(type == settings::PROTOSERVER)
	{
		QJsonObject obj = config.object();
		if(obj["port"].toInt() != _port)
		{
			_port = obj["port"].toInt();
			stop();
			start();
		}
	}
}

uint16_t ProtoServer::getPort() const
{
	return _port;
}

void ProtoServer::newConnection()
{
	while(_server->hasPendingConnections())
	{
		if(QTcpSocket * socket = _server->nextPendingConnection())
		{
			Debug(_log, "New connection");
			ProtoClientConnection * connection = new ProtoClientConnection(socket);
			_openConnections.insert(connection);

			// register slot for cleaning up after the connection closed
			connect(connection, SIGNAL(connectionClosed(ProtoClientConnection*)), this, SLOT(closedConnection(ProtoClientConnection*)));
			//connect(connection, SIGNAL(newMessage(const proto::HyperionRequest*)), this, SLOT(newMessage(const proto::HyperionRequest*)));
		}
	}
}

void ProtoServer::closedConnection(ProtoClientConnection *connection)
{
	Debug(_log, "Connection closed");
	_openConnections.remove(connection);

	// schedule to delete the connection object
	connection->deleteLater();
}
