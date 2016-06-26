// system includes
#include <stdexcept>

// project includes
#include <udplistener/UDPListener.h>

// hyperion util includes
#include "hyperion/ImageProcessorFactory.h"
#include "hyperion/ImageProcessor.h"
#include "utils/ColorRgb.h"
#include "HyperionConfig.h"

UDPListener::UDPListener(const int priority, const int timeout, const std::string& address, quint16 listenPort) :
	QObject(),
	_hyperion(Hyperion::getInstance()),
	_server(),
	_openConnections(),
	_priority(priority),
	_timeout(timeout),
	_ledColors(Hyperion::getInstance()->getLedCount(), ColorRgb::BLACK),
	_log(Logger::getInstance("UDPLISTENER")),
	_isActive(false)
{
	_server = new QUdpSocket(this);
	QHostAddress listenAddress = address.empty() 
	                           ? QHostAddress::Any 
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

	if (!_server->bind(listenAddress, listenPort))
	{
		Warning(_log, "Could not bind to %s:%d parsed from %s", listenAddress.toString().toStdString().c_str(), listenPort, address.c_str());
	}
	else
	{
		Info(_log, "Started, listening on %s:%d", listenAddress.toString().toStdString().c_str(), listenPort);
//		if (listenAddress.QHostAddress::isMulticast() ) {	// needs qt >= 5.6
		if (listenAddress.isInSubnet(QHostAddress::parseSubnet("224.0.0.0/4"))) {
			bool joinGroupOK = _server->joinMulticastGroup(listenAddress));
			InfoIf   (   joinGroupOK, _log, "Multicast enabled");
			WarningIf( ! joinGroupOK, _log, "Multicast failed");
		}
		_isActive = true;
	}
}

void UDPListener::stop()
{
	if ( ! active() )
		return;

	_server->close();
	_isActive = false;
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
