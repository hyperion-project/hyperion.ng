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
	// Initialise sub-class
	if ( !ProviderHID::init(deviceConfig) )
	{
		return false;
	}

	_ledBuffer.resize(_ledRGBCount);

	return true;
}

int LedDeviceRawHID::write(const QVector<ColorRgb> & ledValues)
{
	// write data
	memcpy(_ledBuffer.data(), ledValues.data(), _ledRGBCount);
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
