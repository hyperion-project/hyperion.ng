#include "LedDeviceAPA102_ftdi.h"

#define LED_HEADER 0b11100000
#define LED_BRIGHTNESS_FULL 31

LedDeviceAPA102_ftdi::LedDeviceAPA102_ftdi(const QJsonObject &deviceConfig) : ProviderFtdi(deviceConfig)
{
}

LedDevice *LedDeviceAPA102_ftdi::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceAPA102_ftdi(deviceConfig);
}

bool LedDeviceAPA102_ftdi::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = false;
	// Initialise sub-class
	if (ProviderFtdi::init(deviceConfig))
	{
        _brightnessControlMaxLevel = deviceConfig["brightnessControlMaxLevel"].toInt(LED_BRIGHTNESS_FULL);
        Info(_log, "[%s] Setting maximum brightness to [%d] = %d%%", QSTRING_CSTR(_activeDeviceType), _brightnessControlMaxLevel, _brightnessControlMaxLevel * 100 / LED_BRIGHTNESS_FULL);

        CreateHeader();
		isInitOK = true;
	}
	return isInitOK;
}

void LedDeviceAPA102_ftdi::CreateHeader()
{
	const unsigned int startFrameSize = 4;
	// Endframe, add additional 4 bytes to cover SK9922 Reset frame (in case SK9922 were sold as AP102) -  has no effect on APA102
	const unsigned int endFrameSize = (_ledCount / 32) * 4 + 4;
	const unsigned int APAbufferSize = (_ledCount * 4) + startFrameSize + endFrameSize;
	_ledBuffer.resize(APAbufferSize, 0);
	Debug(_log, "APA102 buffer created. Led's number: %d", _ledCount);
}

int LedDeviceAPA102_ftdi::write(const std::vector<ColorRgb> &ledValues)
{
	for (signed iLed = 0; iLed < static_cast<int>(_ledCount); ++iLed)
	{
		const ColorRgb &rgb = ledValues[iLed];
		_ledBuffer[4 + iLed * 4 + 0] = LED_HEADER | _brightnessControlMaxLevel;
		_ledBuffer[4 + iLed * 4 + 1] = rgb.red;
		_ledBuffer[4 + iLed * 4 + 2] = rgb.green;
		_ledBuffer[4 + iLed * 4 + 3] = rgb.blue;
	}

	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
