
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
#include "LedUdpDevice.h"

LedUdpDevice::LedUdpDevice(const std::string& output, const int latchTime_ns)
	: _target(output)
	, _LatchTime_ns(latchTime_ns)
{
	_udpSocket = new QUdpSocket();
	QString str = QString::fromStdString(_target);
	QStringList str_splitted = str.split(":");
	if (str_splitted.size() != 2)
	{
		throw("Error parsing hostname:port");
	}
	QHostInfo info = QHostInfo::fromName(str_splitted.at(0));
	if (!info.addresses().isEmpty())
	{
		// use the first IP address
		_address = info.addresses().first();
	}
	_port = str_splitted.at(1).toInt();
}

LedUdpDevice::~LedUdpDevice()
{
	_udpSocket->close();
}

int LedUdpDevice::open()
{
	QHostAddress localAddress = QHostAddress::Any;
	quint16      localPort = 0;

	WarningIf( !_udpSocket->bind(localAddress, localPort), _log, "Couldnt bind local address: %s", strerror(errno));

	return 0;
}

int LedUdpDevice::writeBytes(const unsigned size, const uint8_t * data)
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
