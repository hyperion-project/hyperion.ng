#pragma once

// STL includes
#include <cassert>

// Local hyperion includes
#include "LedRs232Device.h"

namespace ws2811
{
	enum SignalTiming
	{
		option_1,
		option_2,
		option_3,
		option_4
	};

	/**
	 * Enumeration of the signal 'parts' (T 0 high, T 1 high, T 0 low, T 1 low).
	 */
	enum TimeOption
	{
		T0H,
		T1H,
		T0L,
		T1L
	};

	/**
	 * Returns the required baudrate for a specific signal-timing
	 *
	 * @param timing The WS2811/WS2812/WS2812b option
	 *
	 * @return The required baudrate for the signal timing
	 */
	inline unsigned getBaudrate(const SignalTiming timing)
	{
		switch (timing)
		{
		case option_1:
		case option_2:
			// Bit length: 125ns
			return 8000000;
		case option_3:
		case option_4:
			// Bit length: 250ns
			return 4000000;
		}

		return 0;
	}

	/**
	 * The number of 'signal units' (bits) For the subpart of a specific timing scheme
	 *
	 * @param timing The controller option
	 * @param option The signal part
	 */
	inline unsigned getLength(const SignalTiming timing, const TimeOption option)
	{
		switch (timing)
		{
		case option_1:
			// Reference: www.adafruit.com/datasheets/WS2812.pdf‎
			// Unit length: 125ns
			switch (option)
			{
			case T0H:
				return 3; // 350ns +-150ns
			case T0L:
				return 7; // 800ns +-150ns
			case T1H:
				return 6; // 700ns +-150ns
			case T1L:
				return 4; // 600ns +-150ns
			}
		case option_3:
			// Reference: http://www.mikrocontroller.net/attachment/180459/WS2812B_preliminary.pdf
			// Unit length: 125ns
			switch (option)
			{
			case T0H:
				return 3; // 400ns +-150ns
			case T0L:
				return 7; // 850ns +-150ns
			case T1H:
				return 7; // 800ns +-150ns
			case T1L:
				return 3; // 450ns +-150ns
			}
		case option_2:
			// Reference: www.adafruit.com/datasheets/WS2811.pdf‎
			// Unit length: 250ns
			switch (option)
			{
			case T0H:
				return 2; //  500ns +-150ns
			case T0L:
				return 8; // 2000ns +-150ns
			case T1H:
				return 5; // 1200ns +-150ns
			case T1L:
				return 5; // 1300ns +-150ns
			}
		case option_4:
			// Reference: www.szparkson.net/download/WS2811.pdf‎
			// Unit length: 250ns
			switch (option)
			{
			case T0H:
				return 2; //  500ns +-150ns
			case T0L:
				return 8; // 2000ns +-150ns
			case T1H:
				return 8; // 2000ns +-150ns
			case T1L:
				return 2; //  500ns +-150ns
			}
		}
		return 0;
	}

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

	static_assert(sizeof(ByteSignal) == 8, "Incorrect sizeof ByteSignal (expected 8)");

	/**
	 * Constructs a 'bit' based signal with defined 'high' length (and implicite defined 'low'
	 * length. The signal is based on a 10bits bytes (incl. high startbit and low stopbit). The
	 * total length of the high is given as parameter:<br>
	 * lenHigh=7 => |-------|___| => 1 1111 1100 0 => 252 (start and stop bit are implicite)
	 *
	 * @param lenHigh The total length of the 'high' length (incl start-bit)
	 * @return The byte representing the high-low signal
	 */
	inline uint8_t bitToSignal(unsigned lenHigh)
	{
		// Sanity check on the length of the 'high' signal
		assert(0 < lenHigh && lenHigh < 10);

		uint8_t result = 0x00;
		for (unsigned i=1; i<lenHigh; ++i)
		{
			result |= (1 << (8-i));
		}
		return result;
	}

	/**
	 * Translate a byte into signal levels for a specific WS2811 option
	 *
	 * @param ledOption The WS2811 configuration
	 * @param byte The byte to translate
	 *
	 * @return The signal for the given byte (one byte per bit)
	 */
	inline ByteSignal translate(SignalTiming ledOption, uint8_t byte)
	{
		ByteSignal result;
		result.bit_1 = bitToSignal(getLength(ledOption, (byte & 0x80)?T1H:T0H));
		result.bit_2 = bitToSignal(getLength(ledOption, (byte & 0x40)?T1H:T0H));
		result.bit_3 = bitToSignal(getLength(ledOption, (byte & 0x20)?T1H:T0H));
		result.bit_4 = bitToSignal(getLength(ledOption, (byte & 0x10)?T1H:T0H));
		result.bit_5 = bitToSignal(getLength(ledOption, (byte & 0x08)?T1H:T0H));
		result.bit_6 = bitToSignal(getLength(ledOption, (byte & 0x04)?T1H:T0H));
		result.bit_7 = bitToSignal(getLength(ledOption, (byte & 0x02)?T1H:T0H));
		result.bit_8 = bitToSignal(getLength(ledOption, (byte & 0x01)?T1H:T0H));
		return result;
	}
}

class LedDeviceWs2811 : public LedRs232Device
{
public:
	///
	/// Constructs the LedDevice with Ws2811 attached via a serial port
	///
	/// @param outputDevice The name of the output device (eg '/dev/ttyS0')
	/// @param fastDevice The used baudrate for writing to the output device
	///
	LedDeviceWs2811(const std::string& outputDevice);

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

	void fillEncodeTable(const ws2811::SignalTiming ledOption);
	/** Translation table of byte to signal */
	std::vector<ws2811::ByteSignal> _byteToSignalTable;

	/// The buffer containing the packed RGB values
	std::vector<ws2811::ByteSignal> _ledBuffer;
};
