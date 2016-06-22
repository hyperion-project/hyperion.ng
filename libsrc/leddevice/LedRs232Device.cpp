
// STL includes
#include <cstring>
#include <iostream>

// Qt includes
#include <QTimer>

// Local Hyperion includes
#include "LedRs232Device.h"

LedRs232Device::LedRs232Device(const std::string& outputDevice, const unsigned baudrate, int delayAfterConnect_ms) :
	_deviceName(outputDevice),
	_baudRate_Hz(baudrate),
	_delayAfterConnect_ms(delayAfterConnect_ms),
	_rs232Port(this),
	_blockedForDelay(false),
	_log(Logger::getInstance("LedDevice"))
{
}

LedRs232Device::~LedRs232Device()
{
	if (_rs232Port.isOpen())
	{
		_rs232Port.close();
	}
}

int LedRs232Device::open()
{
	Info(_log, "Opening UART: %s", _deviceName.c_str());
	_rs232Port.setPortName(_deviceName.c_str());
	_rs232Port.setBaudRate(_baudRate_Hz);
	if ( ! _rs232Port.open( QIODevice::WriteOnly) )
	{
		Error(_log, "Unable to open RS232 device (%s)", _deviceName.c_str());
		return -1;
	}

	if (_delayAfterConnect_ms > 0)
	{
		_blockedForDelay = true;
		QTimer::singleShot(_delayAfterConnect_ms, this, SLOT(unblockAfterDelay()));
		Debug(_log, "Device blocked for %d ms", _delayAfterConnect_ms);
	}

	return 0;
}

int LedRs232Device::writeBytes(const unsigned size, const uint8_t * data)
{
	if (_blockedForDelay)
	{
		return 0;
	}

	if (!_rs232Port.isOpen())
	{
		// try to reopen
		if( ! _rs232Port.open(QIODevice::WriteOnly) ){
			// Try again in 3 seconds
			int seconds = 3000;
			_blockedForDelay = true;
			QTimer::singleShot(seconds, this, SLOT(unblockAfterDelay()));
			Debug(_log, "Device blocked for %d ms", seconds);
		}
		return status;
	}

	_rs232Port.flush();
	int result = _rs232Port.write((const char*)data, size);
	_rs232Port.flush();

	return (result<0) ? -1 : 0;
}

void LedRs232Device::unblockAfterDelay()
{
	Debug(_log, "Device unblocked");
	_blockedForDelay = false;
}
