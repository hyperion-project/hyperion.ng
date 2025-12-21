#include "LedDevicePaintpack.h"

// Use out report HID device
LedDevicePaintpack::LedDevicePaintpack(const QJsonObject &deviceConfig)
	: ProviderHID(deviceConfig)
{
	_useFeature = false;
}

LedDevice* LedDevicePaintpack::construct(const QJsonObject &deviceConfig)
{
	return new LedDevicePaintpack(deviceConfig);
}

bool LedDevicePaintpack::init(const QJsonObject &deviceConfig)
{
	// Initialise sub-class
	if ( !ProviderHID::init(deviceConfig) )
	{
		return false;
	}

	_ledBuffer.fill(0x00, _ledRGBCount + 2);
	_ledBuffer[0] = 3;
	_ledBuffer[1] = 0;

	return true;
}

int LedDevicePaintpack::write(const QVector<ColorRgb> & ledValues)
{
	auto bufIt = _ledBuffer.begin()+2;
	for (const ColorRgb & color : ledValues)
	{
		*bufIt = color.red;
		++bufIt;
		*bufIt = color.green;
		++bufIt;
		*bufIt = color.blue;
		++bufIt;
	}

	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
