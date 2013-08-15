#pragma once

// STL includes0
#include <fstream>

// Hyperion includes
#include <hyperion/LedDevice.h>

class LedDeviceTest : public LedDevice
{
public:
	LedDeviceTest();
	virtual ~LedDeviceTest();

	virtual int write(const std::vector<RgbColor> & ledValues);

private:
	std::ofstream _ofs;
};
