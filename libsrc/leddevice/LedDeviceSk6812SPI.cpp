
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceSk6812SPI.h"

LedDeviceSk6812SPI::LedDeviceSk6812SPI(const Json::Value &deviceConfig)
	: ProviderSpi(deviceConfig)
	, bitpair_to_byte {
		0b10001000,
		0b10001100,
		0b11001000,
		0b11001100,
	}

{
	setConfig(deviceConfig);
	Debug( _log, "whiteAlgorithm : %s", _whiteAlgorithm.c_str());
}

LedDevice* LedDeviceSk6812SPI::construct(const Json::Value &deviceConfig)
{
	return new LedDeviceSk6812SPI(deviceConfig);
}

bool LedDeviceSk6812SPI::setConfig(const Json::Value &deviceConfig)
{
	ProviderSpi::setConfig(deviceConfig,3000000);

	_whiteAlgorithm = deviceConfig.get("white_algorithm","").asString();

	return true;
}

int LedDeviceSk6812SPI::write(const std::vector<ColorRgb> &ledValues)
{
	// 4 colours, 4 spi bytes per colour + 3 frame end latch bytes
	#define COLOURS_PER_LED       4
	#define SPI_BYTES_PER_COLOUR  4
	#define SPI_BYTES_PER_LED     COLOURS_PER_LED * SPI_BYTES_PER_COLOUR

	unsigned spi_size = _ledCount * SPI_BYTES_PER_LED + 3;
	if(_ledBuffer.size() != spi_size)
	{
		_ledBuffer.resize(spi_size, 0x00);
	}

	unsigned spi_ptr = 0;

	for (const ColorRgb& color : ledValues)
	{
		Rgb_to_Rgbw(color, &_temp_rgbw, _whiteAlgorithm);
		uint32_t colorBits = 
			((uint32_t)_temp_rgbw.red << 24) +
			((uint32_t)_temp_rgbw.green << 16) +
			((uint32_t)_temp_rgbw.blue << 8) +
			_temp_rgbw.white;

		for (int j=SPI_BYTES_PER_LED - 1; j>=0; j--) {
			_ledBuffer[spi_ptr+j] = bitpair_to_byte[ colorBits & 0x3 ];
			colorBits >>= 2;
		}
		spi_ptr += SPI_BYTES_PER_LED;
	}

	_ledBuffer[spi_ptr++] = 0;
	_ledBuffer[spi_ptr++] = 0;
	_ledBuffer[spi_ptr++] = 0;

	return writeBytes(spi_size, _ledBuffer.data());
}
