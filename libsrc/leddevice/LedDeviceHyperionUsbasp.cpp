// stl includes
#include <exception>
#include <cstring>

// Local Hyperion includes
#include "LedDeviceHyperionUsbasp.h"

// Static constants which define the Hyperion Usbasp device
uint16_t LedDeviceHyperionUsbasp::_usbVendorId = 0x16c0;
uint16_t LedDeviceHyperionUsbasp::_usbProductId = 0x05dc;
std::string LedDeviceHyperionUsbasp::_usbProductDescription = "Hyperion led controller";


LedDeviceHyperionUsbasp::LedDeviceHyperionUsbasp(uint8_t writeLedsCommand) :
	LedDevice(),
	_writeLedsCommand(writeLedsCommand),
	_libusbContext(nullptr),
	_deviceHandle(nullptr),
	_ledCount(256)
{
}

LedDeviceHyperionUsbasp::~LedDeviceHyperionUsbasp()
{
	if (_deviceHandle != nullptr)
	{
		libusb_release_interface(_deviceHandle, 0);
		libusb_attach_kernel_driver(_deviceHandle, 0);
		libusb_close(_deviceHandle);

		_deviceHandle = nullptr;
	}

	if (_libusbContext != nullptr)
	{
		libusb_exit(_libusbContext);
		_libusbContext = nullptr;
	}
}

int LedDeviceHyperionUsbasp::open()
{
	int error;

	// initialize the usb context
	if ((error = libusb_init(&_libusbContext)) != LIBUSB_SUCCESS)
	{
		std::cerr << "Error while initializing USB context(" << error << "): " << libusb_error_name(error) << std::endl;
		_libusbContext = nullptr;
		return -1;
	}
	//libusb_set_debug(_libusbContext, 3);
	std::cout << "USB context initialized" << std::endl;

	// retrieve the list of usb devices
	libusb_device ** deviceList;
	ssize_t deviceCount = libusb_get_device_list(_libusbContext, &deviceList);

	// iterate the list of devices
	for (ssize_t i = 0 ; i < deviceCount; ++i)
	{
		// try to open and initialize the device
		error = testAndOpen(deviceList[i]);

		if (error == 0)
		{
			// a device was sucessfully opened. break from list
			break;
		}
	}

	// free the device list
	libusb_free_device_list(deviceList, 1);

	if (_deviceHandle == nullptr)
	{
		std::cerr << "No " << _usbProductDescription << " has been found" << std::endl;
	}

	return _deviceHandle == nullptr ? -1 : 0;
}

int LedDeviceHyperionUsbasp::testAndOpen(libusb_device * device)
{
	libusb_device_descriptor deviceDescriptor;
	int error = libusb_get_device_descriptor(device, &deviceDescriptor);
	if (error != LIBUSB_SUCCESS)
	{
		std::cerr << "Error while retrieving device descriptor(" << error << "): " << libusb_error_name(error) << std::endl;
		return -1;
	}

	if (deviceDescriptor.idVendor == _usbVendorId &&
			deviceDescriptor.idProduct == _usbProductId &&
			deviceDescriptor.iProduct != 0 &&
			getString(device, deviceDescriptor.iProduct) == _usbProductDescription)
	{
		// get the hardware address
		int busNumber = libusb_get_bus_number(device);
		int addressNumber = libusb_get_device_address(device);

		std::cout << _usbProductDescription << " found: bus=" << busNumber << " address=" << addressNumber << std::endl;

		try
		{
			_deviceHandle = openDevice(device);
			std::cout << _usbProductDescription << " successfully opened" << std::endl;
			return 0;
		}
		catch(int e)
		{
			_deviceHandle = nullptr;
			std::cerr << "Unable to open " << _usbProductDescription << ". Searching for other device(" << e << "): " << libusb_error_name(e) << std::endl;
		}
	}

	return -1;
}

int LedDeviceHyperionUsbasp::write(const std::vector<ColorRgb> &ledValues)
{
	_ledCount = ledValues.size();

	int nbytes = libusb_control_transfer(
				_deviceHandle, // device handle
				LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT, // request type
				_writeLedsCommand, // request
				0, // value
				0, // index
				(uint8_t *) ledValues.data(), // data
				(3*_ledCount) & 0xffff, // length
				5000); // timeout

	// Disabling interupts for a little while on the device results in a PIPE error. All seems to keep functioning though...
	if(nbytes < 0 && nbytes != LIBUSB_ERROR_PIPE)
	{
		std::cerr << "Error while writing data to " << _usbProductDescription << " (" << libusb_error_name(nbytes) << ")" << std::endl;
		return -1;
	}

	return 0;
}

int LedDeviceHyperionUsbasp::switchOff()
{
	std::vector<ColorRgb> ledValues(_ledCount, ColorRgb::BLACK);
	return write(ledValues);
}

libusb_device_handle * LedDeviceHyperionUsbasp::openDevice(libusb_device *device)
{
	libusb_device_handle * handle = nullptr;

	int error = libusb_open(device, &handle);
	if (error != LIBUSB_SUCCESS)
	{
		std::cerr << "unable to open device(" << error << "): " << libusb_error_name(error) << std::endl;
		throw error;
	}

	// detach kernel driver if it is active
	if (libusb_kernel_driver_active(handle, 0) == 1)
	{
		error = libusb_detach_kernel_driver(handle, 0);
		if (error != LIBUSB_SUCCESS)
		{
			std::cerr << "unable to detach kernel driver(" << error << "): " << libusb_error_name(error) << std::endl;
			libusb_close(handle);
			throw error;
		}
	}

	error = libusb_claim_interface(handle, 0);
	if (error != LIBUSB_SUCCESS)
	{
		std::cerr << "unable to claim interface(" << error << "): " << libusb_error_name(error) << std::endl;
		libusb_attach_kernel_driver(handle, 0);
		libusb_close(handle);
		throw error;
	}

	return handle;
}

std::string LedDeviceHyperionUsbasp::getString(libusb_device * device, int stringDescriptorIndex)
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
