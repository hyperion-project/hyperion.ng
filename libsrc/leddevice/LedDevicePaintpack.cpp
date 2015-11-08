
// Hyperion includes
#include "LedDevicePaintpack.h"

// Use out report HID device
LedDevicePaintpack::LedDevicePaintpack(const unsigned short VendorId, const unsigned short ProductId, int delayAfterConnect_ms) :
	LedHIDDevice(VendorId, ProductId, delayAfterConnect_ms, false),
	_ledBuffer(0)
{
	// empty
}


int LedDevicePaintpack::write(const std::vector<ColorRgb> & ledValues)
{
	if (_ledBuffer.size() < 2 + ledValues.size()*3)
	{
		_ledBuffer.resize(2 + ledValues.size()*3, uint8_t(0));
		_ledBuffer[0] = 3;
		_ledBuffer[1] = 0;
	}

	auto bufIt = _ledBuffer.begin()+2;
	for (const ColorRgb & ledValue : ledValues)
	{
		*bufIt = ledValue.red;
		++bufIt;
		*bufIt = ledValue.green;
		++bufIt;
		*bufIt = ledValue.blue;
		++bufIt;
	}

	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}


int LedDevicePaintpack::switchOff()
{
	std::fill(_ledBuffer.begin() + 2, _ledBuffer.end(), uint8_t(0));
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
