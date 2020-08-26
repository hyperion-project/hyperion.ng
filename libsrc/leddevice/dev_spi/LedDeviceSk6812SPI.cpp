#include "LedDeviceSk6812SPI.h"

LedDeviceSk6812SPI::LedDeviceSk6812SPI(const QJsonObject &deviceConfig)
	: ProviderSpi(deviceConfig)
	  , _whiteAlgorithm(RGBW::WhiteAlgorithm::INVALID)
	  , SPI_BYTES_PER_COLOUR(4)
	  , bitpair_to_byte {
		  0b10001000,
		  0b10001100,
		  0b11001000,
		  0b11001100,
		  }
{
}

LedDevice* LedDeviceSk6812SPI::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceSk6812SPI(deviceConfig);
}

bool LedDeviceSk6812SPI::init(const QJsonObject &deviceConfig)
{
	_baudRate_Hz = 3000000;

	bool isInitOK = false;

	// Initialise sub-class
	if ( ProviderSpi::init(deviceConfig) )
	{
		QString whiteAlgorithm = deviceConfig["whiteAlgorithm"].toString("white_off");

		_whiteAlgorithm	= RGBW::stringToWhiteAlgorithm(whiteAlgorithm);
		if (_whiteAlgorithm == RGBW::WhiteAlgorithm::INVALID)
		{
			QString errortext = QString ("unknown whiteAlgorithm: %1").arg(whiteAlgorithm);
			this->setInError(errortext);
			isInitOK = false;
		}
		else
		{
			Debug( _log, "whiteAlgorithm : %s", QSTRING_CSTR(whiteAlgorithm));

			WarningIf(( _baudRate_Hz < 2050000 || _baudRate_Hz > 4000000 ), _log, "SPI rate %d outside recommended range (2050000 -> 4000000)", _baudRate_Hz);

			const int SPI_FRAME_END_LATCH_BYTES = 3;
			_ledBuffer.resize(_ledRGBWCount * SPI_BYTES_PER_COLOUR + SPI_FRAME_END_LATCH_BYTES, 0x00);

			isInitOK = true;
		}
	}
	return isInitOK;
}

int LedDeviceSk6812SPI::write(const std::vector<ColorRgb> &ledValues)
{
	unsigned spi_ptr = 0;
	const int SPI_BYTES_PER_LED = sizeof(ColorRgbw) * SPI_BYTES_PER_COLOUR;


	for (const ColorRgb& color : ledValues)
	{
		RGBW::Rgb_to_Rgbw(color, &_temp_rgbw, _whiteAlgorithm);
		uint32_t colorBits = 
			((uint32_t)_temp_rgbw.red << 24) +
			((uint32_t)_temp_rgbw.green << 16) +
			((uint32_t)_temp_rgbw.blue << 8) +
			_temp_rgbw.white;

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

	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
