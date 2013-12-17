#pragma once

// STL includes
#include <cassert>

// Local hyperion includes
#include "LedRs232Device.h"

namespace ws2811
{
	///
	/// Enumaration of known signal timings
	///
	enum SignalTiming
	{
		option_3755,
		option_3773,
		option_2855,
		option_2882,
		not_a_signaltiming
	};

	///
	/// Enumaration of the possible speeds on which the ws2811 can operate.
	///
	enum SpeedMode
	{
		lowspeed,
		highspeed
	};

	///
	/// Enumeration of the signal 'parts' (T 0 high, T 1 high, T 0 low, T 1 low).
	///
	enum TimeOption
	{
		T0H,
		T1H,
		T0L,
		T1L
	};

	///
	/// Structure holding the signal for a signle byte
	///
	struct ByteSignal
	{
		uint8_t bit_1;
		uint8_t bit_2;
		uint8_t bit_3;
		uint8_t bit_4;
		uint8_t bit_5;
		uint8_t bit_6;
		uint8_t bit_7;
		uint8_t bit_8;
	};
	// Make sure the structure is exatly the length we require
	static_assert(sizeof(ByteSignal) == 8, "Incorrect sizeof ByteSignal (expected 8)");

	///
	/// Translates a string to a signal timing
	///
	/// @param signalTiming The string specifying the signal timing
	/// @param defaultValue The default value (used if the string does not match any known timing)
	///
	/// @return The SignalTiming (or not_a_signaltiming if it did not match)
	///
	SignalTiming fromString(const std::string& signalTiming, const SignalTiming defaultValue);

	///
	/// Returns the required baudrate for a specific signal-timing
	///
	/// @param SpeedMode The WS2811/WS2812 speed mode (WS2812b only has highspeed)
	///
	/// @return The required baudrate for the signal timing
	///
	unsigned getBaudrate(const SpeedMode speedMode);

	///
	/// The number of 'signal units' (bits) For the subpart of a specific timing scheme
	///
	/// @param timing The controller option
	/// @param option The signal part
	///
	unsigned getLength(const SignalTiming timing, const TimeOption option);

	///
	/// Constructs a 'bit' based signal with defined 'high' length (and implicite defined 'low'
	/// length. The signal is based on a 10bits bytes (incl. high startbit and low stopbit). The
	/// total length of the high is given as parameter:<br>
	/// lenHigh=7 => |-------|___| => 1 1111 1100 0 => 252 (start and stop bit are implicite)
	///
	/// @param lenHigh The total length of the 'high' length (incl start-bit)
	/// @return The byte representing the high-low signal
	///
	uint8_t bitToSignal(unsigned lenHigh);

	///
	/// Translate a byte into signal levels for a specific WS2811 option
	///
	/// @param ledOption The WS2811 configuration
	/// @param byte The byte to translate
	///
	/// @return The signal for the given byte (one byte per bit)
	///
	ByteSignal translate(SignalTiming ledOption, uint8_t byte);
}

class LedDeviceWs2811 : public LedRs232Device
{
public:
	///
	/// Constructs the LedDevice with Ws2811 attached via a serial port
	///
	/// @param outputDevice The name of the output device (eg '/dev/ttyS0')
	/// @param signalTiming The timing scheme used by the Ws2811 chip
	/// @param speedMode The speed modus of the Ws2811 chip
	///
	LedDeviceWs2811(const std::string& outputDevice, const ws2811::SignalTiming signalTiming, const ws2811::SpeedMode speedMode);

	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb> & ledValues);

	/// Switch the leds off
	virtual int switchOff();


private:

	///
	/// Fill the byte encoding table (_byteToSignalTable) for the specific timing option
	///
	/// @param ledOption The timing option
	///
	void fillEncodeTable(const ws2811::SignalTiming ledOption);

	/// Translation table of byte to signal///
	std::vector<ws2811::ByteSignal> _byteToSignalTable;

	/// The buffer containing the packed RGB values
	std::vector<ws2811::ByteSignal> _ledBuffer;
};
