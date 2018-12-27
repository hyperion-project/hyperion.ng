#pragma once

// system includes
#include <cstdint>

// Qt includes
#include <QSet>
#include <QList>
#include <QStringList>
#include <QJsonDocument>

// hyperion includes
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/VideoMode.h>
#include <utils/Logger.h>
#include <utils/Components.h>

// settings
#include <utils/settings.h>

// forward decl
class ProtoClientConnection;
class ProtoConnection;
class QTcpServer;
class Hyperion;
class BonjourServiceRegister;
class ComponentRegister;
class NetOrigin;

namespace proto {
class HyperionRequest;
}

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
	/// @param config the configuration
	///
	ProtoServer(const QJsonDocument& config);
	~ProtoServer();

	///
	/// @return the port number on which this TCP listens for incoming connections
	///
	uint16_t getPort() const;

public slots:
	void sendImageToProtoSlaves(int priority, const Image<ColorRgb> & image, int duration_ms);
	void componentStateChanged(const hyperion::Components component, bool enable);

	///
	/// @brief Handle settings update from Hyperion Settingsmanager emit or this constructor
	/// @param type   settingyType from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(const settings::type& type, const QJsonDocument& config);

signals:
	///
	/// Forwarding videoMode
	///
	void videoMode(const VideoMode VideoMode);

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

	void newMessage(const proto::HyperionRequest * message);

private:
	/// Hyperion instance
	Hyperion * _hyperion;

	/// The TCP server object
	QTcpServer * _server;

	/// List with open connections
	QSet<ProtoClientConnection *> _openConnections;
	QStringList _forwardClients;

	/// Hyperion proto connection object for forwarding
	QList<ProtoConnection*> _proxy_connections;

	/// Logger instance
	Logger * _log;

	/// Component Register
	ComponentRegister* _componentRegister;

	/// Network Origin Check
	NetOrigin* _netOrigin;

	/// Service register
	BonjourServiceRegister * _serviceRegister = nullptr;

	/// flag if forwarder is enabled
	bool _forwarder_enabled;

	uint16_t _port = 0;

	/// Start server
	void start();
	/// Stop server
	void stop();
};
