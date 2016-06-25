
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

#include <QStringList>
#include <QUdpSocket>
#include <QHostInfo>

// Local Hyperion includes
#include "LedUdpDevice.h"

LedUdpDevice::LedUdpDevice(const std::string& outputDevice, const unsigned baudrate, const int latchTime_ns) :
	mDeviceName(outputDevice),
	mBaudRate_Hz(baudrate),
	mLatchTime_ns(latchTime_ns)
{
	udpSocket = new QUdpSocket();
	QString str = QString::fromStdString(mDeviceName);
	QStringList _list = str.split(":");
	if (_list.size() != 2)  {
		Error( Logger::getInstance("LedDevice"), "LedUdpDevice: Error parsing hostname:port");
		exit (-1);
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

	WarningIf( !udpSocket->bind(_localAddress, _localPort), 
		Logger::getInstance("LedDevice"), "LedUdpDevice: Couldnt bind local address: %s", strerror(errno));

	return 0;
}

int LedUdpDevice::writeBytes(const unsigned size, const uint8_t * data)
{

	qint64 retVal = udpSocket->writeDatagram((const char *)data,size,_address,_port);

	if (retVal >= 0 && mLatchTime_ns > 0)
	{
		// The 'latch' time for latching the shifted-value into the leds
		timespec latchTime;
		latchTime.tv_sec  = 0;
		latchTime.tv_nsec = mLatchTime_ns;

		// Sleep to latch the leds (only if write succesfull)
		nanosleep(&latchTime, NULL);
	} else {
		Warning( Logger::getInstance("LedDevice"), "LedUdpDevice: Error sending: %s", strerror(errno));
	}

	return retVal;
}
