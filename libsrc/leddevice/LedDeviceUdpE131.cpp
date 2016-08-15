
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceUdpE131.h"

LedDeviceUdpE131::LedDeviceUdpE131(const std::string& outputDevice, const unsigned latchTime)
	: LedUdpDevice(outputDevice, latchTime)
{
	// empty
}

int LedDeviceUdpE131::write(const std::vector<ColorRgb> &ledValues)
{
	_ledCount = ledValues.size();

	const unsigned dataLen = _ledCount * sizeof(ColorRgb);
	const uint8_t * dataPtr = reinterpret_cast<const uint8_t *>(ledValues.data());

	return writeBytes(dataLen, dataPtr);
}

int LedDeviceUdpE131::switchOff()
{
	return write(std::vector<ColorRgb>(_ledCount, ColorRgb{0,0,0}));
}
