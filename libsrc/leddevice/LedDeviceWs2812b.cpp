
// Linux includes
#include <unistd.h>

// Local Hyperion-Leddevice includes
#include "LedDeviceWs2812b.h"

LedDeviceWs2812b::LedDeviceWs2812b() :
	LedRs232Device("/dev/ttyAMA0", 2500000)
{
	// empty
}

int LedDeviceWs2812b::write(const std::vector<ColorRgb> & ledValues)
{
	// Ensure the size of the led-buffer
	if (_ledBuffer.size() != ledValues.size()*8)
	{
		_ledBuffer.resize(ledValues.size()*8, ~0x24);
	}

	// Translate the channel of each color to a signal
	uint8_t * signal_ptr = _ledBuffer.data();
	for (const ColorRgb & color : ledValues)
	{
		signal_ptr = color2signal(color, signal_ptr);
	}

	const int result = writeBytes(_ledBuffer.size(), _ledBuffer.data());
	// Official latch time is 50us (lets give it 50us more)
	usleep(100);
	return result;
}

uint8_t * LedDeviceWs2812b::color2signal(const ColorRgb & color, uint8_t * signal)
{
	*signal = bits2Signal(color.red & 0x80, color.red & 0x40, color.red & 0x20);
	++signal;
	*signal = bits2Signal(color.red & 0x10, color.red & 0x08, color.red & 0x04);
	++signal;
	*signal = bits2Signal(color.red & 0x02, color.green & 0x01, color.green & 0x80);
	++signal;
	*signal = bits2Signal(color.green & 0x40, color.green & 0x20, color.green & 0x10);
	++signal;
	*signal = bits2Signal(color.green & 0x08, color.green & 0x04, color.green & 0x02);
	++signal;
	*signal = bits2Signal(color.green & 0x01, color.blue & 0x80, color.blue & 0x40);
	++signal;
	*signal = bits2Signal(color.blue & 0x20, color.blue & 0x10, color.blue & 0x08);
	++signal;
	*signal = bits2Signal(color.blue & 0x04, color.blue & 0x02, color.blue & 0x01);
	++signal;

	return signal;
}

int LedDeviceWs2812b::switchOff()
{
	// Set all bytes in the signal buffer to zero
	for (uint8_t & signal : _ledBuffer)
	{
		signal = ~0x24;
	}

	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

uint8_t LedDeviceWs2812b::bits2Signal(const bool bit_1, const bool bit_2, const bool bit_3) const
{
	// See https://github.com/tvdzwan/hyperion/wiki/Ws2812b for the explanation of the given
	// translations

	// Bit index(default):1   2   3
	//                    |   |   |
	// default value  (1) 00 100 10 (0)
	//
	// Reversed value (1) 01 001 00 (0)
	//                    |   |   |
	// Bit index (rev):   3   2   1
	uint8_t result = 0x24;

	if(bit_1)
	{
		result |= 0x01;
	}
	if (bit_2)
	{
		result |= 0x08;
	}
	if (bit_3)
	{
		result |= 0x40;
	}

	return ~result;
}
