#pragma once

// Qt includes
#include <QTcpServer>
#include <QSet>

// Hyperion includes
#include <hyperion/Hyperion.h>
#include <utils/Logger.h>

class JsonClientConnection;

///
/// This class creates a TCP server which accepts connections wich can then send
/// in JSON encoded commands. This interface to Hyperion is used by hyperion-remote
/// to control the leds
///
class JsonServer : public QObject
{
	Q_OBJECT

public:
	///
	/// JsonServer constructor
	/// @param hyperion Hyperion instance
	/// @param port port number on which to start listening for connections
	///
	JsonServer(uint16_t port = 19444);
	~JsonServer();

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
	///
	void closedConnection(void);

	/// forward message to all json slaves
	void forwardJsonMessage(const QJsonObject &message);

public slots:
	/// process current forwarder state
	void componentStateChanged(const hyperion::Components component, bool enable);

	///
	/// forward message to a single json slaves
	///
	/// @param message The JSON message to send
	///
	void sendMessage(const QJsonObject & message, QTcpSocket * socket);

private:
	/// The TCP server object
	QTcpServer _server;

	/// Link to Hyperion to get config state emiter
	Hyperion * _hyperion;

	/// List with open connections
	QSet<JsonClientConnection *> _openConnections;

	/// the logger instance
	Logger * _log;

	/// Flag if forwarder is enabled
	bool _forwarder_enabled = true;
};
