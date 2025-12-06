#include <flatbufserver/FlatBufferServer.h>

#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>

#include "FlatBufferClient.h"
#include "HyperionConfig.h"

#include <utils/NetOrigin.h>
#include <utils/GlobalSignals.h>

Q_LOGGING_CATEGORY(flatbuffer_server_flow, "hyperion.flatbuffer.server.flow");

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
	TRACK_SCOPE();
}

FlatBufferServer::~FlatBufferServer()
{
	TRACK_SCOPE();
}

void FlatBufferServer::initServer()
{
	qCDebug(flatbuffer_server_flow) << "Initializing FlatBuffer server";
	_server.reset(new QTcpServer());
	_netOriginWeak = NetOrigin::getInstance();
	connect(_server.get(), &QTcpServer::newConnection, this, &FlatBufferServer::newConnection);

	// apply config
	handleSettingsUpdate(settings::FLATBUFSERVER, _config);
}

void FlatBufferServer::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type == settings::FLATBUFSERVER)
	{
		qCDebug(flatbuffer_server_flow) << "Handling FlatBuffer server settings update";
		const QJsonObject& obj = config.object();

		auto port = static_cast<quint16>(obj["port"].toInt(19400));

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
			Debug(_log, "New connection from %s", QSTRING_CSTR(socket->peerAddress().toString()));
			auto* client = new FlatBufferClient(socket, _timeout, this);

			client->setPixelDecimation(_pixelDecimation);

			// internal
			connect(client, &FlatBufferClient::clientDisconnected, this, &FlatBufferServer::clientDisconnected);
			connect(client, &FlatBufferClient::registerGlobalInput, GlobalSignals::getInstance(), &GlobalSignals::registerGlobalInput);
			connect(client, &FlatBufferClient::clearGlobalInput, GlobalSignals::getInstance(), &GlobalSignals::clearGlobalInput);
			connect(client, &FlatBufferClient::setGlobalInputImage, GlobalSignals::getInstance(), &GlobalSignals::setGlobalImage);
			connect(client, &FlatBufferClient::setGlobalInputColor, GlobalSignals::getInstance(), &GlobalSignals::setGlobalColor);
			connect(client, &FlatBufferClient::setBufferImage, GlobalSignals::getInstance(), &GlobalSignals::setBufferImage);
			connect(GlobalSignals::getInstance(), &GlobalSignals::globalRegRequired, client, &FlatBufferClient::registationRequired);

			_openConnections.append(client);
		}
	}
}

void FlatBufferServer::clientDisconnected()
{

	FlatBufferClient* client = qobject_cast<FlatBufferClient*>(sender());

	qCDebug(flatbuffer_server_flow) << "FlatBuffer client disconnected" << QString("%1@%2").arg(client->getOrigin(),client->getAddress());
	client->deleteLater();
	_openConnections.removeAll(client);
}

void FlatBufferServer::start() const
{
	qCDebug(flatbuffer_server_flow) << "Starting FlatBuffer server, isListening" << _server->isListening() << "on port" << _port;
}

void FlatBufferServer::stop()
{
	qCDebug(flatbuffer_server_flow) << "Stopping FlatBuffer server on port" << _port;
	if (!_server.isNull())
	{
		close();
	}

	emit isStopped();
}

void FlatBufferServer::open()
{
	qCDebug(flatbuffer_server_flow) << "Open FlatBuffer server on port" << _port;
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

void FlatBufferServer::close()
{
	qCDebug(flatbuffer_server_flow) << "Close FlatBuffer server. Number of open connections:" << _openConnections.size();
	if(_server->isListening())
	{
		// close client connections
		for(const auto& client : _openConnections)
		{
			client->forceClose();
		}
		_server->close();
		_openConnections.clear();

		Info(_log, "FlatBuffer-Server closed");
	}
}

void  FlatBufferServer::registerClients() const
{
	for (const auto& client : _openConnections)
	{
		qCDebug(flatbuffer_server_flow) << "Registering FlatBuffer client" << QString("%1@%2").arg(client->getOrigin(),client->getAddress());
		emit client->registerGlobalInput(client->getPriority(), hyperion::COMP_FLATBUFSERVER, QSTRING_CSTR(QString("%1@%2").arg(client->getOrigin(),client->getAddress())));
	}	
}
