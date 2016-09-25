#include "LedDeviceAdalight.h"

LedDeviceAdalight::LedDeviceAdalight(const Json::Value &deviceConfig)
	: ProviderRs232()

{
	_deviceReady = setConfig(deviceConfig);
}

LedDevice* LedDeviceAdalight::construct(const Json::Value &deviceConfig)
{
	return new LedDeviceAdalight(deviceConfig);
}

bool LedDeviceAdalight::setConfig(const Json::Value &deviceConfig)
{
	ProviderRs232::setConfig(deviceConfig);

	_ledBuffer.resize(6 + _ledRGBCount);
	_ledBuffer[0] = 'A';
	_ledBuffer[1] = 'd';
	_ledBuffer[2] = 'a';
	_ledBuffer[3] = (((unsigned int)_ledCount - 1) >> 8) & 0xFF; // LED count high byte
	_ledBuffer[4] = ((unsigned int)_ledCount - 1) & 0xFF;        // LED count low byte
	_ledBuffer[5] = _ledBuffer[3] ^ _ledBuffer[4] ^ 0x55; // Checksum
	Debug( _log, "Adalight header for %d leds: %c%c%c 0x%02x 0x%02x 0x%02x", _ledCount,
		_ledBuffer[0], _ledBuffer[1], _ledBuffer[2], _ledBuffer[3], _ledBuffer[4], _ledBuffer[5]
	);

	return true;
}

int LedDeviceAdalight::write(const std::vector<ColorRgb> & ledValues)
{
	memcpy(6 + _ledBuffer.data(), ledValues.data(), ledValues.size() * 3);
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

