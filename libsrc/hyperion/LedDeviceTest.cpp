
// Local-Hyperion includes
#include "LedDeviceTest.h"

LedDeviceTest::LedDeviceTest() :
	_ofs("/home/pi/LedDevice.out")
{
	// empty
}

LedDeviceTest::~LedDeviceTest()
{
	// empty
}

int LedDeviceTest::write(const std::vector<RgbColor> & ledValues)
{
	_ofs << "[";
	for (const RgbColor& color : ledValues)
	{
		_ofs << color;
	}
	_ofs << "]" << std::endl;

	return 0;
}
