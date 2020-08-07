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
	bool isInitOK = false;

	// Initialise sub-class
	if ( ProviderHID::init(deviceConfig) )
	{
		_ledBuffer.resize(_ledRGBCount + 2, uint8_t(0));
		_ledBuffer[0] = 3;
		_ledBuffer[1] = 0;

		isInitOK = true;
	}
	return isInitOK;
}

int LedDevicePaintpack::write(const std::vector<ColorRgb> & ledValues)
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
