
// Local hyperion includes
#include "LedDeviceWs2811.h"

LedDeviceWs2811::LedDeviceWs2811(const std::string & deviceName, const bool fastDevice) :
	LedRs232Device(deviceName, fastDevice?4000000:2000000)
{
	fillEncodeTable();
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

void LedDeviceWs2811::fillEncodeTable()
{
	for (unsigned byteValue=0; byteValue<256; ++byteValue)
	{
		char byteSignal[4];
		for (unsigned iBit=0; iBit<8; iBit=2)
		{
			// Isolate two bits
			char bitVal = (byteValue >> (6-iBit)) & 0x03;

			switch (bitVal)
			{
			case 0:
				//  _          _
				// | | _ _ _ _| |_ _ _  _|
				//     <----bits----->
				byteSignal[iBit/2] = 0x08;
				break;
			case 1:
				//  _          _ _
				// | | _ _ _ _|   |_ _  _|
				//     <----bits----->
				byteSignal[iBit/2] = 0x0C;;
				break;
			case 2:
				//  _  _       _
				// |    |_ _ _| |_ _ _  _|
				//     <----bits----->
				byteSignal[iBit/2] = 0x88;
				break;
			case 3:
				//  _  _       _ _
				// |    |_ _ _|   |_ _  _|
				//     <----bits----->
				byteSignal[iBit/2] = 0x8C;
				break;
			default:
				// Should not happen
				std::cerr << "two bits evaluated to other value: " << bitVal << std::endl;
			}
		}
		const unsigned byteSignalVal =
				(byteSignal[0] & 0x00ff) <<  0 |
				(byteSignal[1] & 0x00ff) <<  8 |
				(byteSignal[2] & 0x00ff) << 16 |
				(byteSignal[3] & 0x00ff) << 24;
		_byteToSignalTable.push_back(byteSignalVal);
	}
}
