#pragma once

// system includes
#include <cstdint>

// Qt includes
#include <QTcpServer>
#include <QSet>

// Hyperion includes
#include <hyperion/Hyperion.h>

class BoblightClientConnection;

///
/// This class creates a TCP server which accepts connections from boblight clients.
///
class BoblightServer : public QObject
{
	Q_OBJECT

public:
	///
	/// BoblightServer constructor
	/// @param hyperion Hyperion instance
	/// @param port port number on which to start listening for connections
	///
	BoblightServer(Hyperion * hyperion, uint16_t port = 19333);
	~BoblightServer();

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
	void closedConnection(BoblightClientConnection * connection);

private:
	/// Hyperion instance
	Hyperion * _hyperion;

	/// The TCP server object
	QTcpServer _server;

	/// List with open connections
	QSet<BoblightClientConnection *> _openConnections;
};
