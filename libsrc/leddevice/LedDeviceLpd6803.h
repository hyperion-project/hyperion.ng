#pragma once

// Local hyperion incluse
#include "LedSpiDevice.h"

///
/// Implementation of the LedDevice interface for writing to LDP6803 led device.
///
/// 00000000 00000000 00000000 00000000 1RRRRRGG GGGBBBBB 1RRRRRGG GGGBBBBB ...
/// |---------------------------------| |---------------| |---------------|
/// 32 zeros to start the frame Led1 Led2 ...
///
/// For each led, the first bit is always 1, and then you have 5 bits each for red, green and blue
/// (R, G and B in the above illustration) making 16 bits per led. Total bytes = 4 + (2 x number of
/// leds)
///
class LedDeviceLpd6803 : public LedSpiDevice
{
public:
	///
	/// Constructs the LedDevice for a string containing leds of the type LDP6803
	///
	/// @param[in] outputDevice The name of the output device (eg '/dev/spidev0.0')
	/// @param[in] baudrate The used baudrate for writing to the output device
	///
	LedDeviceLpd6803(const std::string& outputDevice, const unsigned baudrate);

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
	/// The buffer containing the packed RGB values
	std::vector<uint8_t> _ledBuffer;
};
