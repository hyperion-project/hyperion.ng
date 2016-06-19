#pragma once

// system includes
#include <cstdint>

// Qt includes
#include <QUdpSocket>
#include <QSet>

// Hyperion includes
#include <hyperion/Hyperion.h>

class UDPClientConnection;

///
/// This class creates a UDP server which accepts connections from boblight clients.
///
class UDPListener : public QObject
{
	Q_OBJECT

public:
	///
	/// UDPListener constructor
	/// @param hyperion Hyperion instance
	/// @param port port number on which to start listening for connections
	///
	UDPListener(Hyperion * hyperion, const int priority, uint16_t port = 2801);
	~UDPListener();

	///
	/// @return the port number on which this UDP listens for incoming connections
	///
	uint16_t getPort() const;

private slots:
	///
	/// Slot which is called when a client tries to create a new connection
	///
//	void newConnection();
	void readPendingDatagrams();


	///
	/// Slot which is called when a client closes a connection
	/// @param connection The Connection object which is being closed
	///
//	void closedConnection(UDPClientConnection * connection);

private:
	/// Hyperion instance
	Hyperion * _hyperion;

	/// The UDP server object
	QUdpSocket * _server;

	/// List with open connections
	QSet<UDPClientConnection *> _openConnections;

	/// hyperion priority
	const int _priority;
};
