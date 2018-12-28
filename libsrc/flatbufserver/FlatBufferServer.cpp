#include <flatbufserver/FlatBufferServer.h>
#include "FlatBufferClient.h"

// qt
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>


#include <QDebug>


FlatBufferServer::FlatBufferServer(const QJsonDocument& config, QObject* parent)
	: QObject(parent)
	, _server(new QTcpServer(this))
	, _log(Logger::getInstance("FLATBUFSERVER"))
	, _timeout(5000)
	, _config(config)
{

}

FlatBufferServer::~FlatBufferServer()
{
	stopServer();
	delete _server;
}

void FlatBufferServer::initServer()
{
	qDebug()<<"Thread in InitServer is"<<this->thread();
	connect(_server, &QTcpServer::newConnection, this, &FlatBufferServer::newConnection);

	// apply config
	handleSettingsUpdate(settings::FLATBUFSERVER, _config);
}

void FlatBufferServer::handleSettingsUpdate(const settings::type& type, const QJsonDocument& config)
{
	qDebug()<<"Thread in handleSettingsUpdate is"<<this->thread();
	if(type == settings::FLATBUFSERVER)
	{
		const QJsonObject& obj = config.object();

		quint16 port = obj["port"].toInt(19400);

		// port check
		if(_server->serverPort() != port)
		{
			stopServer();
			_port = port;
		}

		// new timeout just for new connections
		_timeout = obj["timeout"].toInt(5000);
		// enable check
		obj["enable"].toBool(true) ? startServer() : stopServer();
	}
}

void FlatBufferServer::newConnection()
{
	qDebug()<<"Thread in newConnection is"<<this->thread();
	while(_server->hasPendingConnections())
	{
		if(QTcpSocket* socket = _server->nextPendingConnection())
		{
			Debug(_log, "New connection from %s", QSTRING_CSTR(socket->peerAddress().toString()));
			FlatBufferClient *client = new FlatBufferClient(socket, _timeout, this);
			// internal
			connect(client, &FlatBufferClient::clientDisconnected, this, &FlatBufferServer::clientDisconnected);
			// forward data
			//connect(clientThread, &FlatBufferClient::);
			_openConnections.append(client);
		}
	}
}

void FlatBufferServer::clientDisconnected()
{
	FlatBufferClient* client = qobject_cast<FlatBufferClient*>(sender());
	client->deleteLater();
	_openConnections.removeAll(client);
}

void FlatBufferServer::startServer()
{
	if(!_server->isListening())
	{
	    if(!_server->listen(QHostAddress::Any, _port))
	    {
	        Error(_log,"Failed to bind port %d", _port);
	    }
	    else
	    {
	        Info(_log,"Started on port %d", _port);
	    }
	}
}

void FlatBufferServer::stopServer()
{
	if(_server->isListening())
	{
		// close client connections
		for(auto client : _openConnections)
		{
			client->forceClose();
		}
		_server->close();
		Info(_log, "Stopped");
	}
}
