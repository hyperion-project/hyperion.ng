
#pragma once

// system includes
#include <cstdint>

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
#include <utils/Logger.h>
#include <utils/Components.h>

///
/// This class will check if KODI is playing something. When it does not, this class will send all black data to Hyperion.
/// This allows grabbed screen data to be overriden while in the KODI menus.
///
/// Note: The json TCP server needs to be enabled manually in KODI in System/Settings/Network/Services
///
class KODIVideoChecker : public QObject
{
Q_OBJECT

public:
	static KODIVideoChecker* initInstance(const QString & address, uint16_t port, bool grabVideo, bool grabPhoto, bool grabAudio, bool grabMenu, bool grabPause, bool grabScreensaver, bool enable3DDetection);
	static KODIVideoChecker* getInstance();

	~KODIVideoChecker();
	void setConfig(const QString & address, uint16_t port, bool grabVideo, bool grabPhoto, bool grabAudio, bool grabMenu, bool grabPause, bool grabScreensaver, bool enable3DDetection);

	bool componentState() { return _active; }

public slots:
	///
	/// Start polling KODI
	///
	void start();

	///
	/// Stop polling KODI
	///
	void stop();

	void componentStateChanged(const hyperion::Components component, bool enable);

signals:
	/// Signal emitted when the grabbing mode changes
	void grabbingMode(GrabbingMode grabbingMode);

	/// Signal emitted when a 3D movie is detected
	void videoMode(VideoMode videoMode);

private slots:
	/// Receive a reply from KODI
	void receiveReply();

	/// Called when connected to KODI
	void connected();

	/// Called when disconnected from KODI
	void disconnected();

	/// reconnect to KODI
	void reconnect();

	/// Called when a connection error was encountered
	void connectionError(QAbstractSocket::SocketError error);

private:
	///
	/// Constructor
	///
	/// @param address Network address of the KODI instance
	/// @param port Port number to use (KODI default = 9090)
	/// @param grabVideo Whether or not to grab when the KODI video player is playing
	/// @param grabPhoto Whether or not to grab when the KODI photo player is playing
	/// @param grabAudio Whether or not to grab when the KODI audio player is playing
	/// @param grabMenu Whether or not to grab when nothing is playing (in KODI menu)
	/// @param grabScreensaver Whether or not to grab when the KODI screensaver is activated
	/// @param enable3DDetection Wheter or not to enable the detection of 3D movies playing
	///
	KODIVideoChecker(const QString & address, uint16_t port, bool grabVideo, bool grabPhoto, bool grabAudio, bool grabMenu, bool grabPause, bool grabScreensaver, bool enable3DDetection);
	
	/// Set the grabbing mode
	void setGrabbingMode(GrabbingMode grabbingMode);
	
	/// Set the video mode
	void setVideoMode(VideoMode videoMode);

private:
	/// The network address of the KODI instance
	QString _address;

	/// The port number of KODI
	uint16_t _port;

	/// The JSON-RPC message to check the active player
	const QString _activePlayerRequest;

	/// The JSON-RPC message to check the currently playing file
	const QString _currentPlayingItemRequest;

	/// The JSON-RPC message to check the screensaver
	const QString _checkScreensaverRequest;

	/// The JSON-RPC message to check the active stereoscopicmode
	const QString _getStereoscopicMode;

	/// The JSON-RPC message to check the kodi version
	QString _getKodiVersion;
	
	/// The JSON-RPC message to check the current Playback State
	const QString _getCurrentPlaybackState; 

	/// The QT TCP Socket with connection to KODI
	QTcpSocket _socket;

	/// Flag indicating whether or not to grab when the KODI video player is playing
	bool _grabVideo;

	/// Flag indicating whether or not to grab when the KODI photo player is playing
	bool _grabPhoto;

	/// Flag indicating whether or not to grab when the KODI audio player is playing
	bool _grabAudio;

	/// Flag indicating whether or not to grab when KODI is playing nothing (in menu)
	bool _grabMenu;

	/// Flag indicating whether or not to grab when the KODI videoplayer is at pause state
	bool _grabPause;
	
	/// Flag indicating whether or not to grab when the KODI screensaver is activated
	bool _grabScreensaver;

	/// Flag indicating wheter or not to enable the detection of 3D movies playing
	bool _enable3DDetection;
	
	/// Previous emitted grab mode
	GrabbingMode _previousGrabbingMode;

	/// Previous emitted video mode
	VideoMode _previousVideoMode;
	
	/// Current Playback State
	bool _currentPlaybackState;
	
	/// Current Kodi PlayerID
	int _currentPlayerID;

	/// KODI version number
	int _kodiVersion;

	/// Logger Instance
	Logger * _log;
	
	/// flag indicating state
	bool _active;

	/// flag indicates if playbackState is valid
	bool _getCurrentPlaybackStateInitialized;

	static KODIVideoChecker* _kodichecker;
};
