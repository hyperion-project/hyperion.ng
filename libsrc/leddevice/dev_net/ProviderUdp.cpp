#include "ProviderUdp.h"

// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>
#include <exception>
// Linux includes
#include <fcntl.h>


#include <QStringList>
#include <QUdpSocket>
#include <QHostInfo>

#include <utils/NetUtils.h>

ProviderUdp::ProviderUdp(const QJsonObject& deviceConfig)
	: LedDevice(deviceConfig)
	  , _port(-1)
	  , _udpSocket(nullptr)
{
	_latchTime_ms = 0;
}

int ProviderUdp::open()
{
	_isDeviceReady = false;
	
	if (_isDeviceInError)
	{
		return -1;
	}

	if (_udpSocket.isNull())
	{
		_udpSocket.reset(new QUdpSocket());
	}

	// Try to bind the UDP-Socket
	if (_udpSocket.isNull())
	{
		this->setInError(" Open error. UDP Socket not initialised!");
		return -1;
	}

	if (!NetUtils::resolveHostToAddress(_log, _hostName, _ipAddress, _port))
	{
		this->setInError(" Open error. Unable to resolve IP-address for hostname: " + _hostName);
		return -1;
	}

	Info(_log, "Stream UDP data to [%s] port: [%d]", QSTRING_CSTR(_ipAddress.toString()), _port);
	if (_udpSocket->state() != QAbstractSocket::BoundState)
	{
		QHostAddress localAddress = QHostAddress::Any;
		quint16      localPort = 0;
		if (!_udpSocket->bind(localAddress, localPort))
		{
			QString warntext = QString("Could not bind local address: %1, (%2) %3").arg(localAddress.toString()).arg(_udpSocket->error()).arg(_udpSocket->errorString());
			Warning(_log, "%s", QSTRING_CSTR(warntext));
		}
	}

	return 0;
}

int ProviderUdp::close()
{
	_isDeviceReady = false;

	if (!_udpSocket.isNull())
	{
		// Test, if device requires closing
		if (_udpSocket->isOpen())
		{
			Debug(_log, "Close UDP-device: %s", QSTRING_CSTR(this->getActiveDeviceType()));
			_udpSocket->close();
			// Everything is OK -> device is closed
		}
	}
	return 0;
}
 
int ProviderUdp::writeBytes(const unsigned size, const uint8_t* data)
{
	int rc = 0;
	qint64 bytesWritten = _udpSocket->writeDatagram(reinterpret_cast<const char*>(data), size, _ipAddress, static_cast<quint16>(_port));

	if (bytesWritten == -1 || bytesWritten != size)
	{
		Warning(_log, "%s", QSTRING_CSTR(QString("(%1:%2) Write Error: (%3) %4").arg(_ipAddress.toString()).arg(_port).arg(_udpSocket->error()).arg(_udpSocket->errorString())));
		rc = -1;
	}
	return  rc;
}

int ProviderUdp::writeBytes(const QByteArray& bytes)
{
	int rc = 0;
	qint64 bytesWritten = _udpSocket->writeDatagram(bytes,_ipAddress, static_cast<quint16>(_port));

	if (bytesWritten == -1 || bytesWritten != bytes.size())
	{
		Warning(_log, "%s", QSTRING_CSTR(QString("(%1:%2) Write Error: (%3) %4").arg(_ipAddress.toString()).arg(_port).arg(_udpSocket->error()).arg(_udpSocket->errorString())));
		rc = -1;
	}
	return  rc;
}
