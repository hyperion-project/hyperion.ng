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
	bool isInitOK = false;

	// Initialise sub-class
	if (ProviderSpi::init(deviceConfig))
	{
		_globalBrightnessControlThreshold = deviceConfig["globalBrightnessControlThreshold"].toInt(255);
		_globalBrightnessControlMaxLevel = deviceConfig["globalBrightnessControlMaxLevel"].toInt(SK9822_GBC_MAX_LEVEL);
		Info(_log, "[SK9822] Using global brightness control with threshold of %d and max level of %d", _globalBrightnessControlThreshold, _globalBrightnessControlMaxLevel);

		const unsigned int startFrameSize = 4;
		const unsigned int endFrameSize = ((_ledCount/32) + 1)*4;
		const unsigned int bufferSize = (_ledCount * 4) + startFrameSize + endFrameSize;

		_ledBuffer.resize(0, 0x00);
		_ledBuffer.resize(bufferSize, 0x00);

		isInitOK = true;
	}
	return isInitOK;
}


void LedDeviceSK9822::bufferWithMaxCurrent(std::vector<uint8_t> &txBuf, const std::vector<ColorRgb> & ledValues, const int maxLevel) {
	const int ledCount = static_cast<int>(_ledCount);

	for (int iLed = 0; iLed < ledCount; ++iLed)
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

inline __attribute__((always_inline)) unsigned LedDeviceSK9822::scale(const uint8_t value, const int maxLevel, const uint16_t brightness) {
	return (((maxLevel * value + (brightness >> 1)) / brightness));
}

void LedDeviceSK9822::bufferWithAdjustedCurrent(std::vector<uint8_t> &txBuf, const std::vector<ColorRgb> & ledValues, const int threshold, const int maxLevel) {
	const int ledCount = static_cast<int>(_ledCount);

	for (int iLed = 0; iLed < ledCount; ++iLed)
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
			const uint16_t /* 16 bit! */ brightness = (((maxValue + 1) * maxLevel - 1) >> 8) + 1;

			level = (brightness & SK9822_GBC_MAX_LEVEL);
			red = scale(red, maxLevel, brightness);
			green = scale(green, maxLevel, brightness);
			blue = scale(blue, maxLevel, brightness);
		}

		txBuf[b + 0] = level | SK9822_GBC_UPPER_BITS;
		txBuf[b + 1] = red;
		txBuf[b + 2] = green;
		txBuf[b + 3] = blue;
	}
}

int LedDeviceSK9822::write(const std::vector<ColorRgb> &ledValues)
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
