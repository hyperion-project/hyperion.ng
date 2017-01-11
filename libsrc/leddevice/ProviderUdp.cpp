
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>
#include <exception>
// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

#include <QStringList>
#include <QUdpSocket>
#include <QHostInfo>

// Local Hyperion includes
#include "ProviderUdp.h"

ProviderUdp::ProviderUdp()
	: LedDevice()
	, _LatchTime_ns(-1)
	, _port(1)
{
	_udpSocket = new QUdpSocket();
}

ProviderUdp::~ProviderUdp()
{
	_udpSocket->close();
}

bool ProviderUdp::init(const QJsonObject &deviceConfig, std::string defaultHost)
{
	LedDevice::init(deviceConfig);

	QString host = deviceConfig["host"].toString(QString::fromStdString(defaultHost));
	
	if (_address.setAddress(host) )
	{
		Debug( _log, "Successfully parsed %s as an ip address.", deviceConfig["host"].toString().toStdString().c_str());
	}
	else
	{
		Debug( _log, "Failed to parse %s as an ip address.", deviceConfig["host"].toString().toStdString().c_str());
		QHostInfo info = QHostInfo::fromName(host);
		if (info.addresses().isEmpty())
		{
			Debug( _log, "Failed to parse %s as a hostname.", deviceConfig["host"].toString().toStdString().c_str());
			throw std::runtime_error("invalid target address");
		}
		Debug( _log, "Successfully parsed %s as a hostname.", deviceConfig["host"].toString().toStdString().c_str());
		_address = info.addresses().first();
	}

	_port = deviceConfig["port"].toInt(_port);
	if ( _port<=0 || _port > 65535)
	{
		throw std::runtime_error("invalid target port");
	}
	
	Debug( _log, "UDP using %s:%d", _address.toString().toStdString().c_str() , _port );
	
	_LatchTime_ns = deviceConfig["latchtime"].toInt(_LatchTime_ns);

	return true;
}

int ProviderUdp::open()
{
	QHostAddress localAddress = QHostAddress::Any;
	quint16      localPort = 0;

	WarningIf( !_udpSocket->bind(localAddress, localPort), _log, "Could not bind local address: %s", strerror(errno));

	return 0;
}

int ProviderUdp::writeBytes(const unsigned size, const uint8_t * data)
{

	qint64 retVal = _udpSocket->writeDatagram((const char *)data,size,_address,_port);

	if (retVal >= 0 && _LatchTime_ns > 0)
	{
		// The 'latch' time for latching the shifted-value into the leds
		timespec latchTime;
		latchTime.tv_sec  = 0;
		latchTime.tv_nsec = _LatchTime_ns;

		// Sleep to latch the leds (only if write succesfull)
		nanosleep(&latchTime, NULL);
	}
	else
	{
		Warning( _log, "Error sending: %s", strerror(errno));
	}

	return retVal;
}
