// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceLdp6803.h"

LedDeviceLdp6803::LedDeviceLdp6803(const std::string& outputDevice, const unsigned baudrate) :
	LedSpiDevice(outputDevice, baudrate),
	_ledBuffer(0)
{
	// empty
}

int LedDeviceLdp6803::write(const std::vector<RgbColor> &ledValues)
{
	// Reconfigure if the current connfiguration does not match the required configuration
	if (ledValues.size() != _ledBuffer.size())
	{
		// Initialise the buffer with all 'black' values
		_ledBuffer.resize(ledValues.size() + 2, 0x80);
		_ledBuffer[0] = 0;
		_ledBuffer[1] = 0;
	}

	// Copy the colors from the RgbColor vector to the Ldp6803Rgb vector
	for (unsigned iLed=0; iLed<ledValues.size(); ++iLed)
	{
		const RgbColor& rgb = ledValues[iLed];

		const char packedRed   = rgb.red   & 0xf8;
		const char packedGreen = rgb.green & 0xf8;
		const char packedBlue  = rgb.blue  & 0xf8;
		const unsigned short packedRgb = 0x80 | (packedRed << 7) | (packedGreen << 2) | (packedBlue >> 3);

		_ledBuffer[iLed + 2] = packedRgb;
	}

	// Write the data
	const unsigned bufCnt = _ledBuffer.size() * sizeof(short);
	const char * bufPtr   = reinterpret_cast<const char *>(_ledBuffer.data());
	if (latch(bufCnt, bufPtr, 0) < 0)
	{
		return -1;
	}
	return 0;
}

int LedDeviceLdp6803::switchOff()
{
	return write(std::vector<RgbColor>(_ledBuffer.size(), RgbColor::BLACK));
}
