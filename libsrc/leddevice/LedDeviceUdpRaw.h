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
	/// Constructs the LedDevice for sending led colors via udp
	///
	/// @param outputDevice hostname:port
	/// @param latchTime 
	///
	LedDeviceUdpRaw(const Json::Value &deviceConfig);

	bool setConfig(const Json::Value &deviceConfig);

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
