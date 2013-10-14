// Qt includes
#include <QUrl>

#include <xbmcvideochecker/XBMCVideoChecker.h>

XBMCVideoChecker::XBMCVideoChecker(const std::string & address, uint16_t port, uint64_t interval_ms, bool grabVideo, bool grabPhoto, bool grabAudio, bool grabMenu) :
	QObject(),
	_address(QString::fromStdString(address)),
	_port(port),
	_request(R"({"jsonrpc":"2.0","method":"Player.GetActivePlayers","id":666})"),
	_timer(),
	_socket(),
	_grabVideo(grabVideo),
	_grabPhoto(grabPhoto),
	_grabAudio(grabAudio),
	_grabMenu(grabMenu),
	_previousMode(GRABBINGMODE_INVALID)
{
	// setup timer
	_timer.setSingleShot(false);
	_timer.setInterval(interval_ms);
	connect(&_timer, SIGNAL(timeout()), this, SLOT(sendRequest()));

	// setup socket
	connect(&_socket, SIGNAL(readyRead()), this, SLOT(receiveReply()));
}

void XBMCVideoChecker::start()
{
	_timer.start();
}

void XBMCVideoChecker::sendRequest()
{
	switch (_socket.state())
	{
	case QTcpSocket::UnconnectedState:
		// not connected. try to connect
		std::cout << "Connecting to " << _address.toStdString() << ":" << _port << " to check XBMC player status" << std::endl;
		_socket.connectToHost(_address, _port);
		break;
	case QTcpSocket::ConnectedState:
		// write the request on the socket
		_socket.write(_request);
		break;
	default:
		// whatever. let's check again at the next timer tick
		break;
	}
}

void XBMCVideoChecker::receiveReply()
{
	// expect that the reply is received as a single message. Probably oke considering the size of the expected reply
	QString reply(_socket.readAll());

	// check if the resply is a reply to one of my requests
	if (!reply.contains("\"id\":666"))
	{
		// probably not. Leave this mreply as is and don't act on it
		return;
	}

	GrabbingMode newMode = GRABBINGMODE_INVALID;
	if (reply.contains("video"))
	{
		// video is playing
		newMode = _grabVideo ? GRABBINGMODE_VIDEO : GRABBINGMODE_OFF;
	}
	else if (reply.contains("picture"))
	{
		// picture viewer is playing
		newMode = _grabPhoto ? GRABBINGMODE_PHOTO : GRABBINGMODE_OFF;
	}
	else if (reply.contains("audio"))
	{
		// audio is playing
		newMode = _grabAudio ? GRABBINGMODE_AUDIO : GRABBINGMODE_OFF;
	}
	else
	{
		// Nothing is playing.
		newMode = _grabMenu ? GRABBINGMODE_MENU : GRABBINGMODE_OFF;
	}

	// emit new state if applicable
	if (newMode != _previousMode)
	{
		emit grabbingMode(newMode);
		_previousMode = newMode;
	}
}

