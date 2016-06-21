// system includes
#include <stdexcept>

// project includes
#include <udplistener/UDPListener.h>

// hyperion util includes
#include "hyperion/ImageProcessorFactory.h"
#include "hyperion/ImageProcessor.h"
#include "utils/ColorRgb.h"
#include "HyperionConfig.h"

UDPListener::UDPListener(const int priority, const int timeout, uint16_t port) :
	QObject(),
	_hyperion(Hyperion::getInstance()),
	_server(),
	_openConnections(),
	_priority(priority),
	_timeout(timeout),
	_ledColors(Hyperion::getInstance()->getLedCount(), ColorRgb::BLACK)
{
	_server = new QUdpSocket(this);
	if (!_server->bind(QHostAddress::Any, port))
	{
		throw std::runtime_error("UDPLISTENER ERROR: server could not bind to port");
	}

	// Set trigger for incoming connections
	connect(_server, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
}

UDPListener::~UDPListener()
{
	// clear the current channel
	_hyperion->clear(_priority);
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

//		std::cout << "UDPLISTENER INFO: new packet from " << std::endl;
		processTheDatagram(&datagram);

	}
}


void UDPListener::processTheDatagram(const QByteArray * datagram)
{
//        std::cout << "udp message: " << datagram->data() << std::endl;

	int packlen = datagram->size()/3;
	int ledlen = _ledColors.size();
//	int maxled = std::min(datagram->size()/3, _ledColors.size());
	int maxled = std::min(packlen , ledlen);

	for (int ledIndex=0; ledIndex < maxled; ledIndex++) {
		ColorRgb & rgb =  _ledColors[ledIndex];
		rgb.red = datagram->at(ledIndex*3+0);
		rgb.green = datagram->at(ledIndex*3+1);
		rgb.blue = datagram->at(ledIndex*3+2);
//		printf("%02x%02x%02x%02x ", ledIndex, rgb.red, rgb.green, rgb.blue);
	}
//	printf ("\n");

	_hyperion->setColors(_priority, _ledColors, _timeout, -1);

}
