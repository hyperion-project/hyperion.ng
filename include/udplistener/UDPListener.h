#pragma once

// system includes
#include <cstdint>

// Qt includes
#include <QUdpSocket>
#include <QSet>

// Hyperion includes
#include <hyperion/Hyperion.h>
#include <utils/Logger.h>

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
	UDPListener(const int priority, const int timeout, const std::string& address, quint16 listenPort, bool shared);
	~UDPListener();

	///
	/// @return the port number on which this UDP listens for incoming connections
	///
	uint16_t getPort() const;

private slots:
	///
	/// Slot which is called when a client tries to create a new connection
	///
	void readPendingDatagrams();
	void processTheDatagram(const QByteArray * _datagram);


private:
	/// Hyperion instance
	Hyperion * _hyperion;

	/// The UDP server object
	QUdpSocket * _server;

	/// List with open connections
	QSet<UDPClientConnection *> _openConnections;

	/// hyperion priority
	int _priority;

	/// hyperion priority
	int _timeout;

        /// The latest led color data
        std::vector<ColorRgb> _ledColors;

	/// Logger instance
	Logger * _log;
};
