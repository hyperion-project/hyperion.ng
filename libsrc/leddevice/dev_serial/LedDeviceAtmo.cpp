// hyperion local includes
#include "LedDeviceAtmo.h"

LedDeviceAtmo::LedDeviceAtmo(const QJsonObject &deviceConfig)
	: ProviderRs232()
{
	_deviceReady = init(deviceConfig);
}

LedDevice* LedDeviceAtmo::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceAtmo(deviceConfig);
}

bool LedDeviceAtmo::init(const QJsonObject &deviceConfig)
{
	ProviderRs232::init(deviceConfig);

	if (_ledCount != 5)
	{
		Error( _log, "%d channels configured. This should always be 5!", _ledCount);
		return 0;
	}

	_ledBuffer.resize(4 + 5*3); // 4-byte header, 5 RGB values
	_ledBuffer[0] = 0xFF;       // Startbyte
	_ledBuffer[1] = 0x00;       // StartChannel(Low)
	_ledBuffer[2] = 0x00;       // StartChannel(High)
	_ledBuffer[3] = 0x0F;       // Number of Databytes send (always! 15)

	return true;
}

int LedDeviceAtmo::write(const std::vector<ColorRgb> &ledValues)
{
	memcpy(4 + _ledBuffer.data(), ledValues.data(), _ledCount * sizeof(ColorRgb));
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
