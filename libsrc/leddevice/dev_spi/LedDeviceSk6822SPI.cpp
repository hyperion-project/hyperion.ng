#include "LedDeviceSk6822SPI.h"
/*
From the data sheet:

(TH+TL=1.7μs±600ns)

T0H,	 0 code, high level time,	 0.35µs ±0.150ns
T0L,	 0 code, low level time,	 1.36µs ±0.150ns
T1H,	 1 code, high level time,	 1.36µs ±0.150ns
T1L,	 1 code, low level time,	 0.35µs ±0.150ns
WT,	 Wait for the processing time,	 12µs ±0.150ns
Trst,	 Reset code,low level time,	 50µs

To normalise the pulse times so they fit in 4 SPI bits:

Use timings at upper end of tolerance:
1.36 -> 1.50 uS
0.35 -> 0.50 uS

A SPI bit time of 0.50uS = 2Mbit/sec
T0 is sent as 1000
T1 is sent as 1110

With a bit of excel testing, we can work out the maximum and minimum speeds:
2000000 MIN
2230000 AVG
2460000 MAX

Wait time:
using the min of  2000000, the bit time is 0.500
Wait time is 12uS = 24 bits = 3 bytes

Reset time:
using the min of  2000000, the bit time is 0.500
Reset time is 50uS = 100 bits = 13 bytes

*/

LedDeviceSk6822SPI::LedDeviceSk6822SPI(const QJsonObject &deviceConfig)
	: ProviderSpi(deviceConfig)
	  , SPI_BYTES_PER_COLOUR(4)
	  , SPI_BYTES_WAIT_TIME(3)
	  , SPI_FRAME_END_LATCH_BYTES(13)
	  , bitpair_to_byte {
		  0b10001000,
		  0b10001110,
		  0b11101000,
		  0b11101110,
		  }
{
}

LedDevice* LedDeviceSk6822SPI::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceSk6822SPI(deviceConfig);
}

bool LedDeviceSk6822SPI::init(const QJsonObject &deviceConfig)
{
	_baudRate_Hz = 2230000;

	bool isInitOK = false;

	// Initialise sub-class
	if ( ProviderSpi::init(deviceConfig) )
	{
		WarningIf(( _baudRate_Hz < 2000000 || _baudRate_Hz > 2460000 ), _log, "SPI rate %d outside recommended range (2000000 -> 2460000)", _baudRate_Hz);

		_ledBuffer.resize( (_ledRGBCount *  SPI_BYTES_PER_COLOUR) + (_ledCount * SPI_BYTES_WAIT_TIME ) + SPI_FRAME_END_LATCH_BYTES, 0x00);
		isInitOK = true;
	}

	return isInitOK;
}

int LedDeviceSk6822SPI::write(const std::vector<ColorRgb> &ledValues)
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
		spi_ptr += SPI_BYTES_WAIT_TIME;	// the wait between led time is all zeros
	}

#if 0
	// debug the whole SPI packet
	char debug_line[2048];
	int ptr=0;
	for (unsigned int i=0; i < _ledBuffer.size(); i++)
	{
		if (i%16 == 0)
		{
			ptr += snprintf (ptr+debug_line, sizeof(debug_line)-ptr, "%03x: ", i);
		}

		ptr += snprintf (ptr+debug_line, sizeof(debug_line)-ptr, "%02x ", _ledBuffer.data()[i]);

		if ( (i%16 == 15) || ( i == _ledBuffer.size()-1 ) )
		{
			Debug(_log, debug_line);
			ptr = 0;
		}
	}
#endif

	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
