#pragma once

// system includes
#include <cstdint>

// Qt includes
#include <QTcpServer>
#include <QSet>

// Hyperion includes
#include <hyperion/Hyperion.h>
#include <utils/Logger.h>
#include <utils/Components.h>

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
	BoblightServer(const int priority, uint16_t port = 19333);
	~BoblightServer();

	///
	/// @return the port number on which this TCP listens for incoming connections
	///
	uint16_t getPort() const;
	
	/// @return true if server is active (bind to a port)
	///
	bool active() { return _isActive; };
	bool componentState() { return active(); };

public slots:
	///
	/// bind server to network
	///
	void start();
	
	///
	/// close server
	///
	void stop();

	void componentStateChanged(const hyperion::Components component, bool enable);

signals:
	void statusChanged(bool isActive);

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

	/// hyperion priority
	const int _priority;

	/// Logger instance
	Logger * _log;

	/// state of connection
	bool _isActive;

	uint16_t  _port;
};
