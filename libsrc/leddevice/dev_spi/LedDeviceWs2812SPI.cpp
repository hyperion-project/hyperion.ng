#include "LedDeviceWs2812SPI.h"

	/*
From the data sheet:

(TH+TL=1.25μs±600ns)

T0H,	 0 code, high level time,	 0.40µs ±0.150ns
T0L,	 0 code, low level time,	 0.85µs ±0.150ns
T1H,	 1 code, high level time,	 0.80µs ±0.150ns
T1L,	 1 code, low level time,	 0.45µs ±0.150ns
WT,	 Wait for the processing time,	 NA
Trst,	 Reset code,low level time,	 50µs (not anymore... need 300uS for latest revision)

To normalise the pulse times so they fit in 4 SPI bits:

On the assumption that the "low" time doesnt matter much

A SPI bit time of 0.40uS = 2.5 Mbit/sec
T0 is sent as 1000
T1 is sent as 1100

With a bit of excel testing, we can work out the maximum and minimum speeds:
2106000 MIN
2590500 AVG
3075000 MAX

Wait time:
Not Applicable for WS2812

Reset time:
using the max of 3075000, the bit time is 0.325
Reset time is 300uS = 923 bits = 116 bytes

*/

LedDeviceWs2812SPI::LedDeviceWs2812SPI(const QJsonObject &deviceConfig)
	: ProviderSpi(deviceConfig)
	  , SPI_BYTES_PER_COLOUR(4)
	  , SPI_FRAME_END_LATCH_BYTES(116)
	  , bitpair_to_byte {
		  0b10001000,
		  0b10001100,
		  0b11001000,
		  0b11001100,
		  }
{
}

LedDevice* LedDeviceWs2812SPI::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceWs2812SPI(deviceConfig);
}

bool LedDeviceWs2812SPI::init(const QJsonObject &deviceConfig)
{
	_baudRate_Hz = 2600000;

	bool isInitOK = false;

	// Initialise sub-class
	if ( ProviderSpi::init(deviceConfig) )
	{
		WarningIf(( _baudRate_Hz < 2106000 || _baudRate_Hz > 3075000 ), _log, "SPI rate %d outside recommended range (2106000 -> 3075000)", _baudRate_Hz);

		_ledBuffer.resize(_ledRGBCount * SPI_BYTES_PER_COLOUR + SPI_FRAME_END_LATCH_BYTES, 0x00);

		isInitOK = true;
	}

	return isInitOK;
}

int LedDeviceWs2812SPI::write(const std::vector<ColorRgb> &ledValues)
{
	unsigned spi_ptr = 0;
	const int SPI_BYTES_PER_LED = sizeof(ColorRgb) * SPI_BYTES_PER_COLOUR;

	for (const ColorRgb& color : ledValues)
	{
		uint32_t colorBits = ((unsigned int)color.red << 16)
							 | ((unsigned int)color.green << 8)
							 | color.blue;

		for (int j=SPI_BYTES_PER_LED - 1; j>=0; j--)
		{
			_ledBuffer[spi_ptr+j] = bitpair_to_byte[ colorBits & 0x3 ];
			colorBits >>= 2;
		}
		spi_ptr += SPI_BYTES_PER_LED;
	}

	for (int j=0; j < SPI_FRAME_END_LATCH_BYTES; j++)
	{
		_ledBuffer[spi_ptr++] = 0;
	}

	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
