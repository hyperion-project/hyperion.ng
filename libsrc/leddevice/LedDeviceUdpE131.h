#pragma once

// STL includes
#include <string>

// hyperion incluse
#include "LedUdpDevice.h"

///
/// Implementation of the LedDevice interface for sending led colors via udp.
///
class LedDeviceUdpE131 : public LedUdpDevice
{
public:
	///
	/// Constructs the LedDevice for sending led colors via udp
	///
	/// @param outputDevice hostname:port
	/// @param latchTime 
	///

	LedDeviceUdpE131(const std::string& outputDevice, const unsigned latchTime);

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
