#include "LedDeviceAPA102.h"

// Constants
namespace {

/// The value that determines the higher bits of the APA102 brightness control field
const int APA102_LEDFRAME_UPPER_BITS = 0xE0;

} //End of constants


LedDeviceAPA102::LedDeviceAPA102(const QJsonObject &deviceConfig)
	: ProviderSpi(deviceConfig)
{
	// Overwrite non supported/required features
	_latchTime_ms = 0;
}

LedDevice* LedDeviceAPA102::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceAPA102(deviceConfig);
}

bool LedDeviceAPA102::init(const QJsonObject &deviceConfig)
{
	// Initialise sub-class
	if ( !ProviderSpi::init(deviceConfig) )
	{
		return false;
	}

	_brightnessControlMaxLevel = deviceConfig["brightnessControlMaxLevel"].toInt(APA102_BRIGHTNESS_MAX_LEVEL);
	Info(_log, "[%s] Setting maximum brightness to [%d] = %d%%", QSTRING_CSTR(_activeDeviceType), _brightnessControlMaxLevel, _brightnessControlMaxLevel * 100 / APA102_BRIGHTNESS_MAX_LEVEL);

	const unsigned int startFrameSize = 4;
	//Endframe, add additional 4 bytes to cover SK9922 Reset frame (in case SK9922 were sold as AP102) -  has no effect on APA102
	const unsigned int endFrameSize = (_ledCount/32) * 4 + 4;
	const unsigned int APAbufferSize = (_ledCount * 4) + startFrameSize + endFrameSize;

	_ledBuffer.fill(0x00, APAbufferSize);

	return true;
}

void LedDeviceAPA102::bufferWithBrightness(QVector<uint8_t> &txBuf, const QVector<ColorRgb> & ledValues, const int brightness) const
{

	for (int iLed = 0; iLed < static_cast<int>(_ledCount); ++iLed)
	{
		const ColorRgb &rgb = ledValues[iLed];
		const uint8_t red = rgb.red;
		const uint8_t green = rgb.green;
		const uint8_t blue = rgb.blue;

		/// The LED index in the buffer
		const int b = 4 + iLed * 4;

		txBuf[b + 0] = static_cast<uint8_t>(brightness | APA102_LEDFRAME_UPPER_BITS);
		txBuf[b + 1] = blue;
		txBuf[b + 2] = green;
		txBuf[b + 3] = red;
	}
}

int LedDeviceAPA102::write(const QVector<ColorRgb> &ledValues)
{
	this->bufferWithBrightness(_ledBuffer, ledValues, _brightnessControlMaxLevel);

	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
