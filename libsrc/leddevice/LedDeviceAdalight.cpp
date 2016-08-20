
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceAdalight.h"
#include <leddevice/LedDevice.h>

LedDeviceAdalight::LedDeviceAdalight(const Json::Value &deviceConfig)
	: LedRs232Device(deviceConfig)
	, _timer()
{
	// setup the timer
	_timer.setSingleShot(false);
	_timer.setInterval(5000);
	connect(&_timer, SIGNAL(timeout()), this, SLOT(rewriteLeds()));

	// start the timer
	_timer.start();
}

LedDevice* LedDeviceAdalight::createLedDevice(const Json::Value &deviceConfig)
{
	return new LedDeviceAdalight(deviceConfig);
}

int LedDeviceAdalight::write(const std::vector<ColorRgb> & ledValues)
{
	if (_ledBuffer.size() == 0)
	{
		_ledBuffer.resize(6 + 3*ledValues.size());
		_ledBuffer[0] = 'A';
		_ledBuffer[1] = 'd';
		_ledBuffer[2] = 'a';
		_ledBuffer[3] = ((ledValues.size() - 1) >> 8) & 0xFF; // LED count high byte
		_ledBuffer[4] = (ledValues.size() - 1) & 0xFF;        // LED count low byte
		_ledBuffer[5] = _ledBuffer[3] ^ _ledBuffer[4] ^ 0x55; // Checksum
	}

	// restart the timer
	_timer.start();

	// write data
	memcpy(6 + _ledBuffer.data(), ledValues.data(), ledValues.size() * 3);
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

int LedDeviceAdalight::switchOff()
{
	// restart the timer
	_timer.start();

	// write data
	memset(6 + _ledBuffer.data(), 0, _ledBuffer.size()-6);
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

void LedDeviceAdalight::rewriteLeds()
{
	writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
