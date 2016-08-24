#pragma once

#include <leddevice/LedDevice.h>
#include <ws2811.h>

class LedDeviceWS281x : public LedDevice
{
public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	LedDeviceWS281x(const Json::Value &deviceConfig);

	///
	/// Destructor of the LedDevice, waits for DMA to complete and then cleans up
	///
	~LedDeviceWS281x();
	
	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	bool setConfig(const Json::Value &deviceConfig);

	/// constructs leddevice
	static LedDevice* construct(const Json::Value &deviceConfig);

	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb> &ledValues);

	/// Switch the leds off
	virtual int switchOff();

private:
	ws2811_t    _led_string;
	int         _channel;
	bool        _initialized;
	std::string _whiteAlgorithm;
	ColorRgbw   _temp_rgbw;
};
