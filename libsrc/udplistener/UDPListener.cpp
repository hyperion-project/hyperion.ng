// project includes
#include <udplistener/UDPListener.h>

// bonjour includes
#include <bonjour/bonjourserviceregister.h>

// hyperion includes
#include "HyperionConfig.h"

// qt includes
#include <QUdpSocket>
#include <QJsonObject>

using namespace hyperion;

UDPListener::UDPListener(const QJsonDocument& config) :
	QObject(),
	_server(new QUdpSocket(this)),
	_priority(0),
	_timeout(0),
	_log(Logger::getInstance("UDPLISTENER")),
	_isActive(false),
	_listenPort(0)
{
	// init
	handleSettingsUpdate(settings::UDPLISTENER, config);
}

UDPListener::~UDPListener()
{
	// clear the current channel
	stop();
	delete _server;
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

		if(_serviceRegister == nullptr)
		{
			_serviceRegister = new BonjourServiceRegister(this);
			_serviceRegister->registerService("_hyperiond-udp._udp", _listenPort);
		}
		else if( _serviceRegister->getPort() != _listenPort)
		{
			delete _serviceRegister;
			_serviceRegister = new BonjourServiceRegister(this);
			_serviceRegister->registerService("_hyperiond-udp._udp", _listenPort);
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
	emit clearGlobalPriority(_priority, hyperion::COMP_UDPLISTENER);
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
	//DebugIf( (packetLedCount != hyperionLedCount), _log, "packetLedCount (%d) != hyperionLedCount (%d)", packetLedCount, hyperionLedCount);

	std::vector<ColorRgb> _ledColors(packetLedCount, ColorRgb::BLACK);

	for (int ledIndex=0; ledIndex < packetLedCount; ledIndex++) {
		ColorRgb & rgb =  _ledColors[ledIndex];
		rgb.red   = datagram->at(ledIndex*3+0);
		rgb.green = datagram->at(ledIndex*3+1);
		rgb.blue  = datagram->at(ledIndex*3+2);
	}
	// TODO provide a setInput with origin arg to overwrite senders smarter
	emit registerGlobalInput(_priority, hyperion::COMP_UDPLISTENER, QString("UDPListener@%1").arg(sender->toString()));
	emit setGlobalInput(_priority, _ledColors, _timeout);
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
