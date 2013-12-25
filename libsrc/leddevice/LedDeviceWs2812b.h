
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
	/// Structure holding the four output-bytes corresponding to a single input byte
	///
	struct ByteSignal
	{
		uint8_t bit_12;
		uint8_t bit_34;
		uint8_t bit_56;
		uint8_t bit_78;
	};
	/// Translation table from single input-byte to output-bytes
	std::vector<ByteSignal> _byte2signalTable;

	///
	/// Fills the translation table (_byte2signalTable)
	///
	void fillTable();

	///
	/// Computes the output bytes that belong to a given input-byte (no table lookup)
	///
	/// @param byte The input byte
	/// @return The four bytes (ByteSignal) for the output signal
	///
	ByteSignal byte2Signal(const uint8_t byte) const;

	///
	/// Translates two bits to a single byte
	///
	/// @param bit1 The value of the first bit (1=true, zero=false)
	/// @param bit1 The value of the ssecond bit (1=true, zero=false)
	///
	/// @return The output-byte for the given two bit
	///
	uint8_t bits2Signal(const bool bit1, const bool bit2) const;

	///
	/// The output buffer for writing bytes to the output
	///
	std::vector<ByteSignal> _ledBuffer;
};
