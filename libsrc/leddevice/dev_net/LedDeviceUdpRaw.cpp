#include "LedDeviceUdpRaw.h"

const ushort RAW_DEFAULT_PORT=5568;

LedDeviceUdpRaw::LedDeviceUdpRaw(const QJsonObject &deviceConfig)
	: ProviderUdp(deviceConfig)
{
}

LedDevice* LedDeviceUdpRaw::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceUdpRaw(deviceConfig);
}

bool LedDeviceUdpRaw::init(const QJsonObject &deviceConfig)
{
	_port = RAW_DEFAULT_PORT;

	// Initialise sub-class
	bool isInitOK = ProviderUdp::init(deviceConfig);
	return isInitOK;
}

int LedDeviceUdpRaw::write(const std::vector<ColorRgb> &ledValues)
{
	const uint8_t * dataPtr = reinterpret_cast<const uint8_t *>(ledValues.data());

	return writeBytes(_ledRGBCount, dataPtr);
}
