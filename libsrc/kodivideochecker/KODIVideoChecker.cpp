// Qt includes
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <kodivideochecker/KODIVideoChecker.h>
#include <hyperion/Hyperion.h>

using namespace hyperion;

KODIVideoChecker* KODIVideoChecker::_kodichecker = nullptr;

KODIVideoChecker* KODIVideoChecker::initInstance(const QString & address, uint16_t port, bool grabVideo, bool grabPhoto, bool grabAudio, bool grabMenu, bool grabPause, bool grabScreensaver, bool enable3DDetection)
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

KODIVideoChecker::KODIVideoChecker(const QString & address, uint16_t port, bool grabVideo, bool grabPhoto, bool grabAudio, bool grabMenu, bool grabPause, bool grabScreensaver, bool enable3DDetection)
	: QObject()
	, _address(address)
	, _port(port)
	, _activePlayerRequest(R"({"jsonrpc":"2.0","method":"Player.GetActivePlayers", "id":666})")
	, _currentPlayingItemRequest(R"({"id":667,"jsonrpc":"2.0","method":"Player.GetItem","params":{"playerid":%1,"properties":["file"]}})")
	, _checkScreensaverRequest(R"({"id":668,"jsonrpc":"2.0","method":"XBMC.GetInfoBooleans","params":{"booleans":["System.ScreenSaverActive"]}})")
	, _getStereoscopicMode(R"({"jsonrpc":"2.0","method":"GUI.GetProperties","params":{"properties":["stereoscopicmode"]},"id":669})")
	, _getKodiVersion(R"({"jsonrpc":"2.0","method":"Application.GetProperties","params":{"properties":["version"]},"id":670})")
	, _getCurrentPlaybackState(R"({"id":671,"jsonrpc":"2.0","method":"Player.GetProperties","params":{"playerid":%1,"properties":["speed"]}})")
	, _socket()
	, _grabVideo(grabVideo)
	, _grabPhoto(grabPhoto)
	, _grabAudio(grabAudio)
	, _grabMenu(grabMenu)
	, _grabPause(grabPause)
	, _grabScreensaver(grabScreensaver)
	, _enable3DDetection(enable3DDetection)
	, _previousGrabbingMode(GRABBINGMODE_INVALID)
	, _previousVideoMode(VIDEO_2D)
	, _currentPlaybackState(false)
	, _currentPlayerID(0)
	, _kodiVersion(0)
	, _log(Logger::getInstance("KODI"))
	, _active(false)
	, _getCurrentPlaybackStateInitialized(false)
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


