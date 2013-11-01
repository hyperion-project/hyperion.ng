#pragma once

// Local hyperion incluse
#include "LedSpiDevice.h"

///
/// Implementation of the LedDevice interface for writing to LDP6803 led device.
///
class LedDeviceLDP6803 : public LedSpiDevice
{
public:
	///
	/// Constructs the LedDevice for a string containing leds of the type LDP6803
	///
	/// @param[in] outputDevice The name of the output device (eg '/etc/SpiDev.0.0')
	/// @param[in] baudrate The used baudrate for writing to the output device
	///
	LedDeviceLDP6803(const std::string& outputDevice, const unsigned baudrate);

	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<RgbColor> &ledValues);

	/// Switch the leds off
	virtual int switchOff();

private:
	/// The 'latch' time for latching the shifted-value into the leds
	timespec latchTime;

	/// the number of leds (needed when switching off)
	size_t mLedCount;
};
