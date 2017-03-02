#pragma once

// system includes
#include <cstdint>

// Qt includes
#include <QUdpSocket>
#include <QSet>
#include <QHostAddress>

// Hyperion includes
#include <hyperion/Hyperion.h>
#include <utils/Logger.h>
#include <utils/Components.h>

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
	UDPListener(const int priority, const int timeout, const QString& address, quint16 listenPort, bool shared);
	~UDPListener();

	///
	/// @return the port number on which this UDP listens for incoming connections
	///
	uint16_t getPort() const;
	
	///
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
	void readPendingDatagrams();
	void processTheDatagram(const QByteArray * datagram, const QHostAddress * sender);

private:
	/// Hyperion instance
	Hyperion * _hyperion;

	/// The UDP server object
	QUdpSocket * _server;

	/// List with open connections
	QSet<UDPClientConnection *> _openConnections;

	/// hyperion priority
	int _priority;

	/// hyperion timeout
	int _timeout;

	/// Logger instance
	Logger * _log;
	
	/// state of connection
	bool _isActive;
	
	/// address to bind
	QHostAddress              _listenAddress;
	quint16                   _listenPort;
	QAbstractSocket::BindFlag _bondage;
};
