
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
	: ProviderUdp(deviceConfig)
{
	setConfig(deviceConfig);
}

bool LedDeviceUdpRaw::setConfig(const Json::Value &deviceConfig)
{
	ProviderUdp::setConfig(deviceConfig,5568);
	_LatchTime_ns = deviceConfig.get("latchtime",500000).asInt();

	return true;
}

LedDevice* LedDeviceUdpRaw::construct(const Json::Value &deviceConfig)
{
	return new LedDeviceUdpRaw(deviceConfig);
}

int LedDeviceUdpRaw::write(const std::vector<ColorRgb> &ledValues)
{
	_ledCount = ledValues.size();

	const unsigned dataLen = _ledCount * sizeof(ColorRgb);
	const uint8_t * dataPtr = reinterpret_cast<const uint8_t *>(ledValues.data());

	return writeBytes(dataLen, dataPtr);
}

int LedDeviceUdpRaw::switchOff()
{
	return write(std::vector<ColorRgb>(_ledCount, ColorRgb{0,0,0}));
}
