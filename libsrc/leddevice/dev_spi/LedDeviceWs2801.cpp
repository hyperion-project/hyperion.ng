#include "LedDeviceWs2801.h"

LedDeviceWs2801::LedDeviceWs2801(const QJsonObject &deviceConfig)
	: ProviderSpi()
{
	_deviceReady = 	ProviderSpi::init(deviceConfig);
}

LedDevice* LedDeviceWs2801::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceWs2801(deviceConfig);
}

int LedDeviceWs2801::write(const std::vector<ColorRgb> &ledValues)
{
	const unsigned dataLen = _ledCount * sizeof(ColorRgb);
	const uint8_t * dataPtr = reinterpret_cast<const uint8_t *>(ledValues.data());

	return writeBytes(dataLen, dataPtr);
}
