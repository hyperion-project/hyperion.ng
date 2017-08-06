#include "LedDeviceTpm2.h"


LedDeviceTpm2::LedDeviceTpm2(const QJsonObject &deviceConfig)
	: ProviderRs232()
{
	_deviceReady = init(deviceConfig);
}

LedDevice* LedDeviceTpm2::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceTpm2(deviceConfig);
}

bool LedDeviceTpm2::init(const QJsonObject &deviceConfig)
{
	ProviderRs232::init(deviceConfig);

	_ledBuffer.resize(5 + _ledRGBCount);
	_ledBuffer[0] = 0xC9; // block-start byte
	_ledBuffer[1] = 0xDA; // DATA frame
	_ledBuffer[2] = (_ledRGBCount >> 8) & 0xFF; // frame size high byte
	_ledBuffer[3] = _ledRGBCount & 0xFF; // frame size low byte
	_ledBuffer.back() = 0x36; // block-end byte

	return true;
}

int LedDeviceTpm2::write(const std::vector<ColorRgb> &ledValues)
{
	memcpy(4 + _ledBuffer.data(), ledValues.data(), _ledRGBCount);
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