void KODIVideoChecker::setConfig(const QString & address, uint16_t port, bool grabVideo, bool grabPhoto, bool grabAudio, bool grabMenu, bool grabPause, bool grabScreensaver, bool enable3DDetection)
{
	_address                 = address;
	_port                    = port;
	_grabVideo               = grabVideo;
	_grabPhoto               = grabPhoto;
	_grabAudio               = grabAudio;
	_grabMenu                = grabMenu;
	_grabPause               = grabPause;
	_grabScreensaver         = grabScreensaver;
	_enable3DDetection       = enable3DDetection;
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

void KODIVideoChecker::componentStateChanged(const hyperion::Components component, bool enable)
{
	if (component == COMP_KODICHECKER)
	{
		if (_active != enable)
		{
			if (enable) start();
			else        stop();
			Info(_log, "change state to %s", (_active ? "enabled" : "disabled") );
		}
		Hyperion::getInstance()->getComponentRegister().componentStateChanged(component, _active);
	}
}


void KODIVideoChecker::receiveReply()
{
	QJsonParseError error;
	QByteArray qtreply = _socket.readAll();
	QJsonDocument doc = QJsonDocument::fromJson(qtreply, &error);
	
	if (error.error == QJsonParseError::NoError)
	{
		if (doc.isObject())
		{
			Debug(_log, "message: %s", doc.toJson(QJsonDocument::Compact).constData());

			// Reply
			if (doc.object().contains("id"))
			{

				int id = doc.object()["id"].toInt();
				switch (id)
				{
					case 666:
					{
						if (doc.object()["result"].isArray())
						{
							QJsonArray resultArray = doc.object()["result"].toArray();
							
							if (!resultArray.isEmpty())
							{
								// always start a new video in 2D mode
								emit videoMode(VIDEO_2D);

								QString type = resultArray[0].toObject()["type"].toString();

								int prevCurrentPlayerID = _currentPlayerID;
								_currentPlayerID = resultArray[0].toObject()["playerid"].toInt();

								// set initial player state
								if (! _getCurrentPlaybackStateInitialized && prevCurrentPlayerID == 0 && _currentPlayerID != 0)
								{
									_socket.write(_getCurrentPlaybackState.arg(_currentPlayerID).toUtf8());
									_getCurrentPlaybackStateInitialized = true;
									return;
								}

								if (type == "video")
								{
									if (_currentPlaybackState)
									{
										// video is playing
										setGrabbingMode(_grabVideo ? GRABBINGMODE_VIDEO : GRABBINGMODE_OFF);
									}
									else
									{
										setGrabbingMode(_grabPause ? GRABBINGMODE_PAUSE : GRABBINGMODE_OFF);
									}

									// request info of the playing item
									_socket.write(_currentPlayingItemRequest.arg(_currentPlayerID).toUtf8());
								}
								else if (type == "picture")
								{
									if (_currentPlaybackState)
									{
										// picture is playing
										setGrabbingMode(_grabPhoto ? GRABBINGMODE_PHOTO : GRABBINGMODE_OFF);
									}
									else
									{
										setGrabbingMode(_grabPause ? GRABBINGMODE_PAUSE : GRABBINGMODE_OFF);
									}
								}
								else if (type == "audio")
								{
									// audio is playing
									if (_currentPlaybackState)
									{
										// audio is playing
										setGrabbingMode(_grabAudio ? GRABBINGMODE_AUDIO : GRABBINGMODE_OFF);
									}
									else
									{
										setGrabbingMode(_grabPause ? GRABBINGMODE_PAUSE : GRABBINGMODE_OFF);
									}
								}
							}
							else
							{
								// Nothing is playing.
								setGrabbingMode(_grabMenu ? GRABBINGMODE_MENU : GRABBINGMODE_OFF);
							}
						}
						else
						{
							setGrabbingMode(_grabMenu ? GRABBINGMODE_MENU : GRABBINGMODE_OFF);
						}
						break;
					}
					case 667:
					{
						if (_kodiVersion >= 13)
						{
							// check of active stereoscopicmode
							_socket.write(_getStereoscopicMode.toUtf8());
						}
						else
						{
							if (doc.object()["result"].toObject()["item"].toObject().contains("file"))
							{
								QString filename = doc.object()["result"].toObject()["item"].toObject()["file"].toString();
								if (filename.contains("3DSBS", Qt::CaseInsensitive) || filename.contains("HSBS", Qt::CaseInsensitive))
									setVideoMode(VIDEO_3DSBS);
								else if (filename.contains("3DTAB", Qt::CaseInsensitive) || filename.contains("HTAB", Qt::CaseInsensitive))
									setVideoMode(VIDEO_3DTAB);
								else
									setVideoMode(VIDEO_2D);
							}
							else
							{
								setGrabbingMode(_grabMenu ? GRABBINGMODE_MENU : GRABBINGMODE_OFF);
							}
						}
						break;
					}
					case 668:
					{
						if (doc.object()["result"].toObject().contains("System.ScreenSaverActive"))
						{
							// result of System.ScreenSaverActive
							if (doc.object()["result"].toObject()["System.ScreenSaverActive"].toBool())
								setGrabbingMode(_grabScreensaver ? GRABBINGMODE_SCREENSAVER : GRABBINGMODE_OFF);
							else
								_socket.write(_activePlayerRequest.toUtf8());

							// check here kodi version
							if (_socket.state() == QTcpSocket::ConnectedState)
							{
								if (_kodiVersion == 0)
								{
									_socket.write(_getKodiVersion.toUtf8());
								}
							}
						}
						else
						{
							setGrabbingMode(_grabMenu ? GRABBINGMODE_MENU : GRABBINGMODE_OFF);
						}
						break;
					}
					case 669:
					{
						if (doc.object()["result"].toObject()["stereoscopicmode"].toObject().contains("mode"))
						{
							// result of stereoscopicmode
							QString mode = doc.object()["result"].toObject()["stereoscopicmode"].toObject()["mode"].toString();
							if (mode == "split_vertical")
								setVideoMode(VIDEO_3DSBS);
							else if (mode == "split_horizontal")
								setVideoMode(VIDEO_3DTAB);
						}
						else
						{
							setGrabbingMode(_grabMenu ? GRABBINGMODE_MENU : GRABBINGMODE_OFF);
						}
						break;
					}
					case 670:
					{
						if (doc.object()["result"].toObject()["version"].toObject().contains("major"))
						{
							// kodi major version
							_kodiVersion = doc.object()["result"].toObject()["version"].toObject()["major"].toInt();
						}
						else
						{
							setGrabbingMode(_grabMenu ? GRABBINGMODE_MENU : GRABBINGMODE_OFF);
						}
						break;
					}
					case 671:
					{
						if (doc.object()["result"].toObject().contains("speed"))
						{
							// result of Player.PlayPause 
							_currentPlaybackState = static_cast<bool>(doc.object()["result"].toObject()["speed"].toInt());
						}
						else
						{
							setGrabbingMode(_grabMenu ? GRABBINGMODE_MENU : GRABBINGMODE_OFF);
						}
						break;
					}
				}
			}
			// Notification
			else if (doc.object().contains("method"))
			{
				QString method = doc.object()["method"].toString();
				if (method == "Player.OnPlay")
				{
					if (doc.object()["params"].toObject()["data"].toObject()["player"].toObject().contains("speed"))
					{
						_getCurrentPlaybackStateInitialized = true;
						_currentPlaybackState = static_cast<bool>(doc.object()["params"].toObject()["data"].toObject()["player"].toObject()["speed"].toInt());
					}
					// send a request for the current player state
					_socket.write(_activePlayerRequest.toUtf8());
					return;
				}
				else if (method == "Player.OnStop")
				{
					// the player has stopped
					setGrabbingMode(_grabMenu ? GRABBINGMODE_MENU : GRABBINGMODE_OFF);
					setVideoMode(VIDEO_2D);
				}
				else if (method == "Player.OnPause")
				{
					if (doc.object()["params"].toObject()["data"].toObject()["player"].toObject().contains("speed"))
					{
						_getCurrentPlaybackStateInitialized = true;
						_currentPlaybackState = static_cast<bool>(doc.object()["params"].toObject()["data"].toObject()["player"].toObject()["speed"].toInt());
					}
					// player at pause
					setGrabbingMode(_grabPause ? GRABBINGMODE_PAUSE : GRABBINGMODE_OFF);
				}
				else if (method == "GUI.OnScreensaverActivated")
					setGrabbingMode(_grabScreensaver ? GRABBINGMODE_SCREENSAVER : GRABBINGMODE_OFF);
				else if (method == "GUI.OnScreensaverDeactivated")
					_socket.write(_activePlayerRequest.toUtf8());
				else if (method == "Playlist.OnAdd" &&
					(doc.object()["params"]
					.toObject()["data"]
					.toObject()["item"]
					.toObject()["type"]) == QString("picture"))
				{
					// picture viewer is playing
					setGrabbingMode(_grabPhoto ? GRABBINGMODE_PHOTO : GRABBINGMODE_OFF);
				}
				else if (method == "Input.OnInputFinished")
				{
					// This Event is fired when Kodi Login
					_socket.write(_activePlayerRequest.toUtf8());
					_socket.write(_checkScreensaverRequest.toUtf8());
					if (_currentPlayerID != 0)
						_socket.write(_getCurrentPlaybackState.arg(_currentPlayerID).toUtf8());
				}
			}
		}
		else
		{
			Debug(_log, "Incomplete data");
		}
	}
}

void KODIVideoChecker::connected()
{
	Info(_log, "Connected");
	_getCurrentPlaybackStateInitialized = false;
	// send a request for the current player state
	_socket.write(_activePlayerRequest.toUtf8());
	_socket.write(_checkScreensaverRequest.toUtf8());
}

void KODIVideoChecker::disconnected()
{
	Info(_log, "Disconnected");
	_previousGrabbingMode    = GRABBINGMODE_INVALID;
	_previousVideoMode       = VIDEO_2D;
	_kodiVersion             = 0;
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
	Error(_log,"Connection error (%s)", _socket.errorString().toStdString().c_str());

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
	case GRABBINGMODE_SCREENSAVER:
		Info(_log, "switching to SCREENSAVER mode");
		break;
	default:
		Warning(_log, "switching to INVALID mode");
		break;
	}

	emit grabbingMode(newGrabbingMode);
	_previousGrabbingMode = newGrabbingMode;
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
		Info(_log, "switching to 2D mode");
		break;
	case VIDEO_3DSBS:
		Info(_log, "switching to 3D SBS mode");
		break;
	case VIDEO_3DTAB:
		Info(_log, "switching to 3D TAB mode");
		break;
	}

	emit videoMode(newVideoMode);
	_previousVideoMode = newVideoMode;
}
