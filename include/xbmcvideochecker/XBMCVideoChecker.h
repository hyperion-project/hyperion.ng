
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
#include <utils/VideoMode.h>

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
	/// @param grabVideo Whether or not to grab when the XBMC video player is playing
	/// @param grabPhoto Whether or not to grab when the XBMC photo player is playing
	/// @param grabAudio Whether or not to grab when the XBMC audio player is playing
	/// @param grabMenu Whether or not to grab when nothing is playing (in XBMC menu)
	/// @param grabScreensaver Whether or not to grab when the XBMC screensaver is activated
	/// @param enable3DDetection Wheter or not to enable the detection of 3D movies playing
	///
	XBMCVideoChecker(const std::string & address, uint16_t port, bool grabVideo, bool grabPhoto, bool grabAudio, bool grabMenu, bool grabScreensaver, bool enable3DDetection);

	///
	/// Start polling XBMC
	///
	void start();

signals:
	/// Signal emitted when the grabbing mode changes
	void grabbingMode(GrabbingMode grabbingMode);

	/// Signal emitted when a 3D movie is detected
	void videoMode(VideoMode videoMode);

private slots:
	/// Receive a reply from XBMC
	void receiveReply();

	/// Called when connected to XBMC
	void connected();

	/// Called when disconnected from XBMC
	void disconnected();

	/// reconnect to XBMC
	void reconnect();

	/// Called when a connection error was encountered
	void connectionError(QAbstractSocket::SocketError error);

private:
	/// Set the grabbing mode
	void setGrabbingMode(GrabbingMode grabbingMode);

	void setScreensaverMode(bool isOnScreensaver);

	/// Set the video mode
	void setVideoMode(VideoMode videoMode);

private:
	/// The network address of the XBMC instance
	const QString _address;

	/// The port number of XBMC
	const uint16_t _port;

	/// The JSON-RPC message to check the active player
	const QString _activePlayerRequest;

	/// The JSON-RPC message to check the currently playing file
	const QString _currentPlayingItemRequest;

	/// The JSON-RPC message to check the screensaver
	const QString _checkScreensaverRequest;

	/// The JSON-RPC message to check the active stereoscopicmode
	const QString _getStereoscopicMode;

	/// The JSON-RPC message to check the xbmc version
	const QString _getXbmcVersion;

	/// The QT TCP Socket with connection to XBMC
	QTcpSocket _socket;

	/// Flag indicating whether or not to grab when the XBMC video player is playing
	const bool _grabVideo;

	/// Flag indicating whether or not to grab when the XBMC photo player is playing
	const bool _grabPhoto;

	/// Flag indicating whether or not to grab when the XBMC audio player is playing
	const bool _grabAudio;

	/// Flag indicating whether or not to grab when XBMC is playing nothing (in menu)
	const bool _grabMenu;

	/// Flag indicating whether or not to grab when the XBMC screensaver is activated
	const bool _grabScreensaver;

	/// Flag indicating wheter or not to enable the detection of 3D movies playing
	const bool _enable3DDetection;

	/// Flag indicating if XBMC is on screensaver
	bool _previousScreensaverMode;

	/// Previous emitted grab mode
	GrabbingMode _previousGrabbingMode;

	/// Previous emitted video mode
	VideoMode _previousVideoMode;

	/// XBMC version number
	int _xbmcVersion;
};
