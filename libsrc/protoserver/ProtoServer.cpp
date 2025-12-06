#include "ProtoClientConnection.h"
#include <protoserver/ProtoServer.h>

#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>

#include <utils/NetOrigin.h>
#include <utils/GlobalSignals.h>

Q_LOGGING_CATEGORY(proto_server_flow, "hyperion.proto.server.flow");

// Constants
namespace {

const char SERVICE_TYPE[] = "protobuffer";

} //End of constants

ProtoServer::ProtoServer(const QJsonDocument& config, QObject* parent)
	: QObject(parent)
	, _server(nullptr)
	, _log(Logger::getInstance("PROTOSERVER"))
	, _timeout(5000)
	, _config(config)
{
	TRACK_SCOPE();
}

ProtoServer::~ProtoServer()
{
	TRACK_SCOPE();
}

void ProtoServer::initServer()
{
	qCDebug(proto_server_flow) << "Initializing Protocol Buffer Server";
	_server.reset(new QTcpServer());
	_netOriginWeak = NetOrigin::getInstance();
	connect(_server.get(), &QTcpServer::newConnection, this, &ProtoServer::newConnection);

	// apply config
	handleSettingsUpdate(settings::PROTOSERVER, _config);
}

void ProtoServer::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type == settings::PROTOSERVER)
	{
		qCDebug(proto_server_flow) << "Handling Protocol Buffer server settings update";
		
		const QJsonObject& obj = config.object();

		quint16 port = obj["port"].toInt(19445);

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
	}
}

void ProtoServer::newConnection()
{
	while(_server->hasPendingConnections())
	{
		if(QTcpSocket * socket = _server->nextPendingConnection())
		{
			Debug(_log, "New connection from %s", QSTRING_CSTR(socket->peerAddress().toString()));
			auto* client = new ProtoClientConnection(socket, _timeout, this);
			// internal
			connect(client, &ProtoClientConnection::clientDisconnected, this, &ProtoServer::clientDisconnected);
			connect(client, &ProtoClientConnection::registerGlobalInput, GlobalSignals::getInstance(), &GlobalSignals::registerGlobalInput);
			connect(client, &ProtoClientConnection::clearGlobalInput, GlobalSignals::getInstance(), &GlobalSignals::clearGlobalInput);
			connect(client, &ProtoClientConnection::setGlobalInputImage, GlobalSignals::getInstance(), &GlobalSignals::setGlobalImage);
			connect(client, &ProtoClientConnection::setGlobalInputColor, GlobalSignals::getInstance(), &GlobalSignals::setGlobalColor);
			connect(client, &ProtoClientConnection::setBufferImage, GlobalSignals::getInstance(), &GlobalSignals::setBufferImage);
			connect(GlobalSignals::getInstance(), &GlobalSignals::globalRegRequired, client, &ProtoClientConnection::registationRequired);
			_openConnections.append(client);
		}
	}
}

void ProtoServer::clientDisconnected()
{
	ProtoClientConnection* client = qobject_cast<ProtoClientConnection*>(sender());

	qCDebug(proto_server_flow) << "Protocol Buffer client disconnected" << QString("PROTO@%1").arg(client->getAddress());
	client->deleteLater();
	_openConnections.removeAll(client);
}

void ProtoServer::start()
{
	qCDebug(proto_server_flow) << "Starting Protocol Buffer server, isListening" << _server->isListening() << "on port" << _port;
}

void ProtoServer::stop()
{
	qCDebug(proto_server_flow) << "Stopping Protocol Buffer server on port" << _port;
	if (!_server.isNull())
	{
		close();
	}

	emit isStopped();
}

void ProtoServer::open()
{
	qCDebug(proto_server_flow) << "Open Protocol Buffer server on port" << _port;
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

void ProtoServer::close()
{
	qCDebug(proto_server_flow) << "Close Protocol Buffer server. Number of open connections:" << _openConnections.size();
	if(_server->isListening())
	{
		// close client connections
		for(const auto& client : _openConnections)
		{
			client->forceClose();
		}
		_server->close();
		_openConnections.clear();

		Info(_log, "Protocol Buffer server closed");
	}
}

void  ProtoServer::registerClients() const
{
	for (const auto& client : _openConnections)
	{
		qCDebug(proto_server_flow) << "Registering Protocol Buffer client" << QString("PROTO@%1").arg(client->getAddress());
		emit client->registerGlobalInput(client->getPriority(), hyperion::COMP_PROTOSERVER, QSTRING_CSTR(QString("PROTO@%1").arg(client->getAddress())));
	}	
}
