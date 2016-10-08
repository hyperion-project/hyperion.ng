#include "LedDeviceTpm2.h"


LedDeviceTpm2::LedDeviceTpm2(const Json::Value &deviceConfig)
	: ProviderRs232()
{
	_deviceReady = init(deviceConfig);
}

LedDevice* LedDeviceTpm2::construct(const Json::Value &deviceConfig)
{
	return new LedDeviceTpm2(deviceConfig);
}

bool LedDeviceTpm2::init(const Json::Value &deviceConfig)
{
	ProviderRs232::init(deviceConfig);

	_ledBuffer.resize(5 + 3*_ledCount);
	_ledBuffer[0] = 0xC9; // block-start byte
	_ledBuffer[1] = 0xDA; // DATA frame
	_ledBuffer[2] = ((3 * _ledCount) >> 8) & 0xFF; // frame size high byte
	_ledBuffer[3] = (3 * _ledCount) & 0xFF; // frame size low byte
	_ledBuffer.back() = 0x36; // block-end byte

	return true;
}

int LedDeviceTpm2::write(const std::vector<ColorRgb> &ledValues)
{
	memcpy(4 + _ledBuffer.data(), ledValues.data(), _ledRGBCount);
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
