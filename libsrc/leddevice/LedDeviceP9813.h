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
	/// Constructs the LedDevice for a string containing leds of the type P9813
	///
	/// @param outputDevice The name of the output device (eg '/etc/SpiDev.0.0')
	/// @param baudrate The used baudrate for writing to the output device
	///
	LedDeviceP9813(const std::string& outputDevice,
					const unsigned baudrate);

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

	/// the number of leds
	size_t _ledCount;

	/// Buffer for writing/written led data
	std::vector<uint8_t> _ledBuf;

	///
	/// Calculates the required checksum for one led
	///
	/// @param color The color of the led
	/// @return The checksum for the led
	///
	uint8_t calculateChecksum(const ColorRgb & color) const;
};
