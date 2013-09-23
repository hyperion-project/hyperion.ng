
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

// Utils includes
#include <utils/GrabbingMode.h>

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
	/// @param grabVideo Whether or not to grab when the XBMC video player is playing
	/// @param grabPhoto Whether or not to grab when the XBMC photo player is playing
	/// @param grabAudio Whether or not to grab when the XBMC audio player is playing
	/// @param grabMenu Whether or not to grab when nothing is playing (in XBMC menu)
	///
	XBMCVideoChecker(const std::string & address, uint16_t port, uint64_t interval, bool grabVideo, bool grabPhoto, bool grabAudio, bool grabMenu);

	///
	/// Start polling XBMC
	///
	void start();

signals:
	void grabbingMode(GrabbingMode grabbingMode);

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

	/// Flag indicating whether or not to grab when the XBMC video player is playing
	bool _grabVideo;

	/// Flag indicating whether or not to grab when the XBMC photo player is playing
	bool _grabPhoto;

	/// Flag indicating whether or not to grab when the XBMC audio player is playing
	bool _grabAudio;

	/// Flag indicating whether or not to grab when XBMC is playing nothing (in menu)
	bool _grabMenu;

	/// Previous emitted grab state
	GrabbingMode _previousMode;
};
