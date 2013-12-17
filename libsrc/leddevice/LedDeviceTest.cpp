
// Local-Hyperion includes
#include "LedDeviceTest.h"

LedDeviceTest::LedDeviceTest(const std::string& output) :
	_ofs(output.empty()?"/home/pi/LedDevice.out":output.c_str())
{
	// empty
}

LedDeviceTest::~LedDeviceTest()
{
	// empty
}

int LedDeviceTest::write(const std::vector<ColorRgb> & ledValues)
{
	_ofs << "[";
	for (const ColorRgb& color : ledValues)
	{
		_ofs << color;
	}
	_ofs << "]" << std::endl;

	return 0;
}

int LedDeviceTest::switchOff()
{
	return 0;
}
