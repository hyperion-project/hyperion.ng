
// STL includes
#include <cstring>
#include <iostream>

// hyperion local includes
#include "LedDeviceAtmo.h"

LedDeviceAtmo::LedDeviceAtmo(const std::string& outputDevice, const unsigned baudrate) :
	LedRs232Device(outputDevice, baudrate),
	_ledBuffer(4 + 5*3) // 4-byte header, 5 RGB values
{
	_ledBuffer[0] = 0xFF; // Startbyte
	_ledBuffer[1] = 0x00; // StartChannel(Low)
	_ledBuffer[2] = 0x00; // StartChannel(High)
	_ledBuffer[3] = 0x0F; // Number of Databytes send (always! 15)
}

int LedDeviceAtmo::write(const std::vector<ColorRgb> &ledValues)
{
	// The protocol is shomehow limited. we always need to send exactly 5 channels + header
	// (19 bytes) for the hardware to recognize the data
	if (ledValues.size() != 5)
	{
			std::cerr << "AtmoLight: " << ledValues.size() << " channels configured. This should always be 5!" << std::endl;
			return 0;
	}

	// write data
	memcpy(4 + _ledBuffer.data(), ledValues.data(), ledValues.size() * sizeof(ColorRgb));
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

int LedDeviceAtmo::switchOff()
{
	memset(4 + _ledBuffer.data(), 0, _ledBuffer.size() - 4);
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
