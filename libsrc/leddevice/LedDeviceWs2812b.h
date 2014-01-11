
#pragma once

// Hyperion leddevice includes
#include "LedRs232Device.h"

///
/// The LedDevice for controlling a string of WS2812B leds. These are controlled over the mini-UART
/// of the RPi (/dev/ttyAMA0).
///
class LedDeviceWs2812b : public LedRs232Device
{
public:
	///
	/// Constructs the device (all required parameters are hardcoded)
	///
	LedDeviceWs2812b();

	///
	/// Write the color data the the WS2812B led string
	///
	/// @param ledValues The color data
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb> & ledValues);

	///
	/// Write zero to all leds(that have been written by a previous write operation)
	///
	/// @return Zero on succes else negative
	///
	virtual int switchOff();

private:

	///
	/// Translate a color to the signal bits. The resulting bits are written to the given memory.
	///
	/// @param color The color to translate
	/// @param signal The pointer at the beginning of the signal to write
	/// @return The pointer at the end of the written signal
	///
	uint8_t * color2signal(const ColorRgb & color, uint8_t * signal);

	///
	/// Translates three bits to a single byte
	///
	/// @param bit1 The value of the first bit (1=true, zero=false)
	/// @param bit2 The value of the second bit (1=true, zero=false)
	/// @param bit3 The value of the third bit (1=true, zero=false)
	///
	/// @return The output-byte for the given two bit
	///
	uint8_t bits2Signal(const bool bit1, const bool bit2, const bool bit3) const;

	///
	/// The output buffer for writing bytes to the output
	///
	std::vector<uint8_t> _ledBuffer;
};
