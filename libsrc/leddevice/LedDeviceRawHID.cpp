#include "LedDeviceRawHID.h"

// Use feature report HID device
LedDeviceRawHID::LedDeviceRawHID(const QJsonObject &deviceConfig)
	: ProviderHID()
{
	ProviderHID::init(deviceConfig);
	_useFeature = true;
	_ledBuffer.resize(_ledRGBCount);
}

LedDevice* LedDeviceRawHID::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceRawHID(deviceConfig);
}

int LedDeviceRawHID::write(const std::vector<ColorRgb> & ledValues)
{
	// write data
	memcpy(_ledBuffer.data(), ledValues.data(), _ledRGBCount);
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

void LedDeviceRawHID::rewriteLeds()
{
	writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
