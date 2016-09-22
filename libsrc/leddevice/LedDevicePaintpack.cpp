
// Hyperion includes
#include "LedDevicePaintpack.h"

// Use out report HID device
LedDevicePaintpack::LedDevicePaintpack(const Json::Value &deviceConfig)
	: ProviderHID(deviceConfig)
{
	_useFeature = false;
}

LedDevice* LedDevicePaintpack::construct(const Json::Value &deviceConfig)
{
	return new LedDevicePaintpack(deviceConfig);
}

int LedDevicePaintpack::write(const std::vector<ColorRgb> & ledValues)
{
	unsigned newSize = 3*_ledCount + 2;
	if (_ledBuffer.size() < newSize)
	{
		_ledBuffer.resize(newSize, uint8_t(0));
		_ledBuffer[0] = 3;
		_ledBuffer[1] = 0;
	}

	auto bufIt = _ledBuffer.begin()+2;
	for (const ColorRgb & color : ledValues)
	{
		*bufIt = color.red;
		++bufIt;
		*bufIt = color.green;
		++bufIt;
		*bufIt = color.blue;
		++bufIt;
	}

	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
