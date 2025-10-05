#include "LedDeviceLpd6803.h"

LedDeviceLpd6803::LedDeviceLpd6803(const QJsonObject &deviceConfig)
	: ProviderSpi(deviceConfig)
{
}

LedDevice* LedDeviceLpd6803::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceLpd6803(deviceConfig);
}

bool LedDeviceLpd6803::init(const QJsonObject &deviceConfig)
{
	// Initialise sub-class
	if ( !ProviderSpi::init(deviceConfig) )
	{
		return false;
	}

	unsigned messageLength = 4 + 2*_ledCount + _ledCount/8 + 1;
	// Initialise the buffer
	_ledBuffer.fill(0x00, messageLength);

	return true;
}

int LedDeviceLpd6803::write(const QVector<ColorRgb> &ledValues)
{
	// Copy the colors from the ColorRgb vector to the Ldp6803 data vector
	for (unsigned iLed=0; iLed<_ledCount; ++iLed)
	{
		const ColorRgb& color = ledValues[iLed];

		_ledBuffer[4 + 2 * iLed] = 0x80 | ((color.red & 0xf8) >> 1) | (color.green >> 6);
		_ledBuffer[5 + 2 * iLed] = ((color.green & 0x38) << 2) | (color.blue >> 3);
	}

	// Write the data
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
