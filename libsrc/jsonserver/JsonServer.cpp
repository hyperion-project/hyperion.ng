// system includes
#include <stdexcept>

// project includes
#include <jsonserver/JsonServer.h>
#include "JsonClientConnection.h"

// hyperion include
#include <hyperion/Hyperion.h>
#include <hyperion/MessageForwarder.h>
#include <bonjour/bonjourserviceregister.h>
#include <hyperion/ComponentRegister.h>

// qt includes
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QByteArray>

JsonServer::JsonServer(const QJsonDocument& config)
	: QObject()
	, _server(new QTcpServer(this))
	, _hyperion(Hyperion::getInstance())
	, _openConnections()
	, _log(Logger::getInstance("JSONSERVER"))
	, _componentRegister( & _hyperion->getComponentRegister())
{
	Debug(_log, "Created instance");

	// Set trigger for incoming connections
	connect(_server, SIGNAL(newConnection()), this, SLOT(newConnection()));

	// receive state of forwarder
	connect(_componentRegister, &ComponentRegister::updatedComponentState, this, &JsonServer::componentStateChanged);

	// listen for component register changes
	connect(_hyperion, &Hyperion::forwardJsonMessage, this, &JsonServer::forwardJsonMessage);

	// init
	handleSettingsUpdate(settings::JSONSERVER, config);

	// set initial state of forwarding
	componentStateChanged(hyperion::COMP_FORWARDER, _componentRegister->isComponentEnabled(hyperion::COMP_FORWARDER));
}

JsonServer::~JsonServer()
{
	foreach (JsonClientConnection * connection, _openConnections) {
		delete connection;
	}
}

void JsonServer::start()
{
	if(_server->isListening())
		return;

	if (!_server->listen(QHostAddress::Any, _port))
	{
		Error(_log,"Could not bind to port '%d', please use an available port", _port);
		return;
	}
	Info(_log, "Started on port %d", _port);

	if(_serviceRegister == nullptr)
	{
		_serviceRegister = new BonjourServiceRegister();
		_serviceRegister->registerService("_hyperiond-json._tcp", _port);
	}
}

void JsonServer::stop()
{
	if(!_server->isListening())
		return;

	_server->close();
	Info(_log, "Stopped");
}

void JsonServer::handleSettingsUpdate(const settings::type& type, const QJsonDocument& config)
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
			Debug(_log, "New connection from: %s ",socket->localAddress().toString().toStdString().c_str());
			JsonClientConnection * connection = new JsonClientConnection(socket);
			_openConnections.insert(connection);

			// register slot for cleaning up after the connection closed
			connect(connection, &JsonClientConnection::connectionClosed, this, &JsonServer::closedConnection);
		}
	}
}

void JsonServer::closedConnection(void)
{
	JsonClientConnection* connection = qobject_cast<JsonClientConnection*>(sender());
	Debug(_log, "Connection closed");
	_openConnections.remove(connection);

	// schedule to delete the connection object
	connection->deleteLater();
}

void JsonServer::componentStateChanged(const hyperion::Components component, bool enable)
{
	if (component == hyperion::COMP_FORWARDER)
	{
		if(enable)
		{
			connect(_hyperion, &Hyperion::forwardJsonMessage, this, &JsonServer::forwardJsonMessage);
		}
		else
		{
			disconnect(_hyperion, &Hyperion::forwardJsonMessage, this, &JsonServer::forwardJsonMessage);
		}
	}
}

void JsonServer::forwardJsonMessage(const QJsonObject &message)
{
	QTcpSocket client;
	QStringList list = _hyperion->getForwarder()->getJsonSlaves();

	for (const auto& entry : list)
	{
		QStringList splitted = entry.split(":");
		client.connectToHost(splitted[0], splitted[1].toInt());
		if ( client.waitForConnected(500) )
		{
			sendMessage(message,&client);
			client.close();
		}
	}
}

void JsonServer::sendMessage(const QJsonObject & message, QTcpSocket * socket)
{
	// serialize message
	QJsonDocument writer(message);
	QByteArray serializedMessage = writer.toJson(QJsonDocument::Compact) + "\n";

	// write message
	socket->write(serializedMessage);
	if (!socket->waitForBytesWritten())
	{
		Debug(_log, "Error while writing data to host");
		return;
	}

	// read reply data
	QByteArray serializedReply;
	while (!serializedReply.contains('\n'))
	{
		// receive reply
		if (!socket->waitForReadyRead())
		{
			Debug(_log, "Error while writing data from host");
			return;
		}

		serializedReply += socket->readAll();
	}

	// parse reply data
	QJsonParseError error;
	QJsonDocument reply = QJsonDocument::fromJson(serializedReply ,&error);

	if (error.error != QJsonParseError::NoError)
	{
		Error(_log, "Error while parsing reply: invalid json");
		return;
	}

}
