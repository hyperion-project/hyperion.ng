
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceP9813.h"

LedDeviceP9813::LedDeviceP9813(const std::string& outputDevice, const unsigned baudrate) :
	LedSpiDevice(outputDevice, baudrate, 0),
	mLedCount(0)
{
	// empty
}

int LedDeviceP9813::write(const std::vector<ColorRgb> &ledValues)
{
	mLedCount = ledValues.size();

	const unsigned dataLen = ledValues.size() * 4 + 8;
	uint8_t data[dataLen];

    memset(data, 0x00, dataLen);

    int j = 4;
    for (unsigned i = 0; i < mLedCount; i++){
        data[j++] = calculateChecksum(ledValues[i]);
        data[j++] = ledValues[i].blue;        
        data[j++] = ledValues[i].green;        
        data[j++] = ledValues[i].red;        
    }

	return writeBytes(dataLen, data);
}

int LedDeviceP9813::switchOff()
{
	return write(std::vector<ColorRgb>(mLedCount, ColorRgb{0,0,0}));
}

const uint8_t LedDeviceP9813::calculateChecksum(const ColorRgb color)
{
    uint8_t res = 0;

    res |= (uint8_t)0x03 << 6;  
    res |= (uint8_t)(~(color.blue  >> 6) & 0x03) << 4; 
    res |= (uint8_t)(~(color.green >> 6) & 0x03) << 2;    
    res |= (uint8_t)(~(color.red   >> 6) & 0x03); 

    return res;
}
