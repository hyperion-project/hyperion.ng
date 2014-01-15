
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
	mLedCount(0)
{
	// empty
}

int LedDeviceP9813::write(const std::vector<ColorRgb> &ledValues)
{
	mLedCount = ledValues.size();

	const unsigned dataLen = ledValues.size() * sizeof(ColorRgb);
	const uint8_t * dataPtr = reinterpret_cast<const uint8_t *>(ledValues.data());

	return writeBytes(dataLen, dataPtr);
}

int LedDeviceP9813::switchOff()
{
	return write(std::vector<ColorRgb>(mLedCount, ColorRgb{0,0,0}));
}
