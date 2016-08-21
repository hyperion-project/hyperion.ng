// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceLpd6803.h"

LedDeviceLpd6803::LedDeviceLpd6803(const Json::Value &deviceConfig)
	: LedSpiDevice(deviceConfig)
{
}

LedDevice* LedDeviceLpd6803::construct(const Json::Value &deviceConfig)
{
	return new LedDeviceLpd6803(deviceConfig);
}

int LedDeviceLpd6803::write(const std::vector<ColorRgb> &ledValues)
{
	unsigned messageLength = 4 + 2*ledValues.size() + ledValues.size()/8 + 1;
	// Reconfigure if the current connfiguration does not match the required configuration
	if (messageLength != _ledBuffer.size())
	{
		// Initialise the buffer
		_ledBuffer.resize(messageLength, 0x00);
	}

	// Copy the colors from the ColorRgb vector to the Ldp6803 data vector
	for (unsigned iLed=0; iLed<ledValues.size(); ++iLed)
	{
		const ColorRgb& rgb = ledValues[iLed];

		_ledBuffer[4 + 2 * iLed] = 0x80 | ((rgb.red & 0xf8) >> 1) | (rgb.green >> 6);
		_ledBuffer[5 + 2 * iLed] = ((rgb.green & 0x38) << 2) | (rgb.blue >> 3);
	}

	// Write the data
	if (writeBytes(_ledBuffer.size(), _ledBuffer.data()) < 0)
	{
		return -1;
	}
	return 0;
}

int LedDeviceLpd6803::switchOff()
{
	return write(std::vector<ColorRgb>(_ledBuffer.size(), ColorRgb{0,0,0}));
}
