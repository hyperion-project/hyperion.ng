
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceUdpRaw.h"

LedDeviceUdpRaw::LedDeviceUdpRaw(const Json::Value &deviceConfig)
	: ProviderUdp()
{
	setConfig(deviceConfig, 500000, 5568);
}

LedDevice* LedDeviceUdpRaw::construct(const Json::Value &deviceConfig)
{
	return new LedDeviceUdpRaw(deviceConfig);
}

int LedDeviceUdpRaw::write(const std::vector<ColorRgb> &ledValues)
{
	const unsigned dataLen = _ledCount * sizeof(ColorRgb);
	const uint8_t * dataPtr = reinterpret_cast<const uint8_t *>(ledValues.data());

	return writeBytes(dataLen, dataPtr);
}
