// Qt includes
#include <QUrl>
#include <QRegExp>
#include <QStringRef>

#include <kodivideochecker/KODIVideoChecker.h>


KODIVideoChecker* KODIVideoChecker::_kodichecker = nullptr;

KODIVideoChecker* KODIVideoChecker::initInstance(const std::string & address, uint16_t port, bool grabVideo, bool grabPhoto, bool grabAudio, bool grabMenu, bool grabPause, bool grabScreensaver, bool enable3DDetection)
{
	if ( KODIVideoChecker::_kodichecker != nullptr )
		throw std::runtime_error("KODIVideoChecker::initInstance can be called only one time");
	KODIVideoChecker::_kodichecker = new KODIVideoChecker(address, port, grabVideo, grabPhoto, grabAudio, grabMenu, grabPause, grabScreensaver, enable3DDetection);

	return KODIVideoChecker::_kodichecker;
}

KODIVideoChecker* KODIVideoChecker::getInstance()
{
	return KODIVideoChecker::_kodichecker;
}


// Request player example:
// {"jsonrpc":"2.0","method":"Player.GetActivePlayers", "id":666}
// {"id":666,"jsonrpc":"2.0","result":[{"playerid":1,"type":"video"}]}

// Request playing item example:
// {"id":667,"jsonrpc":"2.0","method":"Player.GetItem","params":{"playerid":1,"properties":["file"]}}
// {"id":667,"jsonrpc":"2.0","result":{"item":{"file":"smb://xbmc:xbmc@192.168.53.12/video/Movies/Avatar (2009)/Avatar.mkv","label":"Avatar","type":"unknown"}}}

// Request if screensaver is on
// {"id":668,"jsonrpc":"2.0","method":"XBMC.GetInfoBooleans","params":{"booleans":["System.ScreenSaverActive"]}}
// {"id":668,"jsonrpc":"2.0","result":{"System.ScreenSaverActive":false}}

// Request stereoscopicmode example:
// {"jsonrpc":"2.0","method":"GUI.GetProperties","params":{"properties":["stereoscopicmode"]},"id":669}
// {"id":669,"jsonrpc":"2.0","result":{"stereoscopicmode":{"label":"Nebeneinander","mode":"split_vertical"}}}

KODIVideoChecker::KODIVideoChecker(const std::string & address, uint16_t port, bool grabVideo, bool grabPhoto, bool grabAudio, bool grabMenu, bool grabPause, bool grabScreensaver, bool enable3DDetection)
	: QObject()
	, _address(QString::fromStdString(address))
	, _port(port)
	, _activePlayerRequest(R"({"jsonrpc":"2.0","method":"Player.GetActivePlayers", "id":666})")
	, _currentPlayingItemRequest(R"({"id":667,"jsonrpc":"2.0","method":"Player.GetItem","params":{"playerid":%1,"properties":["file"]}})")
	, _checkScreensaverRequest(R"({"id":668,"jsonrpc":"2.0","method":"XBMC.GetInfoBooleans","params":{"booleans":["System.ScreenSaverActive"]}})")
	, _getStereoscopicMode(R"({"jsonrpc":"2.0","method":"GUI.GetProperties","params":{"properties":["stereoscopicmode"]},"id":669})")
	, _getKodiVersion(R"({"jsonrpc":"2.0","method":"Application.GetProperties","params":{"properties":["version"]},"id":670})")
	, _socket()
	, _grabVideo(grabVideo)
	, _grabPhoto(grabPhoto)
	, _grabAudio(grabAudio)
	, _grabMenu(grabMenu)
	, _grabPause(grabPause)
	, _grabScreensaver(grabScreensaver)
	, _enable3DDetection(enable3DDetection)
	, _previousScreensaverMode(false)
	, _previousGrabbingMode(GRABBINGMODE_INVALID)
	, _previousVideoMode(VIDEO_2D)
	, _kodiVersion(0)
	, _log(Logger::getInstance("KODI"))
	, _active(false)
{
	// setup socket
	connect(&_socket, SIGNAL(readyRead()), this, SLOT(receiveReply()));
	connect(&_socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
	connect(&_socket, SIGNAL(connected()), this, SLOT(connected()));
	connect(&_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connectionError(QAbstractSocket::SocketError)));
}

KODIVideoChecker::~KODIVideoChecker()
{
	stop();
}


