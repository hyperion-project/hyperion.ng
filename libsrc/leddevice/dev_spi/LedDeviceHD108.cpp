#include "LedDeviceHD108.h"

// Constructor
LedDeviceHD108::LedDeviceHD108(const QJsonObject &deviceConfig)
    : ProviderSpi(deviceConfig)
{
    // Overwrite non supported/required features
	_latchTime_ms = 0;
	// Initialize _global_brightness
	_global_brightness = 0xFFFF;
}

LedDevice* LedDeviceHD108::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceHD108(deviceConfig);
}

// Initialization method
bool LedDeviceHD108::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = false;

	if ( ProviderSpi::init(deviceConfig) )
	{
		_brightnessControlMaxLevel = deviceConfig["brightnessControlMaxLevel"].toInt(HD108_BRIGHTNESS_MAX_LEVEL);
		Info(_log, "[%s] Setting maximum brightness to [%d] = %d%%", QSTRING_CSTR(_activeDeviceType), _brightnessControlMaxLevel, _brightnessControlMaxLevel * 100 / HD108_BRIGHTNESS_MAX_LEVEL);

		// Set the global brightness or control byte based on the provided formula
    	_global_brightness = (1 << 15) | (_brightnessControlMaxLevel << 10) | (_brightnessControlMaxLevel << 5) | _brightnessControlMaxLevel;

		isInitOK = true;
	}

	return isInitOK;
}

// Write method to update the LED colors
int LedDeviceHD108::write(const std::vector<ColorRgb> & ledValues)
{
    std::vector<uint8_t> hd108Data;

    // Start frame - 64 bits of 0 (8 bytes of 0)
    hd108Data.insert(hd108Data.end(), 8, 0x00);

    // Adapted logic from your HD108 library's "show" and "setPixelColor8Bit" methods
    for (const ColorRgb &color : ledValues)
    {
        // Convert 8-bit to 16-bit colors
        uint16_t red16 = (color.red << 8) | color.red;
        uint16_t green16 = (color.green << 8) | color.green;
        uint16_t blue16 = (color.blue << 8) | color.blue;

        // Push global and color components into hd108Data
		// Brightness
        hd108Data.push_back(_global_brightness >> 8);
		hd108Data.push_back(_global_brightness & 0xFF);
		// Color - Red
        hd108Data.push_back(red16 >> 8);
		hd108Data.push_back(red16 & 0xFF);
		// Color - Green
        hd108Data.push_back(green16 >> 8);
		hd108Data.push_back(green16 & 0xFF);
		// Color - Blue
        hd108Data.push_back(blue16 >> 8);
		hd108Data.push_back(blue16 & 0xFF);
    }

    // End frame - write "1"s equal to at least how many pixels are in the string
    hd108Data.insert(hd108Data.end(), ledValues.size() / 16 + 1, 0xFF);

    // Use ProviderSpi's writeBytes method to send the data
    return writeBytes(hd108Data.size(), hd108Data.data());
}
