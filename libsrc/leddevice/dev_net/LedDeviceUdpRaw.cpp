#include "LedDeviceUdpRaw.h"

LedDeviceUdpRaw::LedDeviceUdpRaw(const QJsonObject &deviceConfig)
	: ProviderUdp()
{
	_devConfig = deviceConfig;
	_deviceReady = false;
}

LedDevice* LedDeviceUdpRaw::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceUdpRaw(deviceConfig);
}

bool LedDeviceUdpRaw::init(const QJsonObject &deviceConfig)
{
	_port = RAW_DEFAULT_PORT;
	bool isInitOK = ProviderUdp::init(deviceConfig);
	return isInitOK;
}

int LedDeviceUdpRaw::write(const std::vector<ColorRgb> &ledValues)
{
	const uint8_t * dataPtr = reinterpret_cast<const uint8_t *>(ledValues.data());

	return writeBytes((unsigned)_ledRGBCount, dataPtr);
}
