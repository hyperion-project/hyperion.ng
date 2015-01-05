// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// hyperion local includes
#include "LedDeviceAmbiLed.h"

LedDeviceAmbiLed::LedDeviceAmbiLed(const std::string& outputDevice, const unsigned baudrate, int delayAfterConnect_ms) :
	LedRs232Device(outputDevice, baudrate, delayAfterConnect_ms),
	_ledBuffer(0),
	_timer()
{
	// setup the timer
	_timer.setSingleShot(false);
	_timer.setInterval(5000);
	connect(&_timer, SIGNAL(timeout()), this, SLOT(rewriteLeds()));

	// start the timer
	_timer.start();
}

int LedDeviceAmbiLed::write(const std::vector<ColorRgb> & ledValues)
{
	if (_ledBuffer.size() == 0)
	{
		_ledBuffer.resize(1 + 3*ledValues.size());
		_ledBuffer[3*ledValues.size()] = 255;
	}

	// restart the timer
	_timer.start();

	// write data
	memcpy( _ledBuffer.data(), ledValues.data(), ledValues.size() * 3);
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

int LedDeviceAmbiLed::switchOff()
{
	// restart the timer
	_timer.start();

	// write data
	memset(_ledBuffer.data(), 0, _ledBuffer.size()-6);
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

void LedDeviceAmbiLed::rewriteLeds()
{
	writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
