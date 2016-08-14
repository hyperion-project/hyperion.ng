#pragma once

// STL includes
#include <string>

// hyperion incluse
#include "LedSpiDevice.h"

///
/// Implementation of the LedDevice interface for writing to APA102 led device.
///
/// APA102 is
///
class LedDeviceAPA102 : public LedSpiDevice
{
public:
	///
	/// Constructs the LedDevice for a string containing leds of the type APA102
	///
	/// @param outputDevice The name of the output device (eg '/dev/spidev.0.0')
	/// @param baudrate The used baudrate for writing to the output device
	///
	LedDeviceAPA102(const std::string& outputDevice, const unsigned baudrate );


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