void KODIVideoChecker::setConfig(const std::string & address, uint16_t port, bool grabVideo, bool grabPhoto, bool grabAudio, bool grabMenu, bool grabPause, bool grabScreensaver, bool enable3DDetection)
{
	_address                 = QString::fromStdString(address);
	_port                    = port;
	_grabVideo               = grabVideo;
	_grabPhoto               = grabPhoto;
	_grabAudio               = grabAudio;
	_grabMenu                = grabMenu;
	_grabPause               = grabPause;
	_grabScreensaver         = grabScreensaver;
	_enable3DDetection       = enable3DDetection;
	_previousScreensaverMode = false;
	_previousGrabbingMode    = GRABBINGMODE_INVALID;
	_previousVideoMode       = VIDEO_2D;
	_kodiVersion             = 0;

	// restart if active
	if (_active)
	{
		stop();
		QTimer::singleShot(2000, this, SLOT(()));
	}
}


void KODIVideoChecker::start()
{
	Info(_log, "started");
	_active = true;
	reconnect();
}

void KODIVideoChecker::stop()
{
	Info(_log, "stopped");
	_active = false;
	_socket.close();
}

void KODIVideoChecker::receiveReply()
{
	// expect that the reply is received as a single message. Probably oke considering the size of the expected reply
	QString reply(_socket.readAll());
	Debug(_log, "message: %s", reply.toStdString().c_str());

	if (reply.contains("\"method\":\"Player.OnPlay\""))
	{
		// send a request for the current player state
		_socket.write(_activePlayerRequest.toUtf8());
		return;
	}
	else if (reply.contains("\"method\":\"Player.OnStop\""))
	{
		// the player has stopped
		setGrabbingMode(_grabMenu ? GRABBINGMODE_MENU : GRABBINGMODE_OFF);
		setVideoMode(VIDEO_2D);
	}
	else if (reply.contains("\"method\":\"Player.OnPause\""))
	{
		// player at pause
		setGrabbingMode(_grabPause ? GRABBINGMODE_PAUSE : GRABBINGMODE_OFF);
	}
	else if (reply.contains("\"method\":\"GUI.OnScreensaverActivated\""))
	{
		setScreensaverMode(!_grabScreensaver);
	}
	else if (reply.contains("\"method\":\"GUI.OnScreensaverDeactivated\""))
	{
		setScreensaverMode(false);
	}
	else if (reply.contains("\"id\":666"))
	{
		// Result of Player.GetActivePlayers

		// always start a new video in 2D mode
		emit videoMode(VIDEO_2D);

		if (reply.contains("video"))
		{
			// video is playing
			setGrabbingMode(_grabVideo ? GRABBINGMODE_VIDEO : GRABBINGMODE_OFF);

			// we need to get the filename
			// first retrieve the playerid
			QString key = "\"playerid\":";
			QRegExp regex(key + "(\\d+)");
			int pos = regex.indexIn(reply);
			if (pos > 0)
			{
				// now request info of the playing item
				QStringRef idText(&reply, pos + key.length(), regex.matchedLength() - key.length());
				_socket.write(_currentPlayingItemRequest.arg(idText.toString()).toUtf8());
			}
		}
		else if (reply.contains("picture"))
		{
			// picture viewer is playing
			setGrabbingMode(_grabPhoto ? GRABBINGMODE_PHOTO : GRABBINGMODE_OFF);
		}
		else if (reply.contains("audio"))
		{
			// audio is playing
			setGrabbingMode(_grabAudio ? GRABBINGMODE_AUDIO : GRABBINGMODE_OFF);
		}
		else
		{
			// Nothing is playing.
			setGrabbingMode(_grabMenu ? GRABBINGMODE_MENU : GRABBINGMODE_OFF);
		}
	}
	else if (reply.contains("\"id\":667"))
	{
		if (_kodiVersion >= 13)
		{
			// check of active stereoscopicmode
			_socket.write(_getStereoscopicMode.toUtf8());
		}
		else
		{
			// result of Player.GetItem
			// TODO: what if the filename contains a '"'. In Json this should have been escaped
			QRegExp regex("\"file\":\"((?!\").)*\"");
			int pos = regex.indexIn(reply);
			if (pos > 0)
			{
				QStringRef filename = QStringRef(&reply, pos+8, regex.matchedLength()-9);
				if (filename.contains("3DSBS", Qt::CaseInsensitive) || filename.contains("HSBS", Qt::CaseInsensitive))
				{
					setVideoMode(VIDEO_3DSBS);
				}
				else if (filename.contains("3DTAB", Qt::CaseInsensitive) || filename.contains("HTAB", Qt::CaseInsensitive))
				{
					setVideoMode(VIDEO_3DTAB);
				}
				else
				{
					setVideoMode(VIDEO_2D);
				}
			}
		}
	}
	else if (reply.contains("\"id\":668"))
	{
		// result of System.ScreenSaverActive
		bool active = reply.contains("\"System.ScreenSaverActive\":true");
		setScreensaverMode(!_grabScreensaver && active);

		// check here kodi version
		if (_socket.state() == QTcpSocket::ConnectedState)
		{
			if (_kodiVersion == 0)
			{
				_socket.write(_getKodiVersion.toUtf8());
			}
		}
	}
	else if (reply.contains("\"id\":669"))
	{
		QRegExp regex("\"mode\":\"(split_vertical|split_horizontal)\"");
		int pos = regex.indexIn(reply);
		if (pos > 0)
		{
			QString sMode = regex.cap(1);
			if (sMode == "split_vertical")
			{
				setVideoMode(VIDEO_3DSBS);
			}
			else if (sMode == "split_horizontal")
			{
				setVideoMode(VIDEO_3DTAB);
			}
		}
	}
	else if (reply.contains("\"id\":670"))
	{
		QRegExp regex("\"major\":(\\d+)");
		int pos = regex.indexIn(reply);
		if (pos > 0)
		{
			_kodiVersion = regex.cap(1).toInt();
		}
	}
	else if (reply.contains("picture") && reply.contains("\"method\":\"Playlist.OnAdd\""))
	{
		// picture viewer is playing
		setGrabbingMode(_grabPhoto ? GRABBINGMODE_PHOTO : GRABBINGMODE_OFF);
	}
}

