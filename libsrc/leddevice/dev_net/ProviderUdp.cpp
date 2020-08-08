
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

// Local Hyperion includes
#include "ProviderUdp.h"

const ushort MAX_PORT = 65535;

ProviderUdp::ProviderUdp()
	: LedDevice()
	  , _udpSocket (nullptr)
	  , _port(1)
	  , _defaultHost("127.0.0.1")
{
	_isDeviceReady = false;
	_latchTime_ms = 1;
}

ProviderUdp::~ProviderUdp()
{
	if ( _udpSocket != nullptr )
	{
		delete _udpSocket;
	}
}

bool ProviderUdp::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = false;

	// Initialise sub-class
	if ( LedDevice::init(deviceConfig) )
	{
		QString host = deviceConfig["host"].toString(_defaultHost);

		if (_address.setAddress(host) )
		{
			Debug( _log, "Successfully parsed %s as an ip address.", deviceConfig["host"].toString().toStdString().c_str());
		}
		else
		{
			Debug( _log, "Failed to parse [%s] as an ip address.", deviceConfig["host"].toString().toStdString().c_str());
			QHostInfo info = QHostInfo::fromName(host);
			if (info.addresses().isEmpty())
			{
				Debug( _log, "Failed to parse [%s] as a hostname.", deviceConfig["host"].toString().toStdString().c_str());
				QString errortext = QString ("Invalid target address [%1]!").arg(host);
				this->setInError ( errortext );
				return false;
			}
			else
			{
				Debug( _log, "Successfully parsed %s as a hostname.", deviceConfig["host"].toString().toStdString().c_str());
				_address = info.addresses().first();
			}
		}

		int config_port = deviceConfig["port"].toInt(_port);
		if ( config_port <= 0 || config_port > MAX_PORT )
		{
			QString errortext = QString ("Invalid target port [%1]!").arg(config_port);
			this->setInError ( errortext );
			isInitOK = false;
		}
		else
		{
			_port = static_cast<int>(config_port);
			Debug( _log, "UDP using %s:%d", _address.toString().toStdString().c_str() , _port );

			_udpSocket = new QUdpSocket(this);

			isInitOK = true;
		}
	}
	return isInitOK;
}

int ProviderUdp::open()
{
	int retval = -1;
	_isDeviceReady = false;

	// Try to bind the UDP-Socket
	if ( _udpSocket != nullptr )
	{
		QHostAddress localAddress = QHostAddress::Any;
		quint16      localPort = 0;
		if ( !_udpSocket->bind(localAddress, localPort) )
		{
			QString warntext = QString ("Could not bind local address: %1, (%2) %3").arg(localAddress.toString()).arg(_udpSocket->error()).arg(_udpSocket->errorString());
			Warning ( _log, "%s", QSTRING_CSTR(warntext));
		}
		// Everything is OK, device is ready
		_isDeviceReady = true;
		retval = 0;
	}
	else
	{
		this->setInError( " Open error. UDP Socket not initialised!" );
	}
	return retval;
}

int ProviderUdp::close()
{
	int retval = 0;
	_isDeviceReady = false;

	if ( _udpSocket != nullptr )
	{
		// Test, if device requires closing
		if ( _udpSocket->isOpen() )
		{
			Debug(_log,"Close UDP-device: %s", QSTRING_CSTR( this->getActiveDeviceType() ) );
			_udpSocket->close();
			// Everything is OK -> device is closed
		}
	}
	return retval;
}

int ProviderUdp::writeBytes(unsigned size, const uint8_t * data)
{
	qint64 retVal = _udpSocket->writeDatagram((const char *)data,size,_address,_port);

	WarningIf((retVal<0), _log, "&s", QSTRING_CSTR(QString
								("(%1:%2) Write Error: (%3) %4").arg(_address.toString()).arg(_port).arg(_udpSocket->error()).arg(_udpSocket->errorString())));

	return retVal;
}
