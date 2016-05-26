// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceLpd8806.h"

LedDeviceLpd8806::LedDeviceLpd8806(const std::string& outputDevice, const unsigned baudrate) :
	LedSpiDevice(outputDevice, baudrate),
	_ledBuffer(0)
{
	// empty
}

int LedDeviceLpd8806::write(const std::vector<ColorRgb> &ledValues)
{
	const unsigned clearSize = ledValues.size()/32+1;
	// Reconfigure if the current connfiguration does not match the required configuration
	if (3*ledValues.size() + clearSize != _ledBuffer.size())
	{
		// Initialise the buffer
		_ledBuffer.resize(3*ledValues.size() + clearSize, 0x00);

		// Perform an initial reset to start accepting data on the first led
		writeBytes(clearSize, _ledBuffer.data());
	}

	// Copy the colors from the ColorRgb vector to the Ldp8806 data vector
	for (unsigned iLed=0; iLed<ledValues.size(); ++iLed)
	{
		const ColorRgb& rgb = ledValues[iLed];

		_ledBuffer[iLed*3]   = 0x80 | (rgb.red   >> 1);
		_ledBuffer[iLed*3+1] = 0x80 | (rgb.green >> 1);
		_ledBuffer[iLed*3+2] = 0x80 | (rgb.blue  >> 1);
	}

	// Write the data
	if (writeBytes(_ledBuffer.size(), _ledBuffer.data()) < 0)
	{
		return -1;
	}
	return 0;
}

int LedDeviceLpd8806::switchOff()
{
	return write(std::vector<ColorRgb>(_ledBuffer.size(), ColorRgb{0,0,0}));
}
