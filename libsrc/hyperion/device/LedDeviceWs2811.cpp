
// Local hyperion includes
#include "LedDeviceWs2811.h"

LedDeviceWs2811::LedDeviceWs2811(const std::string & deviceName) :
	LedRs232Device(deviceName, ws2811::getBaudrate(ws2811::option_1))
{
	fillEncodeTable(ws2811::option_1);
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
