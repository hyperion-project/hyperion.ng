#include "LedDeviceSk6812_ftdi.h"

LedDeviceSk6812_ftdi::LedDeviceSk6812_ftdi(const QJsonObject &deviceConfig)
	: ProviderFtdi(deviceConfig),
	  SPI_BYTES_PER_COLOUR(4),
	  bitpair_to_byte{
		  0b10001000,
		  0b10001100,
		  0b11001000,
		  0b11001100}
{
}

LedDevice *LedDeviceSk6812_ftdi::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceSk6812_ftdi(deviceConfig);
}

bool LedDeviceSk6812_ftdi::init(const QJsonObject &deviceConfig)
{

	bool isInitOK = false;

	// Initialise sub-class
	if (ProviderFtdi::init(deviceConfig))
	{
		_brightnessControlMaxLevel = deviceConfig["brightnessControlMaxLevel"].toInt(255);

		QString whiteAlgorithm = deviceConfig["whiteAlgorithm"].toString("white_off");

		_whiteAlgorithm = RGBW::stringToWhiteAlgorithm(whiteAlgorithm);

		Debug(_log, "whiteAlgorithm : %s", QSTRING_CSTR(whiteAlgorithm));

		WarningIf((_baudRate_Hz < 2050000 || _baudRate_Hz > 4000000), _log, "SPI rate %d outside recommended range (2050000 -> 4000000)", _baudRate_Hz);

		const int SPI_FRAME_END_LATCH_BYTES = 3;
		_ledBuffer.resize(_ledRGBWCount * SPI_BYTES_PER_COLOUR + SPI_FRAME_END_LATCH_BYTES, 0x00);

		isInitOK = true;
	}
	return isInitOK;
}


inline __attribute__((always_inline)) uint8_t LedDeviceSk6812_ftdi::scale(uint8_t i, uint8_t scale) {
	return (((uint16_t)i) * (1+(uint16_t)(scale))) >> 8;
}

int LedDeviceSk6812_ftdi::write(const std::vector<ColorRgb> &ledValues)
{
	unsigned spi_ptr = 0;
	const int SPI_BYTES_PER_LED = sizeof(ColorRgbw) * SPI_BYTES_PER_COLOUR;

	if (_ledCount != ledValues.size())
	{
		Warning(_log, "Sk6812SPI led's number has changed (old: %d, new: %d). Rebuilding buffer.", _ledCount, ledValues.size());
		_ledCount = ledValues.size();

		const int SPI_FRAME_END_LATCH_BYTES = 3;
		_ledBuffer.resize(0, 0x00);
		_ledBuffer.resize(_ledRGBWCount * SPI_BYTES_PER_COLOUR + SPI_FRAME_END_LATCH_BYTES, 0x00);
	}
	ColorRgbw temp_rgbw;
	ColorRgb scaled_color;
	for (const ColorRgb &color : ledValues)
	{
		scaled_color.red = scale(color.red, _brightnessControlMaxLevel);
		scaled_color.green = scale(color.green, _brightnessControlMaxLevel);
		scaled_color.blue = scale(color.blue, _brightnessControlMaxLevel);

        RGBW::Rgb_to_Rgbw(scaled_color, &temp_rgbw, _whiteAlgorithm);

		uint32_t colorBits =
			((uint32_t)temp_rgbw.red << 24) +
			((uint32_t)temp_rgbw.green << 16) +
			((uint32_t)temp_rgbw.blue << 8) +
			temp_rgbw.white;

		for (int j = SPI_BYTES_PER_LED - 1; j >= 0; j--)
		{
			_ledBuffer[spi_ptr + j] = bitpair_to_byte[colorBits & 0x3];
			colorBits >>= 2;
		}
		spi_ptr += SPI_BYTES_PER_LED;
	}

	_ledBuffer[spi_ptr++] = 0;
	_ledBuffer[spi_ptr++] = 0;
	_ledBuffer[spi_ptr++] = 0;

	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
