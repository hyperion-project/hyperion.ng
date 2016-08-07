
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

LedUdpDevice::LedUdpDevice(const std::string& output, const int latchTime_ns) :
	_target(output),
	_LatchTime_ns(latchTime_ns)
{
	udpSocket = new QUdpSocket();
	QString str = QString::fromStdString(_target);
	QStringList _list = str.split(":");
	if (_list.size() != 2)  {
		throw("Error parsing hostname:port");
	}
	QHostInfo info = QHostInfo::fromName(_list.at(0));
	if (!info.addresses().isEmpty()) {
	    _address = info.addresses().first();
	    // use the first IP address
	}
	_port = _list.at(1).toInt();
}

LedUdpDevice::~LedUdpDevice()
{
	udpSocket->close();
}

int LedUdpDevice::open()
{
	QHostAddress _localAddress = QHostAddress::Any;
	quint16 _localPort = 0;

	WarningIf( !udpSocket->bind(_localAddress, _localPort), _log, "Couldnt bind local address: %s", strerror(errno));

	return 0;
}

int LedUdpDevice::writeBytes(const unsigned size, const uint8_t * data)
{

	qint64 retVal = udpSocket->writeDatagram((const char *)data,size,_address,_port);

	if (retVal >= 0 && _LatchTime_ns > 0)
	{
		// The 'latch' time for latching the shifted-value into the leds
		timespec latchTime;
		latchTime.tv_sec  = 0;
		latchTime.tv_nsec = _LatchTime_ns;

		// Sleep to latch the leds (only if write succesfull)
		nanosleep(&latchTime, NULL);
	} else {
		Warning( _log, "Error sending: %s", strerror(errno));
	}

	return retVal;
}
