
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceP9813.h"

LedDeviceP9813::LedDeviceP9813(const Json::Value &deviceConfig)
	: ProviderSpi(deviceConfig)
{
}

LedDevice* LedDeviceP9813::construct(const Json::Value &deviceConfig)
{
	return new LedDeviceP9813(deviceConfig);
}

int LedDeviceP9813::write(const std::vector<ColorRgb> &ledValues)
{
	if (_ledBuffer.size() == 0)
	{
		_ledBuffer.resize(ledValues.size() * 4 + 8, 0x00);
	}

	uint8_t * dataPtr = _ledBuffer.data();
	for (const ColorRgb & color : ledValues)
	{
		*dataPtr++ = calculateChecksum(color);
		*dataPtr++ = color.blue;
		*dataPtr++ = color.green;
		*dataPtr++ = color.red;
	}

	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
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
