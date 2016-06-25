
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
	_blockedForDelay(false)
{
}

LedRs232Device::~LedRs232Device()
{
	if (_rs232Port.isOpen())
		_rs232Port.close();
}


int LedRs232Device::open()
{
	Info(_log, "Opening UART: %s", _deviceName.c_str());
	_rs232Port.setPortName(_deviceName.c_str());

	return tryOpen() ? 0 : -1;
}


bool LedRs232Device::tryOpen()
{
	if ( ! _rs232Port.isOpen() )
	{
		if ( ! _rs232Port.open(QIODevice::WriteOnly) )
		{
			Error(_log, "Unable to open RS232 device (%s)", _deviceName.c_str());
			return false;
		}
		_rs232Port.setBaudRate(_baudRate_Hz);
	}
	
	if (_delayAfterConnect_ms > 0)
	{
		_blockedForDelay = true;
		QTimer::singleShot(_delayAfterConnect_ms, this, SLOT(unblockAfterDelay()));
		Debug(_log, "Device blocked for %d ms", _delayAfterConnect_ms);
	}

	return _rs232Port.isOpen();
}


int LedRs232Device::writeBytes(const unsigned size, const uint8_t * data)
{
	if (_blockedForDelay)
		return 0;

	if (!_rs232Port.isOpen())
	{
		_delayAfterConnect_ms = 3000;
		return tryOpen() ? 0 : -1;
	}

	_rs232Port.flush();
	int result = _rs232Port.write(reinterpret_cast<const char*>(data), size);
	_rs232Port.waitForBytesWritten(100);
	Debug(_log, "write %d ", result);
	_rs232Port.flush();

	return (result<0) ? -1 : 0;
}


void LedRs232Device::unblockAfterDelay()
{
	Debug(_log, "Device unblocked");
	_blockedForDelay = false;
}
