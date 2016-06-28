
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceSk6812SPI.h"

LedDeviceSk6812SPI::LedDeviceSk6812SPI(const std::string& outputDevice, const unsigned baudrate) :
	LedSpiDevice(outputDevice, baudrate, 0),
	mLedCount(0),
	bitpair_to_byte {
		0b10001000,
		0b10001100,
		0b11001000,
		0b11001100,
	}

{
	// empty
}

int LedDeviceSk6812SPI::write(const std::vector<ColorRgb> &ledValues)
{
	mLedCount = ledValues.size();

// 4 colours, 4 spi bytes per colour + 3 frame end latch bytes
#define COLOURS_PER_LED		3
#define SPI_BYTES_PER_COLOUR	4
#define SPI_BYTES_PER_LED 	COLOURS_PER_LED * SPI_BYTES_PER_COLOUR

	unsigned spi_size = mLedCount * SPI_BYTES_PER_LED + 3;
	if(_spiBuffer.size() != spi_size){
                _spiBuffer.resize(spi_size, 0x00);
	}

	unsigned spi_ptr = 0;
        for (unsigned i=0; i< mLedCount; ++i) {
		uint8_t white = 0;
		uint32_t colorBits = 
			((unsigned int)ledValues[i].red << 24) |
			((unsigned int)ledValues[i].green << 16) |
			((unsigned int)ledValues[i].blue << 8) |
			white;

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

int LedDeviceSk6812SPI::switchOff()
{
	return write(std::vector<ColorRgb>(mLedCount, ColorRgb{0,0,0}));
}
