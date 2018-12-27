// project includes
#include <udplistener/UDPListener.h>

// hyperion includes
#include <hyperion/Hyperion.h>
#include <bonjour/bonjourserviceregister.h>

// hyperion util includes
#include "utils/ColorRgb.h"
#include "HyperionConfig.h"

// qt includes
#include <QUdpSocket>

using namespace hyperion;

UDPListener::UDPListener(const QJsonDocument& config) :
	QObject(),
	_hyperion(Hyperion::getInstance()),
	_server(new QUdpSocket(this)),
	_openConnections(),
	_priority(0),
	_timeout(0),
	_log(Logger::getInstance("UDPLISTENER")),
	_isActive(false),
	_listenPort(0)
{
	Debug(_log, "Instance created");
	// listen for comp changes
	connect(_hyperion, SIGNAL(componentStateChanged(hyperion::Components,bool)), this, SLOT(componentStateChanged(hyperion::Components,bool)));
	// Set trigger for incoming connections
	connect(_server, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));

	// init
	handleSettingsUpdate(settings::UDPLISTENER, config);
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
		Error(_log, "Could not bind to %s:%d", _listenAddress.toString().toStdString().c_str(), _listenPort);
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
		_hyperion->getComponentRegister().componentStateChanged(COMP_UDPLISTENER, _isActive);

		if(_bonjourService == nullptr)
		{
			_bonjourService = new BonjourServiceRegister();
			_bonjourService->registerService("_hyperiond-udp._udp", _listenPort);
		}
	}
}

void UDPListener::stop()
{
	if ( ! active() )
		return;

	_server->close();
	_isActive = false;
	Info(_log, "Stopped");
	_hyperion->clear(_priority);
	_hyperion->getComponentRegister().componentStateChanged(COMP_UDPLISTENER, _isActive);
}

void UDPListener::componentStateChanged(const hyperion::Components component, bool enable)
{
	if (component == COMP_UDPLISTENER)
	{
		if (_isActive != enable)
		{
			if (enable) start();
			else        stop();
		}
	}
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

		_server->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
		processTheDatagram(&datagram, &sender);
	}
}


void UDPListener::processTheDatagram(const QByteArray * datagram, const QHostAddress * sender)
{
	int packetLedCount = datagram->size()/3;
	int hyperionLedCount = Hyperion::getInstance()->getLedCount();
	DebugIf( (packetLedCount != hyperionLedCount), _log, "packetLedCount (%d) != hyperionLedCount (%d)", packetLedCount, hyperionLedCount);

	std::vector<ColorRgb> _ledColors(Hyperion::getInstance()->getLedCount(), ColorRgb::BLACK);

	for (int ledIndex=0; ledIndex < qMin(packetLedCount, hyperionLedCount); ledIndex++) {
		ColorRgb & rgb =  _ledColors[ledIndex];
		rgb.red   = datagram->at(ledIndex*3+0);
		rgb.green = datagram->at(ledIndex*3+1);
		rgb.blue  = datagram->at(ledIndex*3+2);
	}
	// TODO provide a setInput with origin arg to overwrite senders smarter
	_hyperion->registerInput(_priority, hyperion::COMP_UDPLISTENER, QString("UDPListener@%1").arg(sender->toString()));
	_hyperion->setInput(_priority, _ledColors, _timeout);
}

void UDPListener::handleSettingsUpdate(const settings::type& type, const QJsonDocument& config)
{
	if(type == settings::UDPLISTENER)
	{
		QJsonObject obj = config.object();
		// if we change the prio we need to make sure the old one is cleared before we apply the new one!
		stop();

		QString addr = obj["address"].toString("");
		_priority = obj["priority"].toInt();
		_listenPort = obj["port"].toInt();
		_listenAddress = addr.isEmpty()? QHostAddress::AnyIPv4 : QHostAddress(addr);
		_bondage = (obj["shared"].toBool(false)) ? QAbstractSocket::ShareAddress : QAbstractSocket::DefaultForPlatform;
		_timeout = obj["timeout"].toInt(10000);
		if(obj["enable"].toBool())
			start();
	}
}
