// system includes
#include <stdexcept>

// project includes
#include <boblightserver/BoblightServer.h>
#include "BoblightClientConnection.h"

// hyperion includes
#include <hyperion/Hyperion.h>

// qt incl
#include <QTcpServer>

// netUtil
#include <utils/NetUtils.h>

using namespace hyperion;

BoblightServer::BoblightServer(Hyperion* hyperion,const QJsonDocument& config)
	: QObject()
	, _hyperion(hyperion)
	, _server(new QTcpServer(this))
	, _openConnections()
	, _priority(0)
	, _log(nullptr)
	, _port(0)
{
	QString subComponent = _hyperion->property("instance").toString();
	_log= Logger::getInstance("BOBLIGHT", subComponent);

	Debug(_log, "Instance created");

	// listen for component change
	connect(_hyperion, &Hyperion::compStateChangeRequest, this, &BoblightServer::compStateChangeRequest);
	// listen new connection signal from server
	connect(_server, &QTcpServer::newConnection, this, &BoblightServer::newConnection);

	// init
	handleSettingsUpdate(settings::BOBLSERVER, config);
}

BoblightServer::~BoblightServer()
{
	stop();
}

void BoblightServer::start()
{
	if ( _server->isListening() )
		return;

	if (NetUtils::portAvailable(_port, _log))
		_server->listen(QHostAddress::Any, _port);

	Info(_log, "Started on port: %d", _port);

	_hyperion->setNewComponentState(COMP_BOBLIGHTSERVER, _server->isListening());
}

void BoblightServer::stop()
{
	if ( ! _server->isListening() )
		return;

	qDeleteAll(_openConnections);

	_server->close();

	Info(_log, "Stopped");
	_hyperion->setNewComponentState(COMP_BOBLIGHTSERVER, _server->isListening());
}

bool BoblightServer::active() const
{
	return _server->isListening();
}

void BoblightServer::compStateChangeRequest(hyperion::Components component, bool enable)
{
	if (component == COMP_BOBLIGHTSERVER)
	{
		if (_server->isListening() != enable)
		{
			if (enable) start();
			else        stop();
		}
	}
}

uint16_t BoblightServer::getPort() const
{
	return _server->serverPort();
}

void BoblightServer::newConnection()
{
	QTcpSocket * socket = _server->nextPendingConnection();
	if (socket != nullptr)
	{
		Info(_log, "New connection from %s ", QSTRING_CSTR(QString("Boblight@%1").arg(socket->peerAddress().toString())));
		BoblightClientConnection * connection = new BoblightClientConnection(_hyperion, socket, _priority);
		_openConnections.insert(connection);

		// register slot for cleaning up after the connection closed
		connect(connection, &BoblightClientConnection::connectionClosed, this, &BoblightServer::closedConnection);
	}
}

void BoblightServer::closedConnection(BoblightClientConnection *connection)
{
	Debug(_log, "Connection closed for %s", QSTRING_CSTR(QString("Boblight@%1").arg(connection->getClientAddress())));
	_openConnections.remove(connection);

	// schedule to delete the connection object
	connection->deleteLater();
}

void BoblightServer::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type == settings::BOBLSERVER)
	{
		QJsonObject obj = config.object();
		_port = obj["port"].toInt();
		_priority = obj["priority"].toInt();
		stop();
		if(obj["enable"].toBool())
			start();
	}
}