void KODIVideoChecker::connected()
{
	Info(_log, "Connected");

	// send a request for the current player state
	_socket.write(_activePlayerRequest.toUtf8());
	_socket.write(_checkScreensaverRequest.toUtf8());
}

void KODIVideoChecker::disconnected()
{
	Info(_log, "Disconnected");
	reconnect();
}

void KODIVideoChecker::reconnect()
{
	if (_socket.state() == QTcpSocket::ConnectedState || ! _active )
	{
		return;
	}
	Debug(_log, "try reconnect");

	// try to connect
	switch (_socket.state())
	{
	case QTcpSocket::ConnectingState:
		// Somehow when KODI restarts we get stuck in connecting state
		// If we get here we tried to connect already for 5 seconds. abort and try again in 1 second.
		_socket.reset();
		_socket.waitForDisconnected(50);
		QTimer::singleShot(1000, this, SLOT(reconnect()));
		break;
	case QTcpSocket::UnconnectedState:
		_socket.connectToHost(_address, _port);
		QTimer::singleShot(10000, this, SLOT(reconnect()));
		break;
	default:
		QTimer::singleShot(10000, this, SLOT(reconnect()));
		break;
	}
}

void KODIVideoChecker::connectionError(QAbstractSocket::SocketError error)
{
	Error(_log,"Connection error (%s)", error);

	// close the socket
	_socket.close();
}

void KODIVideoChecker::setGrabbingMode(GrabbingMode newGrabbingMode)
{
	if (newGrabbingMode == _previousGrabbingMode)
	{
		// no change
		return;
	}

	switch (newGrabbingMode)
	{
	case GRABBINGMODE_VIDEO:
		Info(_log, "switching to VIDEO mode");
		break;
	case GRABBINGMODE_PHOTO:
		Info(_log, "switching to PHOTO mode");
		break;
	case GRABBINGMODE_AUDIO:
		Info(_log, "switching to AUDIO mode");
		break;
	case GRABBINGMODE_MENU:
		Info(_log, "switching to MENU mode");
		break;
	case GRABBINGMODE_PAUSE:
		Info(_log, "switching to PAUSE mode");
		break;
	case GRABBINGMODE_OFF:
		Info(_log, "switching to OFF mode");
		break;
	default:
		Warning(_log, "switching to INVALID mode");
		break;
	}

	// only emit the new state when we want to grab in screensaver mode or when the screensaver is deactivated
	if (!_previousScreensaverMode)
	{
		emit grabbingMode(newGrabbingMode);
	}
	_previousGrabbingMode = newGrabbingMode;
}

void KODIVideoChecker::setScreensaverMode(bool isOnScreensaver)
{
	if (isOnScreensaver == _previousScreensaverMode)
	{
		// no change
		return;
	}

	emit grabbingMode(isOnScreensaver ? GRABBINGMODE_OFF : _previousGrabbingMode);
	_previousScreensaverMode = isOnScreensaver;
}

void KODIVideoChecker::setVideoMode(VideoMode newVideoMode)
{
	if (newVideoMode == _previousVideoMode)
	{
		// no change
		return;
	}

	switch (newVideoMode)
	{
	case VIDEO_2D:
		Info(_log, "KODICHECK INFO: switching to 2D mode");
		break;
	case VIDEO_3DSBS:
		Info(_log, "KODICHECK INFO: switching to 3D SBS mode");
		break;
	case VIDEO_3DTAB:
		Info(_log, "KODICHECK INFO: switching to 3D TAB mode");
		break;
	}

	emit videoMode(newVideoMode);
	_previousVideoMode = newVideoMode;
}
