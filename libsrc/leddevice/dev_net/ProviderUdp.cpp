
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

ProviderUdp::ProviderUdp()
	: LedDevice()
	  , _udpSocket (nullptr)
	  , _port(1)
	  , _defaultHost("127.0.0.1")
{
	_deviceReady = false;
	_latchTime_ms = 1;
}

ProviderUdp::~ProviderUdp()
{
	_udpSocket->deleteLater();
}

bool ProviderUdp::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = LedDevice::init(deviceConfig);

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
	}

	return isInitOK;
}

bool ProviderUdp::initNetwork()
{
	bool isInitOK = false;

	_udpSocket =new QUdpSocket(this);

	if ( _udpSocket != nullptr)
	{
		QHostAddress localAddress = QHostAddress::Any;
		quint16      localPort = 0;
		if ( !_udpSocket->bind(localAddress, localPort) )
		{
			Warning ( _log, "Could not bind local address: %s", strerror(errno));
		}
		isInitOK = true;
	}

	return isInitOK;
}

int ProviderUdp::open()
{
	int retval = -1;
	QString errortext;
	_deviceReady = false;

	if ( init(_devConfig) )
	{
		if ( ! initNetwork())
		{
			this->setInError( "UDP Network error!" );
		}
		else
		{
			// Everything is OK -> enable device
			_deviceReady = true;
			setEnable(true);
			retval = 0;
		}
	}
	return retval;
}

void ProviderUdp::close()
{
	LedDevice::close();

	// LedDevice specific closing activites
	if ( _udpSocket != nullptr)
	{
		_udpSocket->close();
	}
}

int ProviderUdp::writeBytes(const unsigned size, const uint8_t * data)
{

	qint64 retVal = _udpSocket->writeDatagram((const char *)data,size,_address,_port);
	WarningIf((retVal<0), _log, "Error sending: %s", strerror(errno));

	return retVal;
}
