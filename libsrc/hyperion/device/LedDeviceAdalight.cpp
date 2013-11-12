
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceAdalight.h"

LedDeviceAdalight::LedDeviceAdalight(const std::string& outputDevice, const unsigned baudrate) :
	LedRs232Device(outputDevice, baudrate),
	_ledBuffer(0)
{
	// empty
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

	memcpy(6 + _ledBuffer.data(), ledValues.data(), ledValues.size() * 3);
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

int LedDeviceAdalight::switchOff()
{
	memset(6 + _ledBuffer.data(), 0, _ledBuffer.size()-6);
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
