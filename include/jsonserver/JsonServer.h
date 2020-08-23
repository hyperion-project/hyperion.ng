#pragma once

// Qt includes
#include <QSet>

// Hyperion includes
#include <utils/Components.h>
#include <utils/Logger.h>
#include <utils/settings.h>

class QTcpServer;
class QTcpSocket;
class JsonClientConnection;
class BonjourServiceRegister;
class NetOrigin;

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
	/// @param The configuration
	///
	JsonServer(const QJsonDocument& config);
	~JsonServer() override;

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
	void closedConnection();

public slots:
	///
	/// @brief Handle settings update from Hyperion Settingsmanager emit or this constructor
	/// @param type   settings type from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(settings::type type, const QJsonDocument& config);

private:
	/// The TCP server object
	QTcpServer * _server;

	/// List with open connections
	QSet<JsonClientConnection *> _openConnections;

	/// the logger instance
	Logger * _log;

	NetOrigin* _netOrigin;

	/// port
	uint16_t _port = 0;

	BonjourServiceRegister * _serviceRegister = nullptr;

	void start();
	void stop();
};
