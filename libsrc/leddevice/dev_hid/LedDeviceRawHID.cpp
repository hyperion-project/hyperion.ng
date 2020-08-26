#include "LedDeviceRawHID.h"

// Use feature report HID device
LedDeviceRawHID::LedDeviceRawHID(const QJsonObject &deviceConfig)
	: ProviderHID(deviceConfig)
{
	_useFeature = true;
}

LedDevice* LedDeviceRawHID::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceRawHID(deviceConfig);
}

bool LedDeviceRawHID::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = false;

	// Initialise sub-class
	if ( ProviderHID::init(deviceConfig) )
	{
		_ledBuffer.resize(_ledRGBCount);
		isInitOK = true;
	}
	return isInitOK;
}

int LedDeviceRawHID::write(const std::vector<ColorRgb> & ledValues)
{
	// write data
	memcpy(_ledBuffer.data(), ledValues.data(), _ledRGBCount);
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
