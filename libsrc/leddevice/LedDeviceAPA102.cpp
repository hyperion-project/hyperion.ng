
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

LedDeviceAPA102::LedDeviceAPA102(const std::string& outputDevice, const unsigned baudrate, const unsigned ledcount) :
	LedSpiDevice(outputDevice, baudrate, 500000),
	_ledBuffer(0)
{
	_HW_ledcount = ledcount;
}

int LedDeviceAPA102::write(const std::vector<ColorRgb> &ledValues)
{
	_mLedCount = ledValues.size();
	const unsigned int max_leds = std::max(_mLedCount, _HW_ledcount);
	const unsigned int startFrameSize = 4;
	const unsigned int endFrameSize = std::max<unsigned int>(((max_leds + 15) / 16), 4);
	const unsigned int APAbufferSize = (max_leds * 4) + startFrameSize + endFrameSize;

	if(_ledBuffer.size() != APAbufferSize){
		_ledBuffer.resize(APAbufferSize, 0xFF);
		_ledBuffer[0] = 0x00; 
		_ledBuffer[1] = 0x00; 
		_ledBuffer[2] = 0x00; 
		_ledBuffer[3] = 0x00; 
	}
	
	unsigned iLed=0;
	for (iLed=0; iLed < _mLedCount; ++iLed) {
		const ColorRgb& rgb = ledValues[iLed];
		_ledBuffer[4+iLed*4]   = 0xFF;
		_ledBuffer[4+iLed*4+1] = rgb.red;
		_ledBuffer[4+iLed*4+2] = rgb.green;
		_ledBuffer[4+iLed*4+3] = rgb.blue;
	}

	for ( ; iLed < max_leds; ++iLed) {
		_ledBuffer[4+iLed*4]   = 0xFF;
		_ledBuffer[4+iLed*4+1] = 0x00;
		_ledBuffer[4+iLed*4+2] = 0x00;
		_ledBuffer[4+iLed*4+3] = 0x00;
	}

	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

int LedDeviceAPA102::switchOff()
{
	return write(std::vector<ColorRgb>(_mLedCount, ColorRgb{0,0,0}));
}
