// system includes
#include <stdexcept>

// project includes
#include <udplistener/UDPListener.h>

// hyperion util includes
#include "hyperion/ImageProcessorFactory.h"
#include "hyperion/ImageProcessor.h"
#include "utils/ColorRgb.h"
#include "HyperionConfig.h"

UDPListener::UDPListener(const int priority, const int timeout, const std::string& address, quint16 listenPort, bool shared) :
	QObject(),
	_hyperion(Hyperion::getInstance()),
	_server(),
	_openConnections(),
	_priority(priority),
	_timeout(timeout),
	_ledColors(Hyperion::getInstance()->getLedCount(), ColorRgb::BLACK),
	_log(Logger::getInstance("UDPLISTENER")),
	_isActive(false),
	_bondage(shared ? QAbstractSocket::ShareAddress : QAbstractSocket::DefaultForPlatform)
{
	_server = new QUdpSocket(this);
	QHostAddress listenAddress = address.empty() 
	                           ? QHostAddress::AnyIPv4 
	                           : QHostAddress( QString::fromStdString(address) );

	// Set trigger for incoming connections
	connect(_server, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
}

UDPListener::~UDPListener()
{
	// clear the current channel
	stop();
	delete _server;
	_hyperion->clear(_priority);
}


void UDPListener::start()
{
	if ( active() )
		return;

	QHostAddress mcastGroup;
	if (_listenAddress.isInSubnet(QHostAddress::parseSubnet("224.0.0.0/4"))) {
		mcastGroup = _listenAddress;
	}

	if (!_server->bind(_listenAddress, _listenPort, _bondage))
	{
		Warning(_log, "Could not bind to %s:%d", _listenAddress.toString().toStdString().c_str(), _listenPort);
	}
	else
	{
		Info(_log, "Started, listening on %s:%d", _listenAddress.toString().toStdString().c_str(), _listenPort);
		if (!mcastGroup.isNull()) {
			bool joinGroupOK = _server->joinMulticastGroup(_listenAddress);
			InfoIf   (   joinGroupOK, _log, "Multicast enabled");
			WarningIf( ! joinGroupOK, _log, "Multicast failed");
		}
		_isActive = true;
		emit statusChanged(_isActive);
	}
}

void UDPListener::stop()
{
	if ( ! active() )
		return;

	_server->close();
	_isActive = false;
	emit statusChanged(_isActive);
}


uint16_t UDPListener::getPort() const
{
	return _server->localPort();
}


void UDPListener::readPendingDatagrams()
{
	while (_server->hasPendingDatagrams()) {
		QByteArray datagram;
		datagram.resize(_server->pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;

		_server->readDatagram(datagram.data(), datagram.size(),
					&sender, &senderPort);

		processTheDatagram(&datagram);

	}
}


void UDPListener::processTheDatagram(const QByteArray * datagram)
{
	int packlen = datagram->size()/3;
	int ledlen = _ledColors.size();
	int maxled = std::min(packlen , ledlen);

	for (int ledIndex=0; ledIndex < maxled; ledIndex++) {
		ColorRgb & rgb =  _ledColors[ledIndex];
		rgb.red = datagram->at(ledIndex*3+0);
		rgb.green = datagram->at(ledIndex*3+1);
		rgb.blue = datagram->at(ledIndex*3+2);
	}

	_hyperion->setColors(_priority, _ledColors, _timeout, -1);

}
