#include <leddevice/LedDevice.h>

LedDevice::LedDevice()
	: _log(Logger::getInstance("LedDevice"))
{
}

int LedDevice::open()
{
	//dummy implemention
	return 0;
}
