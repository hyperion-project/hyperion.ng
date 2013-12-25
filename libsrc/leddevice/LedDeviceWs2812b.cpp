
// Local Hyperion-Leddevice includes
#include "LedDeviceWs2812b.h"

LedDeviceWs2812b::LedDeviceWs2812b() :
	LedRs232Device("/dev/ttyAMA0", 4000000)
{
	fillTable();
}

int LedDeviceWs2812b::write(const std::vector<ColorRgb> & ledValues)
{
	// Ensure the size of the led-buffer
	if (_ledBuffer.size() != ledValues.size()*3)
	{
		_ledBuffer.resize(ledValues.size()*3);
	}

	// Translate the channel of each color to a signal
	auto bufIt = _ledBuffer.begin();
	for (const ColorRgb& color : ledValues)
	{
		*bufIt++ = _byte2signalTable[color.red];
		*bufIt++ = _byte2signalTable[color.green];
		*bufIt++ = _byte2signalTable[color.blue];
	}

	return writeBytes(_ledBuffer.size()*sizeof(ByteSignal), reinterpret_cast<uint8_t *>(_ledBuffer.data()));
}

int LedDeviceWs2812b::switchOff()
{
	// Set all bytes in the signal buffer to zero
	for (ByteSignal & signal : _ledBuffer)
	{
		signal = _byte2signalTable[0];
	}

	return writeBytes(_ledBuffer.size()*sizeof(ByteSignal), reinterpret_cast<uint8_t *>(_ledBuffer.data()));
}

void LedDeviceWs2812b::fillTable()
{
	_byte2signalTable.clear();
	for (int byte=0; byte<256; ++byte)
	{
		const ByteSignal signal = byte2Signal(uint8_t(byte));
		_byte2signalTable.push_back(signal);
	}
}

LedDeviceWs2812b::ByteSignal LedDeviceWs2812b::byte2Signal(const uint8_t byte) const
{
	ByteSignal result;
	result.bit_12 = bits2Signal(byte & 0x80, byte & 0x40);
	result.bit_34 = bits2Signal(byte & 0x20, byte & 0x10);
	result.bit_56 = bits2Signal(byte & 0x08, byte & 0x04);
	result.bit_78 = bits2Signal(byte & 0x02, byte & 0x01);
	return result;
}

uint8_t LedDeviceWs2812b::bits2Signal(const bool bit1, const bool bit2) const
{
	// See https://github.com/tvdzwan/hyperion/wiki/Ws2812b for the explanation of the given
	// translations
	if (bit1)
	{
		if (bit2)
		{
			return 0x8C;
		}
		else
		{
			return 0xCC;
		}
	}
	else
	{
		if (bit2)
		{
			return 0x8E;
		}
		else
		{
			return 0xCE;
		}
	}

	return 0x00;
}
