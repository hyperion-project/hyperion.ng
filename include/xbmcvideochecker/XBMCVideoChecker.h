
#pragma once

// system includes
#include <cstdint>
#include <string>

// QT includes
#include <QTimer>
#include <QString>
#include <QTcpSocket>
#include <QByteArray>

// Hyperion includes
#include <hyperion/Hyperion.h>

/// This class will check if XBMC is playing something. When it does not, this class will send all black data to Hyperion.
/// This allows grabbed screen data to be overriden while in the XBMC menus.
///
/// Note: The json TCP server needs to be enabled manually in XBMC in System/Settings/Network/Services
class XBMCVideoChecker : public QObject
{
Q_OBJECT

public:
	/// @Constructor
	/// @param address Netwrok address of the XBMC instance
	/// @param port Port number to use (XBMC default = 9090)
	/// @param interval The interval at which XBMC is polled
	/// @param hyperion The Hyperion instance
	/// @param priority The priority at which to send the all black data
	XBMCVideoChecker(const std::string & address, uint16_t port, uint64_t interval, Hyperion * hyperion, int priority);

	/// \brief Start polling XBMC
	void start();

private slots:
	/// \brief Send a request to XBMC
	void sendRequest();

	/// @brief receive a reply from XBMC
	void receiveReply();

private:
	const QString _address;

	const uint16_t _port;

	const QByteArray _request;

	QTimer _timer;

	QTcpSocket _socket;

	Hyperion * _hyperion;

	const int _priority;
};

