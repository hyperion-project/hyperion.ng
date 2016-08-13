
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceSk6812SPI.h"

LedDeviceSk6812SPI::LedDeviceSk6812SPI(const std::string& outputDevice, const unsigned baudrate, const std::string& whiteAlgorithm,
                                                const int spiMode, const bool spiDataInvert)
	: LedSpiDevice(outputDevice, baudrate, 0, spiMode, spiDataInvert)
	, mLedCount(0)
	, _whiteAlgorithm(whiteAlgorithm)
	, bitpair_to_byte {
		0b10001000,
		0b10001100,
		0b11001000,
		0b11001100,
	}

{
	Debug( _log, "whiteAlgorithm : %s", whiteAlgorithm.c_str());
}

int LedDeviceSk6812SPI::write(const std::vector<ColorRgb> &ledValues)
{
	mLedCount = ledValues.size();

// 4 colours, 4 spi bytes per colour + 3 frame end latch bytes
#define COLOURS_PER_LED		4
#define SPI_BYTES_PER_COLOUR	4
#define SPI_BYTES_PER_LED 	COLOURS_PER_LED * SPI_BYTES_PER_COLOUR

	unsigned spi_size = mLedCount * SPI_BYTES_PER_LED + 3;
	if(_spiBuffer.size() != spi_size){
                _spiBuffer.resize(spi_size, 0x00);
	}

	unsigned spi_ptr = 0;

        for (const ColorRgb& color : ledValues) {
		Rgb_to_Rgbw(color, &_temp_rgbw, _whiteAlgorithm);
		uint32_t colorBits = 
			((uint32_t)_temp_rgbw.red << 24) +
			((uint32_t)_temp_rgbw.green << 16) +
			((uint32_t)_temp_rgbw.blue << 8) +
			_temp_rgbw.white;

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
