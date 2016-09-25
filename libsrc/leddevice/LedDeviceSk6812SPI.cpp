#include "LedDeviceSk6812SPI.h"

LedDeviceSk6812SPI::LedDeviceSk6812SPI(const Json::Value &deviceConfig)
	: ProviderSpi(deviceConfig)
	, _whiteAlgorithm(RGBW::INVALID)
	, bitpair_to_byte {
		0b10001000,
		0b10001100,
		0b11001000,
		0b11001100,
	}
{
	_deviceReady = setConfig(deviceConfig);
}

LedDevice* LedDeviceSk6812SPI::construct(const Json::Value &deviceConfig)
{
	return new LedDeviceSk6812SPI(deviceConfig);
}

bool LedDeviceSk6812SPI::setConfig(const Json::Value &deviceConfig)
{
	std::string whiteAlgorithm = deviceConfig.get("white_algorithm","white_off").asString();
	_whiteAlgorithm            = RGBW::stringToWhiteAlgorithm(whiteAlgorithm);

	if (_whiteAlgorithm == RGBW::INVALID)
	{
		Error(_log, "unknown whiteAlgorithm %s", whiteAlgorithm.c_str());
		return false;
	}
	Debug( _log, "whiteAlgorithm : %s", whiteAlgorithm.c_str());

	if ( !ProviderSpi::setConfig(deviceConfig,3000000) )
	{
		return false;
	}
	
	const int SPI_BYTES_PER_COLOUR      = 4;
	const int SPI_FRAME_END_LATCH_BYTES = 3;
	_ledBuffer.resize(_ledRGBWCount * SPI_BYTES_PER_COLOUR + SPI_FRAME_END_LATCH_BYTES, 0x00);
	
	return true;
}

int LedDeviceSk6812SPI::write(const std::vector<ColorRgb> &ledValues)
{
	unsigned spi_ptr = 0;
	static const int SPI_BYTES_PER_LED = 4;

	for (const ColorRgb& color : ledValues)
	{
		RGBW::Rgb_to_Rgbw(color, &_temp_rgbw, _whiteAlgorithm);
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

	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
