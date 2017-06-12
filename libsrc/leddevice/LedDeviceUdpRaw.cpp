#include "LedDeviceUdpRaw.h"

LedDeviceUdpRaw::LedDeviceUdpRaw(const QJsonObject &deviceConfig)
	: ProviderUdp()
{
	_port = 5568;
	init(deviceConfig);
}

LedDevice* LedDeviceUdpRaw::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceUdpRaw(deviceConfig);
}

int LedDeviceUdpRaw::write(const std::vector<ColorRgb> &ledValues)
{
	const uint8_t * dataPtr = reinterpret_cast<const uint8_t *>(ledValues.data());

	return writeBytes((unsigned)_ledRGBCount, dataPtr);
}
