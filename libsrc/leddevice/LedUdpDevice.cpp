
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
	mLatchTime_ns(latchTime_ns),
	mFid(-1)
{
	udpSocket = new QUdpSocket();
	QString str = QString::fromStdString(mDeviceName);
	QStringList _list = str.split(":");
	if (_list.size() != 2)  {
		printf ("ERROR: LedUdpDevice: Error parsing hostname:port\n");
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
//	close(mFid);
}

int LedUdpDevice::open()
{
	udpSocket->bind(QHostAddress::Any, 7755);


/*
	if (mFid < 0)
	{
		std::cerr << "Failed to open device('" << mDeviceName << "') " << std::endl;
		return -1;
	}
*/

	return 0;
}

int LedUdpDevice::writeBytes(const unsigned size, const uint8_t * data)
{
/*
	if (mFid < 0)
	{
		return -1;
	}
*/

//	int retVal = udpSocket->writeDatagram((const char *)data,size,QHostAddress::LocalHost,9998);
	int retVal = udpSocket->writeDatagram((const char *)data,size,_address,_port);

	if (retVal == 0 && mLatchTime_ns > 0)
	{
		// The 'latch' time for latching the shifted-value into the leds
		timespec latchTime;
		latchTime.tv_sec  = 0;
		latchTime.tv_nsec = mLatchTime_ns;

		// Sleep to latch the leds (only if write succesfull)
		nanosleep(&latchTime, NULL);
	}

	return retVal;
}
