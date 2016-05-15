
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceWs2812SPI.h"

LedDeviceWs2812SPI::LedDeviceWs2812SPI(const std::string& outputDevice, const unsigned baudrate) :
	LedSpiDevice(outputDevice, baudrate, 0),
	mLedCount(0)
{
	// empty
}

int LedDeviceWs2812SPI::write(const std::vector<ColorRgb> &ledValues)
{
	mLedCount = ledValues.size();

// 3 colours, 4 spi bytes per colour + 3 frame end latch bytes
#define COLOURS_PER_LED		3
#define SPI_BYTES_PER_COLOUR	4
#define SPI_BYTES_PER_LED 	COLOURS_PER_LED * SPI_BYTES_PER_COLOUR

	unsigned spi_size = mLedCount * SPI_BYTES_PER_LED + 3;
	if(_spiBuffer.size() != spi_size){
                _spiBuffer.resize(spi_size, 0x00);
	}

	unsigned spi_ptr = 0;
        for (unsigned i=0; i< mLedCount; ++i) {
		uint32_t colorBits = ((unsigned int)ledValues[i].red << 16) 
			| ((unsigned int)ledValues[i].green << 8) 
			| ledValues[i].blue;

		for (int j=SPI_BYTES_PER_LED - 1; j>=0; j--) {
			_spiBuffer[spi_ptr+j] = bitpair_to_byte[ colorBits & 0x3 ];
			colorBits >>= 2;
		}
		spi_ptr += SPI_BYTES_PER_LED;
        }
	_spiBuffer[spi_ptr++] = 0;
	_spiBuffer[spi_ptr++] = 0;
	_spiBuffer[spi_ptr++] = 0;

	return writeBytes(spi_size, _spiBuffer.data());
}

int LedDeviceWs2812SPI::switchOff()
{
	return write(std::vector<ColorRgb>(mLedCount, ColorRgb{0,0,0}));
}
