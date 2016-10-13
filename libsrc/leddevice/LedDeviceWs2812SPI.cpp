#include "LedDeviceWs2812SPI.h"

LedDeviceWs2812SPI::LedDeviceWs2812SPI(const QJsonObject &deviceConfig)
	: ProviderSpi()
	, bitpair_to_byte {
		0b10001000,
		0b10001100,
		0b11001000,
		0b11001100,
	}
{
	_deviceReady = init(deviceConfig);
}

LedDevice* LedDeviceWs2812SPI::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceWs2812SPI(deviceConfig);
}

bool LedDeviceWs2812SPI::init(const QJsonObject &deviceConfig)
{
	_baudRate_Hz = 3000000;
	ProviderSpi::init(deviceConfig);
	WarningIf(( _baudRate_Hz < 2050000 || _baudRate_Hz > 4000000 ), _log, "SPI rate %d outside recommended range (2050000 -> 4000000)", _baudRate_Hz);

	return true;
}

int LedDeviceWs2812SPI::write(const std::vector<ColorRgb> &ledValues)
{
	// 3 colours, 4 spi bytes per colour + 3 frame end latch bytes
	const int SPI_BYTES_PER_LED  = 3 * 4;
	unsigned spi_size = _ledCount * SPI_BYTES_PER_LED + 3;

	if(_ledBuffer.size() != spi_size)
	{
		_ledBuffer.resize(spi_size, 0x00);
	}

	unsigned spi_ptr = 0;
	for (unsigned i=0; i<(unsigned)_ledCount; ++i)
	{
		uint32_t colorBits = ((unsigned int)ledValues[i].red << 16) 
			| ((unsigned int)ledValues[i].green << 8) 
			| ledValues[i].blue;

		for (int j=SPI_BYTES_PER_LED - 1; j>=0; j--)
		{
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
