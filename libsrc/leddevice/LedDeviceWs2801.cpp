
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceWs2801.h"

LedDeviceWs2801::LedDeviceWs2801(const Json::Value &deviceConfig)
	: ProviderSpi(deviceConfig)
{
}

LedDevice* LedDeviceWs2801::construct(const Json::Value &deviceConfig)
{
	return new LedDeviceWs2801(deviceConfig);
}

int LedDeviceWs2801::write(const std::vector<ColorRgb> &ledValues)
{
	const unsigned dataLen = _ledCount * sizeof(ColorRgb);
	const uint8_t * dataPtr = reinterpret_cast<const uint8_t *>(ledValues.data());

	return writeBytes(dataLen, dataPtr);
}

