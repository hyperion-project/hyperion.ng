#include "LedDeviceSK9822.h"

// Local Hyperion includes
#include <utils/Logger.h>


/// The value that determines the higher bits of the SK9822 global brightness control field
const int SK9822_GBC_UPPER_BITS = 0xE0;

/// The maximal current level supported by the SK9822 global brightness control field, 31
const int SK9822_GBC_MAX_LEVEL = 0x1F;

LedDeviceSK9822::LedDeviceSK9822(const QJsonObject &deviceConfig)
	: ProviderSpi(deviceConfig)
	, _globalBrightnessControlThreshold(255)
	, _globalBrightnessControlMaxLevel(SK9822_GBC_MAX_LEVEL)
{
}

LedDevice *LedDeviceSK9822::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceSK9822(deviceConfig);
}

bool LedDeviceSK9822::init(const QJsonObject &deviceConfig)
{
	// Initialise sub-class
	if ( !ProviderSpi::init(deviceConfig) )
	{
		return false;
	}
	
	_globalBrightnessControlThreshold = deviceConfig["globalBrightnessControlThreshold"].toInt(255);
	_globalBrightnessControlMaxLevel = deviceConfig["globalBrightnessControlMaxLevel"].toInt(SK9822_GBC_MAX_LEVEL);
	Info(_log, "[SK9822] Using global brightness control with threshold of %d and max level of %d", _globalBrightnessControlThreshold, _globalBrightnessControlMaxLevel);

	const unsigned int startFrameSize = 4;
	const unsigned int endFrameSize = ((_ledCount/32) + 1)*4;
	const unsigned int bufferSize = (_ledCount * 4) + startFrameSize + endFrameSize;

	_ledBuffer.resize(bufferSize);
	_ledBuffer.fill(0x00);

	return true;
}

void LedDeviceSK9822::bufferWithMaxCurrent(QVector<uint8_t> &txBuf, const QVector<ColorRgb> & ledValues, const int maxLevel) const
{

	for (int iLed = 0; iLed < static_cast<int>(_ledCount); ++iLed)
	{
		const ColorRgb &rgb = ledValues[iLed];
		const uint8_t red = rgb.red;
		const uint8_t green = rgb.green;
		const uint8_t blue = rgb.blue;

		/// The LED index in the buffer
		const int b = 4 + iLed * 4;

		// Use 0/31 LED-Current for Black, and full LED-Current for all other colors,
		// with PWM control on RGB-Channels
		const int ored = (red|green|blue);

		txBuf[b + 0] = ((ored > 0) * (maxLevel & SK9822_GBC_MAX_LEVEL)) | SK9822_GBC_UPPER_BITS; // (ored > 0) is 1 for any r,g,b > 0, 0 otherwise; branch free
		txBuf[b + 1] = red;
		txBuf[b + 2] = green;
		txBuf[b + 3] = blue;
	}
}

inline __attribute__((always_inline)) unsigned LedDeviceSK9822::scale(const uint8_t value, const int maxLevel, const uint16_t brightness) const 
{
	return ((maxLevel * value + (brightness >> 1)) / brightness);
}

void LedDeviceSK9822::bufferWithAdjustedCurrent(QVector<uint8_t> &txBuf, const QVector<ColorRgb> & ledValues, const int threshold, const int maxLevel) const
{

	for (int iLed = 0; iLed < static_cast<int>(_ledCount); ++iLed)
	{
		const ColorRgb &rgb = ledValues[iLed];
		uint8_t red = rgb.red;
		uint8_t green = rgb.green;
		uint8_t blue = rgb.blue;
		uint8_t level;

		/// The LED index in the buffer
		const int b = 4 + iLed * 4;

		/// The maximal r,g,b-channel grayscale value of the LED
		const uint16_t /* expand to 16 bit! */ maxValue = std::max(std::max(red, green), blue);

		if (maxValue == 0) {
			// Use 0/31 LED-Current for Black
			level = 0;
			red = 0x00;
			green = 0x00;
			blue = 0x00;
		} else if (maxValue >= threshold) {
			// Use full LED-Current when maximal r,g,b-channel grayscale value >= threshold and just use PWM control
			level = (maxLevel & SK9822_GBC_MAX_LEVEL);
		} else {
			// Use adjusted LED-Current for other r,g,b-channel grayscale values
			// See also: https://github.com/FastLED/FastLED/issues/656

			// Scale the r,g,b-channel grayscale values to adjusted current = brightness level
			auto /* 16 bit! */ brightness = static_cast<uint16_t>((((maxValue + 1) * maxLevel - 1) >> 8) + 1);

			level = (brightness & SK9822_GBC_MAX_LEVEL);
			red = static_cast<uint8_t>(scale(red, maxLevel, brightness));
			green = static_cast<uint8_t>(scale(green, maxLevel, brightness));
			blue = static_cast<uint8_t>(scale(blue, maxLevel, brightness));
		}

		txBuf[b + 0] = level | static_cast<uint8_t>(SK9822_GBC_UPPER_BITS);
		txBuf[b + 1] = red;
		txBuf[b + 2] = green;
		txBuf[b + 3] = blue;
	}
}

int LedDeviceSK9822::write(const QVector<ColorRgb> &ledValues)
{
	const int threshold = _globalBrightnessControlThreshold;
	const int maxLevel = _globalBrightnessControlMaxLevel;

	if(threshold > 0) {
		this->bufferWithAdjustedCurrent(_ledBuffer, ledValues, threshold, maxLevel);
	} else {
		this->bufferWithMaxCurrent(_ledBuffer, ledValues, maxLevel);
	}

	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
