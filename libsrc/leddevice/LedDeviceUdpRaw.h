#pragma once

// STL includes
#include <string>

// hyperion incluse
#include "LedUdpDevice.h"

///
/// Implementation of the LedDevice interface for sending led colors via udp.
///
class LedDeviceUdpRaw : public LedUdpDevice
{
public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	LedDeviceUdpRaw(const Json::Value &deviceConfig);

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
};
