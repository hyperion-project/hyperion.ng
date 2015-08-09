// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

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

#define MIN(a,b)	((a)<(b)?(a):(b))
#define MAX(a,b)	((a)>(b)?(a):(b))

#define APA102_START_FRAME_BYTES	4
#define APA102_LED_BYTES		4
#define APA102_END_FRAME_BITS_MIN	32
#define APA102_END_FRAME_BITS(leds)	MAX((((leds-1)/2)+1),APA102_END_FRAME_BITS_MIN)
#define APA102_END_FRAME_BYTES(leds)	(((APA102_END_FRAME_BITS(leds)-1)/8)+1)
#define APA102_LED_HEADER		0xe0
#define APA102_LED_MAX_INTENSITY	0x1f

int LedDeviceAPA102::write(const std::vector<ColorRgb> &ledValues)
{
	const unsigned int startFrameSize = APA102_START_FRAME_BYTES;
        const unsigned int ledsCount = ledValues.size() ;
        const unsigned int ledsSize = ledsCount * APA102_LED_BYTES ;
        const unsigned int endFrameBits = APA102_END_FRAME_BITS(ledsCount) ;
        const unsigned int endFrameSize = APA102_END_FRAME_BYTES(ledsCount) ;
        const unsigned int transferSize = startFrameSize + ledsSize + endFrameSize ;

	if(_ledBuffer.size() != transferSize){
		_ledBuffer.resize(transferSize, 0x00);
	} 

	unsigned idx = 0, i;
	for (i=0; i<APA102_START_FRAME_BYTES; i++) {
		_ledBuffer[idx++] = 0x00 ;
	}

	for (i=0; i<ledsCount; i++) {
		const ColorRgb& rgb = ledValues[i];

		_ledBuffer[idx++]   = APA102_LED_HEADER + APA102_LED_MAX_INTENSITY;
		_ledBuffer[idx++] = rgb.red;
		_ledBuffer[idx++] = rgb.green;
		_ledBuffer[idx++] = rgb.blue;
	}
	for(i=0; i<endFrameSize; i++)
	    _ledBuffer[idx++] = 0x00 ;

	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

int LedDeviceAPA102::switchOff()
{
	return write(std::vector<ColorRgb>(_ledBuffer.size(), ColorRgb{0,0,0}));
}   
