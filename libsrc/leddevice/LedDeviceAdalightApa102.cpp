
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceAdalightApa102.h"

LedDeviceAdalightApa102::LedDeviceAdalightApa102(const std::string& outputDevice, const unsigned baudrate, int delayAfterConnect_ms) :
	LedDeviceAdalight(outputDevice, baudrate, delayAfterConnect_ms),
	_ledBuffer(0),
	_timer()
{
}
// see dependencies folder for arduino sketch for APA102
int LedDeviceAdalightApa102::write(const std::vector<ColorRgb> & ledValues)
{
	ledCount = ledValues.size();
	const unsigned int startFrameSize = 4;
	const unsigned int endFrameSize = std::max<unsigned int>(((ledValues.size() + 15) / 16), 4);
	const unsigned int mLedCount = (ledValues.size() * 4) + startFrameSize + endFrameSize;
	if(_ledBuffer.size() != mLedCount+6){
		_ledBuffer.resize(mLedCount+6, 0x00);
		_ledBuffer[0] = 'A';
		_ledBuffer[1] = 'd';
		_ledBuffer[2] = 'a';
		_ledBuffer[3] = (((unsigned int)(ledValues.size())) >> 8) & 0xFF; // LED count high byte
		_ledBuffer[4] = ((unsigned int)(ledValues.size())) & 0xFF;        // LED count low byte
		_ledBuffer[5] = _ledBuffer[3] ^ _ledBuffer[4] ^ 0x55; // Checksum
	}

	for (unsigned iLed=1; iLed<=ledValues.size(); iLed++) {
		const ColorRgb& rgb = ledValues[iLed-1];
		_ledBuffer[iLed*4+6]   = 0xFF;
		_ledBuffer[iLed*4+1+6] = rgb.red;
		_ledBuffer[iLed*4+2+6] = rgb.green;
		_ledBuffer[iLed*4+3+6] = rgb.blue;
	}
	
	// restart the timer
	_timer.start();

	// write data
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

int LedDeviceAdalightApa102::switchOff()
{
	for (unsigned iLed=1; iLed<=ledCount; iLed++) {
		_ledBuffer[iLed*4+6]   = 0xFF;
		_ledBuffer[iLed*4+1+6] = 0x00;
		_ledBuffer[iLed*4+2+6] = 0x00;
		_ledBuffer[iLed*4+3+6] = 0x00;
	}
	// restart the timer
	_timer.start();

	// write data
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

void LedDeviceAdalightApa102::rewriteLeds()
{
	writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

