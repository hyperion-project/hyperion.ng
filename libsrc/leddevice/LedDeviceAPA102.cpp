
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>
#include <algorithm>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceAPA102.h"

LedDeviceAPA102::LedDeviceAPA102(const Json::Value &deviceConfig)
	: ProviderSpi(deviceConfig)
{
	_latchTime_ns = 500000; // fixed latchtime
}

LedDevice* LedDeviceAPA102::construct(const Json::Value &deviceConfig)
{
	return new LedDeviceAPA102(deviceConfig);
}

int LedDeviceAPA102::write(const std::vector<ColorRgb> &ledValues)
{
	const unsigned int startFrameSize = 4;
	const unsigned int endFrameSize = std::max<unsigned int>(((_ledCount + 15) / 16), 4);
	const unsigned int APAbufferSize = (_ledCount * 4) + startFrameSize + endFrameSize;

	if(_ledBuffer.size() != APAbufferSize){
		_ledBuffer.resize(APAbufferSize, 0xFF);
		_ledBuffer[0] = 0x00; 
		_ledBuffer[1] = 0x00; 
		_ledBuffer[2] = 0x00; 
		_ledBuffer[3] = 0x00; 
	}
	
	for (signed iLed=0; iLed < _ledCount; ++iLed) {
		const ColorRgb& rgb = ledValues[iLed];
		_ledBuffer[4+iLed*4]   = 0xFF;
		_ledBuffer[4+iLed*4+1] = rgb.red;
		_ledBuffer[4+iLed*4+2] = rgb.green;
		_ledBuffer[4+iLed*4+3] = rgb.blue;
	}

	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
