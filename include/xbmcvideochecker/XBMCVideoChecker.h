
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

///
/// This class will check if XBMC is playing something. When it does not, this class will send all black data to Hyperion.
/// This allows grabbed screen data to be overriden while in the XBMC menus.
///
/// Note: The json TCP server needs to be enabled manually in XBMC in System/Settings/Network/Services
///
class XBMCVideoChecker : public QObject
{
Q_OBJECT

public:
	///
	/// Constructor
	///
	/// @param address Network address of the XBMC instance
	/// @param port Port number to use (XBMC default = 9090)
	/// @param interval The interval at which XBMC is polled
	/// @param hyperion The Hyperion instance
	/// @param priority The priority at which to send the all black data
	///
	XBMCVideoChecker(const std::string & address, uint16_t port, uint64_t interval, Hyperion * hyperion, int priority);

	///
	/// Start polling XBMC
	///
	void start();

private slots:
	///
	/// Send a request to XBMC
	///
	void sendRequest();

	///
	/// Receive a reply from XBMC
	///
	void receiveReply();

private:
	/// The network address of the XBMC instance
	const QString _address;

	/// The port number of XBMC
	const uint16_t _port;

	/// The JSON-RPC request message
	const QByteArray _request;

	/// The timer that schedules XBMC queries
	QTimer _timer;

	/// The QT TCP Socket with connection to XBMC
	QTcpSocket _socket;

	/// The Hyperion instance to switch leds to black if in XBMC menu
	Hyperion * _hyperion;

	/// The priority of the BLACK led value when in XBMC menu
	const int _priority;
};
