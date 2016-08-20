#include <leddevice/LedDevice.h>

static LedDeviceRegistry _ledDeviceMap;

LedDevice::LedDevice()
	: QObject()
	, _log(Logger::getInstance("LedDevice"))
	, _ledCount(0)
	, _ledBuffer(0)

{
}

// dummy implemention
int LedDevice::open()
{
	return 0;
}

int LedDevice::addToDeviceMap(std::string name, LedDeviceCreateFuncType funcPtr)
{
	_ledDeviceMap.emplace(name,funcPtr);
	return 0;
}

const LedDeviceRegistry& LedDevice::getDeviceMap()
{
	return _ledDeviceMap;
}
