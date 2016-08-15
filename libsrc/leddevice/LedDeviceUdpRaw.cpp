
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceUdpRaw.h"

LedDeviceUdpRaw::LedDeviceUdpRaw(const std::string& outputDevice, const unsigned latchTime)
	: LedUdpDevice(outputDevice, latchTime)
{
	// empty
}

int LedDeviceUdpRaw::write(const std::vector<ColorRgb> &ledValues)
{
	_ledCount = ledValues.size();

	const unsigned dataLen = _ledCount * sizeof(ColorRgb);
	const uint8_t * dataPtr = reinterpret_cast<const uint8_t *>(ledValues.data());

	return writeBytes(dataLen, dataPtr);
}

int LedDeviceUdpRaw::switchOff()
{
	return write(std::vector<ColorRgb>(_ledCount, ColorRgb{0,0,0}));
}
