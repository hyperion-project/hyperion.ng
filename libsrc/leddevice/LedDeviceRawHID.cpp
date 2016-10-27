#include "LedDeviceRawHID.h"

// Use feature report HID device
LedDeviceRawHID::LedDeviceRawHID(const QJsonObject &deviceConfig)
	: ProviderHID()
	, _timer()
{
	ProviderHID::init(deviceConfig);
	_useFeature = true;
	_ledBuffer.resize(_ledRGBCount);

	// setup the timer
	_timer.setSingleShot(false);
	_timer.setInterval(5000);
	connect(&_timer, SIGNAL(timeout()), this, SLOT(rewriteLeds()));

	// start the timer
	_timer.start();
}

LedDevice* LedDeviceRawHID::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceRawHID(deviceConfig);
}

int LedDeviceRawHID::write(const std::vector<ColorRgb> & ledValues)
{
	// restart the timer
	_timer.start();

	// write data
	memcpy(_ledBuffer.data(), ledValues.data(), _ledRGBCount);
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

void LedDeviceRawHID::rewriteLeds()
{
	writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
