
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceRawHID.h"

// Use feature report HID device
LedDeviceRawHID::LedDeviceRawHID(const Json::Value &deviceConfig)
	: ProviderHID(deviceConfig)
	, _timer()
{
	_useFeature = true;

	// setup the timer
	_timer.setSingleShot(false);
	_timer.setInterval(5000);
	connect(&_timer, SIGNAL(timeout()), this, SLOT(rewriteLeds()));

	// start the timer
	_timer.start();
}

LedDevice* LedDeviceRawHID::construct(const Json::Value &deviceConfig)
{
	return new LedDeviceRawHID(deviceConfig);
}

int LedDeviceRawHID::write(const std::vector<ColorRgb> & ledValues)
{
	// Resize buffer if required
	if (_ledBuffer.size() < ledValues.size() * 3) {
		_ledBuffer.resize(3 * ledValues.size());
	}

	// restart the timer
	_timer.start();

	// write data
	memcpy(_ledBuffer.data(), ledValues.data(), ledValues.size() * 3);
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

int LedDeviceRawHID::switchOff()
{
	// restart the timer
	_timer.start();

	// write data
	std::fill(_ledBuffer.begin(), _ledBuffer.end(), uint8_t(0));
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

void LedDeviceRawHID::rewriteLeds()
{
	writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
