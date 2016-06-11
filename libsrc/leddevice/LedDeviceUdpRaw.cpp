
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceUdpRaw.h"

LedDeviceUdpRaw::LedDeviceUdpRaw(const std::string& outputDevice, const unsigned baudrate) :
	LedUdpDevice(outputDevice, baudrate, 500000),
	mLedCount(0)
{
	// empty
}

LedDeviceUdpRaw::LedDeviceUdpRaw(const std::string& outputDevice, const unsigned baudrate, const unsigned latchTime) :
	LedUdpDevice(outputDevice, baudrate, latchTime),
	mLedCount(0)
{
	// empty
}

int LedDeviceUdpRaw::write(const std::vector<ColorRgb> &ledValues)
{
	mLedCount = ledValues.size();

	const unsigned dataLen = ledValues.size() * sizeof(ColorRgb);
	const uint8_t * dataPtr = reinterpret_cast<const uint8_t *>(ledValues.data());

	return writeBytes(dataLen, dataPtr);
}

int LedDeviceUdpRaw::switchOff()
{
	return write(std::vector<ColorRgb>(mLedCount, ColorRgb{0,0,0}));
}
