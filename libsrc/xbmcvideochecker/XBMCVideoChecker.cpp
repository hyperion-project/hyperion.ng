// Qt includes
#include <QUrl>
#include <QRegExp>
#include <QStringRef>

#include <xbmcvideochecker/XBMCVideoChecker.h>

// Request player example:
// {"id":666,"jsonrpc":"2.0","method":"Player.GetActivePlayers"}
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

XBMCVideoChecker::XBMCVideoChecker(const std::string & address, uint16_t port, bool grabVideo, bool grabPhoto, bool grabAudio, bool grabMenu, bool grabScreensaver, bool enable3DDetection) :
	QObject(),
	_address(QString::fromStdString(address)),
	_port(port),
	_activePlayerRequest(R"({"id":666,"jsonrpc":"2.0","method":"Player.GetActivePlayers"})"),
	_currentPlayingItemRequest(R"({"id":667,"jsonrpc":"2.0","method":"Player.GetItem","params":{"playerid":%1,"properties":["file"]}})"),
	_checkScreensaverRequest(R"({"id":668,"jsonrpc":"2.0","method":"XBMC.GetInfoBooleans","params":{"booleans":["System.ScreenSaverActive"]}})"),
	_getStereoscopicMode(R"({"jsonrpc":"2.0","method":"GUI.GetProperties","params":{"properties":["stereoscopicmode"]},"id":669})"),
	_getXbmcVersion(R"({"jsonrpc":"2.0","method":"Application.GetProperties","params":{"properties":["version"]},"id":670})"),
	_socket(),
	_grabVideo(grabVideo),
	_grabPhoto(grabPhoto),
	_grabAudio(grabAudio),
	_grabMenu(grabMenu),
	_grabScreensaver(grabScreensaver),
	_enable3DDetection(enable3DDetection),
	_previousScreensaverMode(false),
	_previousGrabbingMode(GRABBINGMODE_INVALID),
	_previousVideoMode(VIDEO_2D),
	_xbmcVersion(0)
{
	// setup socket
	connect(&_socket, SIGNAL(readyRead()), this, SLOT(receiveReply()));
	connect(&_socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
	connect(&_socket, SIGNAL(connected()), this, SLOT(connected()));
	connect(&_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connectionError(QAbstractSocket::SocketError)));
}

void XBMCVideoChecker::start()
{
	reconnect();
}

void XBMCVideoChecker::receiveReply()
{
	// expect that the reply is received as a single message. Probably oke considering the size of the expected reply
	QString reply(_socket.readAll());

	std::cout << "Message from XBMC: " << reply.toStdString() << std::endl;

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
		if (_xbmcVersion >= 13)
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

		// check here xbmc version
		if (_socket.state() == QTcpSocket::ConnectedState)
		{
			if (_xbmcVersion == 0)
			{
				_socket.write(_getXbmcVersion.toUtf8());
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
			_xbmcVersion = regex.cap(1).toInt();
		}
	}
}

void XBMCVideoChecker::connected()
{
	std::cout << "XBMC Connected" << std::endl;

	// send a request for the current player state
	_socket.write(_activePlayerRequest.toUtf8());
	_socket.write(_checkScreensaverRequest.toUtf8());
}

void XBMCVideoChecker::disconnected()
{
	std::cout << "XBMC Disconnected" << std::endl;
	reconnect();
}

void XBMCVideoChecker::reconnect()
{
	if (_socket.state() == QTcpSocket::ConnectedState)
	{
		return;
	}

	// try to connect
	switch (_socket.state())
	{
	case QTcpSocket::ConnectingState:
		// Somehow when XBMC restarts we get stuck in connecting state
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

void XBMCVideoChecker::connectionError(QAbstractSocket::SocketError error)
{
	std::cout << "XBMC Connection error (" << error << ")" << std::endl;

	// close the socket
	_socket.close();
}

void XBMCVideoChecker::setGrabbingMode(GrabbingMode newGrabbingMode)
{
	if (newGrabbingMode == _previousGrabbingMode)
	{
		// no change
		return;
	}

	switch (newGrabbingMode)
	{
	case GRABBINGMODE_VIDEO:
		std::cout << "XBMC checker: switching to VIDEO mode" << std::endl;
		break;
	case GRABBINGMODE_PHOTO:
		std::cout << "XBMC checker: switching to PHOTO mode" << std::endl;
		break;
	case GRABBINGMODE_AUDIO:
		std::cout << "XBMC checker: switching to AUDIO mode" << std::endl;
		break;
	case GRABBINGMODE_MENU:
		std::cout << "XBMC checker: switching to MENU mode" << std::endl;
		break;
	case GRABBINGMODE_OFF:
		std::cout << "XBMC checker: switching to OFF mode" << std::endl;
		break;
	case GRABBINGMODE_INVALID:
		std::cout << "XBMC checker: switching to INVALID mode" << std::endl;
		break;
	}

	// only emit the new state when we want to grab in screensaver mode or when the screensaver is deactivated
	if (!_previousScreensaverMode)
	{
		emit grabbingMode(newGrabbingMode);
	}
	_previousGrabbingMode = newGrabbingMode;
}

void XBMCVideoChecker::setScreensaverMode(bool isOnScreensaver)
{
	if (isOnScreensaver == _previousScreensaverMode)
	{
		// no change
		return;
	}

	emit grabbingMode(isOnScreensaver ? GRABBINGMODE_OFF : _previousGrabbingMode);
	_previousScreensaverMode = isOnScreensaver;
}

void XBMCVideoChecker::setVideoMode(VideoMode newVideoMode)
{
	if (newVideoMode == _previousVideoMode)
	{
		// no change
		return;
	}

	switch (newVideoMode)
	{
	case VIDEO_2D:
		std::cout << "XBMC checker: switching to 2D mode" << std::endl;
		break;
	case VIDEO_3DSBS:
		std::cout << "XBMC checker: switching to 3D SBS mode" << std::endl;
		break;
	case VIDEO_3DTAB:
		std::cout << "XBMC checker: switching to 3D TAB mode" << std::endl;
		break;
	}

	emit videoMode(newVideoMode);
	_previousVideoMode = newVideoMode;
}
