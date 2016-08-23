#pragma once

// STL includes
#include <string>

// hyperion include
#include "LedSpiDevice.h"

///
/// Implementation of the LedDevice interface for writing to P9813 led device.
///
class LedDeviceP9813 : public LedSpiDevice
{
public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	LedDeviceP9813(const Json::Value &deviceConfig);

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
	///
	/// Calculates the required checksum for one led
	///
	/// @param color The color of the led
	/// @return The checksum for the led
	///
	uint8_t calculateChecksum(const ColorRgb & color) const;
};
