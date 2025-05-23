#include <flatbufserver/FlatBufferServer.h>
#include "FlatBufferClient.h"
#include "HyperionConfig.h"

// util
#include <utils/NetOrigin.h>
#include <utils/GlobalSignals.h>

// qt
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>

// Constants
namespace {

const char SERVICE_TYPE[] = "flatbuffer";

} //End of constants

FlatBufferServer::FlatBufferServer(const QJsonDocument& config, QObject* parent)
	: QObject(parent)
	, _server(nullptr)
	, _log(Logger::getInstance("FLATBUFSERVER"))
	, _timeout(5000)
	, _config(config)
{
}

FlatBufferServer::~FlatBufferServer()
{
}

void FlatBufferServer::initServer()
{
	_server.reset(new QTcpServer());
	_netOrigin = NetOrigin::getInstance();
	connect(_server.get(), &QTcpServer::newConnection, this, &FlatBufferServer::newConnection);

	// apply config
	handleSettingsUpdate(settings::FLATBUFSERVER, _config);
}

void FlatBufferServer::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type == settings::FLATBUFSERVER)
	{
		const QJsonObject& obj = config.object();

		quint16 port = obj["port"].toInt(19400);

		// port check
		if(_server->serverPort() != port)
		{
			stop();
			_port = port;
		}

		// new timeout just for new connections
		_timeout = obj["timeout"].toInt(5000);
		// enable check
		obj["enable"].toBool(true) ? start() : stop();

		_pixelDecimation = obj["pixelDecimation"].toInt(1);
		for (const auto& client : _openConnections)
		{
			client->setPixelDecimation(_pixelDecimation);
		}
	}
}

void FlatBufferServer::newConnection()
{
	while(_server->hasPendingConnections())
	{
		if(QTcpSocket* socket = _server->nextPendingConnection())
		{
			if(_netOrigin->accessAllowed(socket->peerAddress(), socket->localAddress()))
			{
				Debug(_log, "New connection from %s", QSTRING_CSTR(socket->peerAddress().toString()));
				FlatBufferClient *client = new FlatBufferClient(socket, _timeout, this);

				client->setPixelDecimation(_pixelDecimation);

				// internal
				connect(client, &FlatBufferClient::clientDisconnected, this, &FlatBufferServer::clientDisconnected);
				connect(client, &FlatBufferClient::registerGlobalInput, GlobalSignals::getInstance(), &GlobalSignals::registerGlobalInput);
				connect(client, &FlatBufferClient::clearGlobalInput, GlobalSignals::getInstance(), &GlobalSignals::clearGlobalInput);
				connect(client, &FlatBufferClient::setGlobalInputImage, GlobalSignals::getInstance(), &GlobalSignals::setGlobalImage);
				connect(client, &FlatBufferClient::setGlobalInputColor, GlobalSignals::getInstance(), &GlobalSignals::setGlobalColor);
				connect(client, &FlatBufferClient::setBufferImage, GlobalSignals::getInstance(), &GlobalSignals::setBufferImage);
				_openConnections.append(client);
			}
			else
				socket->close();
		}
	}
}

void FlatBufferServer::clientDisconnected()
{
	FlatBufferClient* client = qobject_cast<FlatBufferClient*>(sender());
	client->deleteLater();
	_openConnections.removeAll(client);
}

void FlatBufferServer::start()
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

			emit publishService(SERVICE_TYPE, _port);
		}
	}
}

void FlatBufferServer::stop()
{
	if(_server->isListening())
	{
		// close client connections
		for(const auto& client : _openConnections)
		{
			client->forceClose();
		}
		_openConnections.clear();
		_server->close();
		Info(_log, "FlatBuffer-Server stopped");
	}
}
