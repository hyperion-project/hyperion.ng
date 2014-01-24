
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceP9813.h"

LedDeviceP9813::LedDeviceP9813(const std::string& outputDevice, const unsigned baudrate) :
	LedSpiDevice(outputDevice, baudrate, 0),
	_ledCount(0)
{
	// empty
}

int LedDeviceP9813::write(const std::vector<ColorRgb> &ledValues)
{
	if (_ledCount != ledValues.size())
	{
		_ledBuf.resize(ledValues.size() * 4 + 8, 0x00);
		_ledCount = ledValues.size();
	}

	uint8_t * dataPtr = _ledBuf.data();
	for (const ColorRgb & color : ledValues)
	{
		*dataPtr++ = calculateChecksum(color);
		*dataPtr++ = color.blue;
		*dataPtr++ = color.green;
		*dataPtr++ = color.red;
	}

	return writeBytes(_ledBuf.size(), _ledBuf.data());
}

int LedDeviceP9813::switchOff()
{
	return write(std::vector<ColorRgb>(_ledCount, ColorRgb{0,0,0}));
}

uint8_t LedDeviceP9813::calculateChecksum(const ColorRgb & color) const
{
	uint8_t res = 0;

	res |= (uint8_t)0x03 << 6;
	res |= (uint8_t)(~(color.blue  >> 6) & 0x03) << 4;
	res |= (uint8_t)(~(color.green >> 6) & 0x03) << 2;
	res |= (uint8_t)(~(color.red   >> 6) & 0x03);

	return res;
}
