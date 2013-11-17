// stl includes
#include <exception>
#include <cstring>
#include <algorithm>

// Local Hyperion includes
#include "LedDeviceMultiLightpack.h"

bool compareLightpacks(LedDeviceLightpack * lhs, LedDeviceLightpack * rhs)
{
	return lhs->getSerialNumber() < rhs->getSerialNumber();
}

LedDeviceMultiLightpack::LedDeviceMultiLightpack() :
	LedDevice(),
	_lightpacks()
{
}

LedDeviceMultiLightpack::~LedDeviceMultiLightpack()
{
	for (LedDeviceLightpack * device : _lightpacks)
	{
		delete device;
	}
}

int LedDeviceMultiLightpack::open()
{
	int error;

	// initialize the usb context
	libusb_context * libusbContext;
	if ((error = libusb_init(&libusbContext)) != LIBUSB_SUCCESS)
	{
		std::cerr << "Error while initializing USB context(" << error << "): " << libusb_error_name(error) << std::endl;
		libusbContext = nullptr;
		return -1;
	}
	//libusb_set_debug(_libusbContext, 3);
	std::cout << "USB context initialized in multi Lightpack device" << std::endl;

	// retrieve the list of usb devices
	libusb_device ** deviceList;
	ssize_t deviceCount = libusb_get_device_list(libusbContext, &deviceList);

	// iterate the list of devices
	for (ssize_t i = 0 ; i < deviceCount; ++i)
	{
		// try to open the device as a Lightpack
		LedDeviceLightpack * device = new LedDeviceLightpack();
		error = device->open(deviceList[i]);

		if (error == 0)
		{
			// Device is successfully opened as Lightpack
			std::cout << "Lightpack with serial " << device->getSerialNumber() << " added to set of Lightpacks" << std::endl;
			_lightpacks.push_back(device);
		}
		else
		{
			// No Lightpack...
			delete device;
		}
	}

	// sort the list of Lightpacks based on the serial to get a fixed order
	std::sort(_lightpacks.begin(), _lightpacks.end(), compareLightpacks);

	// free the device list
	libusb_free_device_list(deviceList, 1);
	libusb_exit(libusbContext);

	if (_lightpacks.size() == 0)
	{
		std::cerr << "No Lightpack devices were found" << std::endl;
	}
	else
	{
		std::cout << _lightpacks.size() << " Lightpack devices were found" << std::endl;
	}

	return _lightpacks.size() > 0 ? 0 : -1;
}

int LedDeviceMultiLightpack::write(const std::vector<ColorRgb> &ledValues)
{
	const ColorRgb * data = ledValues.data();
	int size = ledValues.size();

	for (LedDeviceLightpack * device : _lightpacks)
	{
		int count = std::min(device->getLedCount(), size);

		if (count > 0)
		{
			device->write(data, count);

			data += count;
			size -= count;
		}
		else
		{
			std::cout << "Unable to write data to Lightpack device: no more led data available" << std::endl;
		}
	}

	return 0;
}

int LedDeviceMultiLightpack::switchOff()
{
	for (LedDeviceLightpack * device : _lightpacks)
	{
		device->switchOff();
	}

	return 0;
}
