#include "LedDeviceLpd8806.h"

LedDeviceLpd8806::LedDeviceLpd8806(const QJsonObject &deviceConfig)
	: ProviderSpi(deviceConfig)
{
}

LedDevice* LedDeviceLpd8806::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceLpd8806(deviceConfig);
}

bool LedDeviceLpd8806::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = false;

	// Initialise sub-class
	if ( ProviderSpi::init(deviceConfig) )
	{
		const unsigned clearSize = _ledCount/32+1;
		unsigned messageLength = _ledRGBCount + clearSize;
		// Initialise the buffer
		_ledBuffer.resize(messageLength, 0x00);

		isInitOK = true;
	}
	return isInitOK;
}

int LedDeviceLpd8806::write(const std::vector<ColorRgb> &ledValues)
{
	// Copy the colors from the ColorRgb vector to the Ldp8806 data vector
	for (unsigned iLed=0; iLed<(unsigned)_ledCount; ++iLed)
	{
		const ColorRgb& color = ledValues[iLed];

		_ledBuffer[iLed*3]   = 0x80 | (color.red   >> 1);
		_ledBuffer[iLed*3+1] = 0x80 | (color.green >> 1);
		_ledBuffer[iLed*3+2] = 0x80 | (color.blue  >> 1);
	}

	// Write the data
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
