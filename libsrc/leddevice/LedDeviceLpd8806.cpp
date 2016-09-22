#include "LedDeviceLpd8806.h"

LedDeviceLpd8806::LedDeviceLpd8806(const Json::Value &deviceConfig)
	: ProviderSpi(deviceConfig)
{
}

LedDevice* LedDeviceLpd8806::construct(const Json::Value &deviceConfig)
{
	return new LedDeviceLpd8806(deviceConfig);
}

int LedDeviceLpd8806::write(const std::vector<ColorRgb> &ledValues)
{
	const unsigned clearSize = _ledCount/32+1;
	unsigned messageLength = 3*_ledCount + clearSize;
	// Reconfigure if the current connfiguration does not match the required configuration
	if (messageLength != _ledBuffer.size())
	{
		// Initialise the buffer
		_ledBuffer.resize(messageLength, 0x00);

		// Perform an initial reset to start accepting data on the first led
		writeBytes(clearSize, _ledBuffer.data());
	}

	// Copy the colors from the ColorRgb vector to the Ldp8806 data vector
	for (unsigned iLed=0; iLed<(unsigned)_ledCount; ++iLed)
	{
		const ColorRgb& rgb = ledValues[iLed];

		_ledBuffer[iLed*3]   = 0x80 | (rgb.red   >> 1);
		_ledBuffer[iLed*3+1] = 0x80 | (rgb.green >> 1);
		_ledBuffer[iLed*3+2] = 0x80 | (rgb.blue  >> 1);
	}

	// Write the data
	return (writeBytes(_ledBuffer.size(), _ledBuffer.data()) < 0) ? -1 : 0;
}
