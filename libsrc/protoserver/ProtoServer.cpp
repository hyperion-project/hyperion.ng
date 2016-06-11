// system includes
#include <stdexcept>

// project includes
#include <hyperion/MessageForwarder.h>
#include <protoserver/ProtoServer.h>
#include "protoserver/ProtoConnection.h"
#include "ProtoClientConnection.h"

ProtoServer::ProtoServer(Hyperion *hyperion, uint16_t port) :
	QObject(),
	_hyperion(hyperion),
	_server(),
	_openConnections()
{

	MessageForwarder * forwarder = hyperion->getForwarder();
	QStringList slaves = forwarder->getProtoSlaves();

	for (int i = 0; i < slaves.size(); ++i) {
		if ( QString("127.0.0.1:%1").arg(port) == slaves.at(i) ) {
			throw std::runtime_error("PROTOSERVER ERROR: Loop between proto server and forwarder detected. Fix your config!");
		}

		ProtoConnection* p = new ProtoConnection(slaves.at(i).toLocal8Bit().constData());
		p->setSkipReply(true);
		_proxy_connections << p;
	}

	if (!_server.listen(QHostAddress::Any, port))
	{
		throw std::runtime_error("PROTOSERVER ERROR: Could not bind to port");
	}

	// Set trigger for incoming connections
	connect(&_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

ProtoServer::~ProtoServer()
{
	foreach (ProtoClientConnection * connection, _openConnections) {
		delete connection;
	}
	
	while (!_proxy_connections.isEmpty())
		delete _proxy_connections.takeFirst();
}

uint16_t ProtoServer::getPort() const
{
	return _server.serverPort();
}

void ProtoServer::newConnection()
{
	QTcpSocket * socket = _server.nextPendingConnection();

	if (socket != nullptr)
	{
		std::cout << "PROTOSERVER INFO: New connection" << std::endl;
		ProtoClientConnection * connection = new ProtoClientConnection(socket, _hyperion);
		_openConnections.insert(connection);

		// register slot for cleaning up after the connection closed
		connect(connection, SIGNAL(connectionClosed(ProtoClientConnection*)), this, SLOT(closedConnection(ProtoClientConnection*)));
		connect(connection, SIGNAL(newMessage(const proto::HyperionRequest*)), this, SLOT(newMessage(const proto::HyperionRequest*)));
		
		// register forward signal for xbmc checker
		connect(this, SIGNAL(grabbingMode(GrabbingMode)), connection, SLOT(setGrabbingMode(GrabbingMode)));
		connect(this, SIGNAL(videoMode(VideoMode)), connection, SLOT(setVideoMode(VideoMode)));

	}
}

void ProtoServer::newMessage(const proto::HyperionRequest * message)
{
	for (int i = 0; i < _proxy_connections.size(); ++i)
		_proxy_connections.at(i)->sendMessage(*message);
}

void ProtoServer::sendImageToProtoSlaves(int priority, const Image<ColorRgb> & image, int duration_ms)
{
	for (int i = 0; i < _proxy_connections.size(); ++i)
		_proxy_connections.at(i)->setImage(image, priority, duration_ms);
}

void ProtoServer::closedConnection(ProtoClientConnection *connection)
{
	std::cout << "PROTOSERVER INFO: Connection closed" << std::endl;
	_openConnections.remove(connection);

	// schedule to delete the connection object
	connection->deleteLater();
}
