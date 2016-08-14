#include <leddevice/LedDevice.h>

LedDevice::LedDevice()
	: QObject()
	, _log(Logger::getInstance("LedDevice"))
	, _ledCount(0)
	, _ledBuffer(0)

{
}

int LedDevice::open()
{
	//dummy implemention
	return 0;
}
