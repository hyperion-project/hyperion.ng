
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceAPA102.h"

LedDeviceAPA102::LedDeviceAPA102(const std::string& outputDevice, const unsigned baudrate) :
	LedSpiDevice(outputDevice, baudrate, 500000),
	mLedCount(0)
{
	// empty
}

int LedDeviceAPA102::write(const std::vector<ColorRgb> &ledValues)
{
	mLedCount = ledValues.size();

	const unsigned dataLen = 8 + (ledValues.size() * (sizeof(ColorRgb) + 1));
	const uint8_t data[dataLen] = { 0x00, 0x00, 0x00, 0x00 };
	int position = 4;
	for (ColorRgb* rgb : ledValues ){
		data[i++] = 0xFF;
		data[i++] = rgb.red;
		data[i++] = rgb.green;
		data[i++] = rgb.blue;
	}

	// Write end frame
	data[i++] = 0xFF;
	data[i++] = 0xFF;
	data[i++] = 0xFF;
	data[i++] = 0xFF;

	return writeBytes(dataLen, data);
}

int LedDeviceAPA102::switchOff()
{
	return write(std::vector<ColorRgb>(mLedCount, ColorRgb{0,0,0}));
}
