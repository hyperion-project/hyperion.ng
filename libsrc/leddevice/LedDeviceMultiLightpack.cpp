// stl includes
#include <exception>
#include <cstring>
#include <algorithm>

// Local Hyperion includes
#include "LedDeviceMultiLightpack.h"

// from USB_ID.h (http://code.google.com/p/light-pack/source/browse/CommonHeaders/USB_ID.h)
#define USB_OLD_VENDOR_ID  0x03EB
#define USB_OLD_PRODUCT_ID 0x204F
#define USB_VENDOR_ID      0x1D50
#define USB_PRODUCT_ID     0x6022

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
	// retrieve a list with Lightpack serials
	std::list<std::string> serialList = getLightpackSerials();

	// sort the list of Lightpacks based on the serial to get a fixed order
	std::sort(_lightpacks.begin(), _lightpacks.end(), compareLightpacks);

	// open each lightpack device
	for (const std::string & serial : serialList)
	{
		LedDeviceLightpack * device = new LedDeviceLightpack();
		int error = device->open(serial);

		if (error == 0)
		{
			_lightpacks.push_back(device);
		}
		else
		{
			std::cerr << "Error while creating Lightpack device with serial " << serial << std::endl;
			delete device;
		}
	}

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

std::list<std::string> LedDeviceMultiLightpack::getLightpackSerials()
{
	std::list<std::string> serialList;

	std::cout << "Getting list of Lightpack serials" << std::endl;

	// initialize the usb context
	libusb_context * libusbContext;
	int error = libusb_init(&libusbContext);
	if (error != LIBUSB_SUCCESS)
	{
		std::cerr << "Error while initializing USB context(" << error << "): " << libusb_error_name(error) << std::endl;
		libusbContext = nullptr;
		return serialList;
	}
	//libusb_set_debug(_libusbContext, 3);
	std::cout << "USB context initialized in multi Lightpack device" << std::endl;

	// retrieve the list of usb devices
	libusb_device ** deviceList;
	ssize_t deviceCount = libusb_get_device_list(libusbContext, &deviceList);

	// iterate the list of devices
	for (ssize_t i = 0 ; i < deviceCount; ++i)
	{
		libusb_device_descriptor deviceDescriptor;
		error = libusb_get_device_descriptor(deviceList[i], &deviceDescriptor);
		if (error != LIBUSB_SUCCESS)
		{
			std::cerr << "Error while retrieving device descriptor(" << error << "): " << libusb_error_name(error) << std::endl;
			continue;
		}

		if ((deviceDescriptor.idVendor == USB_VENDOR_ID && deviceDescriptor.idProduct == USB_PRODUCT_ID) ||
			(deviceDescriptor.idVendor == USB_OLD_VENDOR_ID && deviceDescriptor.idProduct == USB_OLD_PRODUCT_ID))
		{
			std::cout << "Found a lightpack device. Retrieving serial..." << std::endl;

			// get the serial number
			std::string serialNumber;
			if (deviceDescriptor.iSerialNumber != 0)
			{
				try
				{
					serialNumber = LedDeviceMultiLightpack::getString(deviceList[i], deviceDescriptor.iSerialNumber);
				}
				catch (int e)
				{
					std::cerr << "Unable to retrieve serial number(" << e << "): " << libusb_error_name(e) << std::endl;
					continue;
				}
			}

			std::cout << "Lightpack device found with serial " << serialNumber << std::endl;
			serialList.push_back(serialNumber);
		}
	}

	// free the device list
	libusb_free_device_list(deviceList, 1);
	libusb_exit(libusbContext);

	return serialList;
}

std::string LedDeviceMultiLightpack::getString(libusb_device * device, int stringDescriptorIndex)
{
	libusb_device_handle * handle = nullptr;

	int error = libusb_open(device, &handle);
	if (error != LIBUSB_SUCCESS)
	{
		throw error;
	}

	char buffer[256];
	error = libusb_get_string_descriptor_ascii(handle, stringDescriptorIndex, reinterpret_cast<unsigned char *>(buffer), sizeof(buffer));
	if (error <= 0)
	{
		libusb_close(handle);
		throw error;
	}

	libusb_close(handle);
	return std::string(buffer, error);
}
