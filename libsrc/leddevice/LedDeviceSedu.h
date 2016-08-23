#pragma once

// STL includes
#include <string>

// hyperion incluse
#include "LedRs232Device.h"

///
/// Implementation of the LedDevice interface for writing to SEDU led device.
///
class LedDeviceSedu : public LedRs232Device
{
public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	LedDeviceSedu(const Json::Value &deviceConfig);

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
};
