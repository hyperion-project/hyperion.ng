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

// mDNS discover
#ifdef ENABLE_MDNS
#include <mdns/MdnsBrowser.h>
#endif
#include <utils/NetUtils.h>

// Local Hyperion includes
#include "ProviderUdp.h"

ProviderUdp::ProviderUdp(const QJsonObject& deviceConfig)
	: LedDevice(deviceConfig)
	  , _udpSocket(nullptr)
	  , _port(-1)
{
	_latchTime_ms = 0;
}

ProviderUdp::~ProviderUdp()
{
	delete _udpSocket;
}

int ProviderUdp::open()
{
	int retval = -1;
	_isDeviceReady = false;

	if (!_isDeviceInError)
	{
			if (_udpSocket == nullptr)
			{
				_udpSocket = new QUdpSocket(this);
			}

			// Try to bind the UDP-Socket
			if (_udpSocket != nullptr)
			{
				Info(_log, "Stream UDP data to %s port: %d", QSTRING_CSTR(_address.toString()), _port);
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
				retval = 0;
			}
			else
			{
				this->setInError(" Open error. UDP Socket not initialised!");
			}
	}
	return retval;
}

int ProviderUdp::close()
{
	int retval = 0;
	_isDeviceReady = false;

	if (_udpSocket != nullptr)
	{
		// Test, if device requires closing
		if (_udpSocket->isOpen())
		{
			Debug(_log, "Close UDP-device: %s", QSTRING_CSTR(this->getActiveDeviceType()));
			_udpSocket->close();
			// Everything is OK -> device is closed
		}
	}
	return retval;
}

int ProviderUdp::writeBytes(const unsigned size, const uint8_t* data)
{
	int rc = 0;
	qint64 bytesWritten = _udpSocket->writeDatagram(reinterpret_cast<const char*>(data), size, _address, static_cast<quint16>(_port));

	if (bytesWritten == -1 || bytesWritten != size)
	{
		Warning(_log, "%s", QSTRING_CSTR(QString("(%1:%2) Write Error: (%3) %4").arg(_address.toString()).arg(_port).arg(_udpSocket->error()).arg(_udpSocket->errorString())));
		rc = -1;
	}
	return  rc;
}

int ProviderUdp::writeBytes(const QByteArray& bytes)
{
	int rc = 0;
	qint64 bytesWritten = _udpSocket->writeDatagram(bytes, _address, static_cast<quint16>(_port));

	if (bytesWritten == -1 || bytesWritten != bytes.size())
	{
		Warning(_log, "%s", QSTRING_CSTR(QString("(%1:%2) Write Error: (%3) %4").arg(_address.toString()).arg(_port).arg(_udpSocket->error()).arg(_udpSocket->errorString())));
		rc = -1;
	}
	return  rc;
}
