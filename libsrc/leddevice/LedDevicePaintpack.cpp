
// Hyperion includes
#include "LedDevicePaintpack.h"

LedDevicePaintpack::LedDevicePaintpack() :
	LedDevice(),
	_deviceHandle(nullptr)
{
	// empty
}

int LedDevicePaintpack::open()
{
	// initialize the usb context
	int error = hid_init();
	if (error != 0)
	{
		std::cerr << "Error while initializing the hidapi context" << std::endl;
		return -1;
	}
	std::cout << "Hidapi initialized" << std::endl;

	// Initialise the paintpack device
	const unsigned short Paintpack_VendorId  = 0x0ebf;
	const unsigned short Paintpack_ProductId = 0x0025;
	_deviceHandle = hid_open(Paintpack_VendorId, Paintpack_ProductId, nullptr);
	if (_deviceHandle == nullptr)
	{
		// Failed to open the device
		std::cerr << "Failed to open HID Paintpakc device " << std::endl;
		return -1;
	}

	return 0;
}

LedDevicePaintpack::~LedDevicePaintpack()
{
	if (_deviceHandle != nullptr)
	{
		hid_close(_deviceHandle);
		_deviceHandle = nullptr;
	}

	hid_exit();
}

int LedDevicePaintpack::write(const std::vector<ColorRgb>& ledValues)
{
	if (_ledBuffer.size() < 3 + ledValues.size()*3)
	{
		_ledBuffer.resize(3 + ledValues.size()*3, uint8_t(0));

		_ledBuffer[0] = 0;
		_ledBuffer[1] = 3;
		_ledBuffer[2] = 0;
	}

	auto bufIt = _ledBuffer.begin()+3;
	for (const ColorRgb & ledValue : ledValues)
	{
		*bufIt = ledValue.red;
		++bufIt;
		*bufIt = ledValue.green;
		++bufIt;
		*bufIt = ledValue.blue;
		++bufIt;
	}

	return hid_write(_deviceHandle, _ledBuffer.data(), _ledBuffer.size());
}

int LedDevicePaintpack::switchOff()
{
	std::fill(_ledBuffer.begin()+3, _ledBuffer.end(), uint8_t(0));
	return hid_write(_deviceHandle, _ledBuffer.data(), _ledBuffer.size());
}
