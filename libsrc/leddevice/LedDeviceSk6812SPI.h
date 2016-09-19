#pragma once

// STL includes
#include <string>

// hyperion incluse
#include "ProviderSpi.h"

///
/// Implementation of the LedDevice interface for writing to Sk6801 led device.
///
class LedDeviceSk6812SPI : public ProviderSpi
{
public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	LedDeviceSk6812SPI(const Json::Value &deviceConfig);

	/// constructs leddevice
	static LedDevice* construct(const Json::Value &deviceConfig);

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	bool setConfig(const Json::Value &deviceConfig);
	
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
	std::string _whiteAlgorithm;
	
	uint8_t bitpair_to_byte[4];
	
	ColorRgbw _temp_rgbw;
};
