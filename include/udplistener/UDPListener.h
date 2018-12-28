#pragma once

// system includes
#include <cstdint>

// Qt includes
#include <QSet>
#include <QHostAddress>
#include <QJsonDocument>

// Hyperion includes
#include <utils/Logger.h>
#include <utils/Components.h>
#include <utils/ColorRgb.h>

// settings
#include <utils/settings.h>

class BonjourServiceRegister;
class QUdpSocket;

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
	UDPListener(const QJsonDocument& config);
	~UDPListener();

	///
	/// @return the port number on which this UDP listens for incoming connections
	///
	uint16_t getPort() const;

	///
	/// @return true if server is active (bind to a port)
	///
	bool active() { return _isActive; };

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

	///
	/// @brief Handle settings update from Hyperion Settingsmanager emit or this constructor
	/// @param type   settingyType from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(const settings::type& type, const QJsonDocument& config);

signals:
	///
	/// @brief forward register data to HyperionDaemon
	///
	void registerGlobalInput(const int priority, const hyperion::Components& component, const QString& origin = "System", const QString& owner = "", unsigned smooth_cfg = 0);

	///
	/// @brief forward led data to HyperionDaemon
	///
	const bool setGlobalInput(const int priority, const std::vector<ColorRgb>& ledColors, const int timeout_ms = -1, const bool& clearEffect = true);

	///
	/// @brief forward clear to HyperionDaemon
	///
	void clearGlobalPriority(const int& _priority, const hyperion::Components& component);

private slots:
	///
	/// Slot which is called when a client tries to create a new connection
	///
	void readPendingDatagrams();
	void processTheDatagram(const QByteArray * datagram, const QHostAddress * sender);

private:

	/// The UDP server object
	QUdpSocket * _server;

	/// hyperion priority
	int _priority;

	/// hyperion timeout
	int _timeout;

	/// Logger instance
	Logger * _log;

	/// Bonjour Service Register
	BonjourServiceRegister* _serviceRegister = nullptr;

	/// state of connection
	bool _isActive;

	/// address to bind
	QHostAddress              _listenAddress;
	uint16_t                  _listenPort;
	QAbstractSocket::BindFlag _bondage;
};
