// Qt includes
#include <QUrl>

#include <xbmcvideochecker/XBMCVideoChecker.h>

XBMCVideoChecker::XBMCVideoChecker(QString address, uint16_t port, uint64_t interval_ms, Hyperion * hyperion, int priority) :
	QObject(),
	_address(address),
	_port(port),
	_request("{\"jsonrpc\":\"2.0\",\"method\":\"Player.GetActivePlayers\",\"id\":1}"),
	_timer(),
	_socket(),
	_hyperion(hyperion),
	_priority(priority)
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
	// expect that the reply is received as a single message. Probaly oke considering the size of the expected reply
	QString reply(_socket.readAll());

	if (reply.contains("playerid"))
	{
		// something is playing. check for "video" to check if a video is playing
		// clear our priority channel to allow the grabbed vido colors to be shown
		_hyperion->clear(_priority);
	}
	else
	{
		// Nothing is playing. set our priority channel completely to black
		// The timeout is used to have the channel cleared after 30 seconds of connection problems...
		_hyperion->setColor(_priority, RgbColor::BLACK, 30000);
	}
}

