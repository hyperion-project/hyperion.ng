
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

LedDeviceAPA102::LedDeviceAPA102(const std::string& outputDevice, const unsigned baudrate) :
	LedSpiDevice(outputDevice, baudrate, 500000),
	_ledBuffer(0)
{
	// empty
}

int LedDeviceAPA102::write(const std::vector<ColorRgb> &ledValues)
{
	const unsigned int startFrameSize = 4;
	const unsigned int endFrameSize = std::max<unsigned int>(((ledValues.size() + 15) / 16), 4);
	const unsigned int mLedCount = (ledValues.size() * 4) + startFrameSize + endFrameSize;
	if(_ledBuffer.size() != mLedCount){
		_ledBuffer.resize(mLedCount, 0xFF);
		_ledBuffer[0] = 0x00; 
		_ledBuffer[1] = 0x00; 
		_ledBuffer[2] = 0x00; 
		_ledBuffer[3] = 0x00; 
	}
	
	for (unsigned iLed=1; iLed<=ledValues.size(); ++iLed) {
		const ColorRgb& rgb = ledValues[iLed-1];
		_ledBuffer[iLed*4]   = 0xFF;
		_ledBuffer[iLed*4+1] = rgb.red;
		_ledBuffer[iLed*4+2] = rgb.green;
		_ledBuffer[iLed*4+3] = rgb.blue;
	}

	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

int LedDeviceAPA102::switchOff()
{
	return write(std::vector<ColorRgb>(_ledBuffer.size(), ColorRgb{0,0,0}));
}
