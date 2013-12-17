
// Local hyperion includes
#include "LedDeviceWs2811.h"


ws2811::SignalTiming ws2811::fromString(const std::string& signalTiming, const SignalTiming defaultValue)
{
	SignalTiming result = defaultValue;
	if (signalTiming == "3755" || signalTiming == "option_3755")
	{
		result = option_3755;
	}
	else if (signalTiming == "3773" || signalTiming == "option_3773")
	{
		result = option_3773;
	}
	else if (signalTiming == "2855" || signalTiming == "option_2855")
	{
		result = option_2855;
	}
	else if (signalTiming == "2882" || signalTiming == "option_2882")
	{
		result = option_2882;
	}

	return result;
}

unsigned ws2811::getBaudrate(const SpeedMode speedMode)
{
	switch (speedMode)
	{
	case highspeed:
		// Bit length: 125ns
		return 8000000;
	case lowspeed:
		// Bit length: 250ns
		return 4000000;
	}

	return 0;
}
inline unsigned ws2811::getLength(const SignalTiming timing, const TimeOption option)
{
	switch (timing)
	{
	case option_3755:
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
	case option_3773:
		// Reference: www.adafruit.com/datasheets/WS2812.pdf‎
		// Unit length: 125ns
		switch (option)
		{
		case T0H:
			return 3; // 350ns +-150ns
		case T0L:
			return 7; // 800ns +-150ns
		case T1H:
			return 7; // 700ns +-150ns
		case T1L:
			return 3; // 600ns +-150ns
		}
	case option_2855:
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
	case option_2882:
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
	default:
		std::cerr << "Unknown signal timing for ws2811: " << timing << std::endl;
	}
	return 0;
}

uint8_t ws2811::bitToSignal(unsigned lenHigh)
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

ws2811::ByteSignal ws2811::translate(SignalTiming ledOption, uint8_t byte)
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

LedDeviceWs2811::LedDeviceWs2811(
		const std::string & outputDevice,
		const ws2811::SignalTiming signalTiming,
		const ws2811::SpeedMode speedMode) :
	LedRs232Device(outputDevice, ws2811::getBaudrate(speedMode))
{
	fillEncodeTable(signalTiming);
}

int LedDeviceWs2811::write(const std::vector<ColorRgb> & ledValues)
{
	if (_ledBuffer.size() != ledValues.size() * 3)
	{
		_ledBuffer.resize(ledValues.size() * 3);
	}

	auto bufIt = _ledBuffer.begin();
	for (const ColorRgb & color : ledValues)
	{
		*bufIt = _byteToSignalTable[color.red  ];
		++bufIt;
		*bufIt = _byteToSignalTable[color.green];
		++bufIt;
		*bufIt = _byteToSignalTable[color.blue ];
		++bufIt;
	}

	writeBytes(_ledBuffer.size() * 3, reinterpret_cast<uint8_t *>(_ledBuffer.data()));

	return 0;
}

int LedDeviceWs2811::switchOff()
{
	write(std::vector<ColorRgb>(_ledBuffer.size()/3, ColorRgb::BLACK));
	return 0;
}

void LedDeviceWs2811::fillEncodeTable(const ws2811::SignalTiming ledOption)
{
	_byteToSignalTable.resize(256);
	for (unsigned byteValue=0; byteValue<256; ++byteValue)
	{
		const uint8_t byteVal = uint8_t(byteValue);
		_byteToSignalTable[byteValue] = ws2811::translate(ledOption, byteVal);
	}
}
