#pragma once

// system includes
#include <cstdint>

// Qt includes
#include <QTcpServer>
#include <QSet>

// Hyperion includes
#include <hyperion/Hyperion.h>

class ProtoClientConnection;

///
/// This class creates a TCP server which accepts connections wich can then send
/// in Protocol Buffer encoded commands. This interface to Hyperion is used by
/// hyperion-remote to control the leds
///
class ProtoServer : public QObject
{
	Q_OBJECT

public:
	///
	/// ProtoServer constructor
	/// @param hyperion Hyperion instance
	/// @param port port number on which to start listening for connections
	///
	ProtoServer(Hyperion * hyperion, uint16_t port = 19445);
	~ProtoServer();

	///
	/// @return the port number on which this TCP listens for incoming connections
	///
	uint16_t getPort() const;

private slots:
	///
	/// Slot which is called when a client tries to create a new connection
	///
	void newConnection();

	///
	/// Slot which is called when a client closes a connection
	/// @param connection The Connection object which is being closed
	///
	void closedConnection(ProtoClientConnection * connection);

private:
	/// Hyperion instance
	Hyperion * _hyperion;

	/// The TCP server object
	QTcpServer _server;

	/// List with open connections
	QSet<ProtoClientConnection *> _openConnections;
};
