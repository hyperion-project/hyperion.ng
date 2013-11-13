// stl includes
#include <exception>
#include <cstring>

// Local Hyperion includes
#include "LedDeviceLightpack.h"

// from USB_ID.h (http://code.google.com/p/light-pack/source/browse/CommonHeaders/USB_ID.h)
#define USB_OLD_VENDOR_ID  0x03EB
#define USB_OLD_PRODUCT_ID 0x204F
#define USB_VENDOR_ID      0x1D50
#define USB_PRODUCT_ID     0x6022

#define LIGHTPACK_INTERFACE 0

// from commands.h (http://code.google.com/p/light-pack/source/browse/CommonHeaders/commands.h)
// Commands to device, sends it in first byte of data[]
enum COMMANDS{
	CMD_UPDATE_LEDS = 1,
	CMD_OFF_ALL,
	CMD_SET_TIMER_OPTIONS,
	CMD_SET_PWM_LEVEL_MAX_VALUE, /* deprecated */
	CMD_SET_SMOOTH_SLOWDOWN,
	CMD_SET_BRIGHTNESS,

	CMD_NOP = 0x0F
};

// from commands.h (http://code.google.com/p/light-pack/source/browse/CommonHeaders/commands.h)
enum DATA_VERSION_INDEXES{
	INDEX_FW_VER_MAJOR = 1,
	INDEX_FW_VER_MINOR
};

LedDeviceLightpack::LedDeviceLightpack(const std::string &serialNumber) :
	LedDevice(),
	_libusbContext(nullptr),
	_deviceHandle(nullptr),
	_busNumber(-1),
	_addressNumber(-1),
	_serialNumber(serialNumber),
	_firmwareVersion({-1,-1}),
	_ledCount(-1),
	_bitsPerChannel(-1),
	_ledBuffer()
{
}

LedDeviceLightpack::~LedDeviceLightpack()
{
	if (_deviceHandle != nullptr)
	{
		libusb_release_interface(_deviceHandle, LIGHTPACK_INTERFACE);
		libusb_attach_kernel_driver(_deviceHandle, LIGHTPACK_INTERFACE);
		libusb_close(_deviceHandle);

		_deviceHandle = nullptr;
	}

	if (_libusbContext != nullptr)
	{
		libusb_exit(_libusbContext);
		_libusbContext = nullptr;
	}
}

int LedDeviceLightpack::open()
{
	int error;

	// initialize the usb context
	if ((error = libusb_init(&_libusbContext)) != LIBUSB_SUCCESS)
	{
		std::cerr << "Error while initializing USB context(" << error << "): " << libusb_error_name(error) << std::endl;
		_libusbContext = nullptr;
		return -1;
	}
	std::cout << "USB context initialized" << std::endl;

	// retrieve the list of usb devices
	libusb_device ** deviceList;
	ssize_t deviceCount = libusb_get_device_list(_libusbContext, &deviceList);

	// iterate the list of devices
	for (ssize_t i = 0 ; i < deviceCount; ++i)
	{
		libusb_device_descriptor deviceDescriptor;
		error = libusb_get_device_descriptor(deviceList[i], &deviceDescriptor);
		if (error != LIBUSB_SUCCESS)
		{
			std::cerr << "Error while retrieving device descriptor(" << error << "): " << libusb_error_name(error) << std::endl;
			// continue with next usb device
			continue;
		}

		if ((deviceDescriptor.idVendor == USB_VENDOR_ID && deviceDescriptor.idProduct == USB_PRODUCT_ID) ||
			(deviceDescriptor.idVendor == USB_OLD_VENDOR_ID && deviceDescriptor.idProduct == USB_OLD_PRODUCT_ID))
		{
			// get the hardware address
			int busNumber = libusb_get_bus_number(deviceList[i]);
			int addressNumber = libusb_get_device_address(deviceList[i]);

			// get the serial number
			std::string serialNumber;
			if (deviceDescriptor.iSerialNumber != 0)
			{
				try
				{
				serialNumber = LedDeviceLightpack::getString(deviceList[i], deviceDescriptor.iSerialNumber);
				}
				catch (int e)
				{
					std::cerr << "unable to retrieve serial number from Lightpack device(" << e << "): " << libusb_error_name(e) << std::endl;
				}
			}

			// get the firmware version
			Version version = {-1,-1};
			try
			{
				version = LedDeviceLightpack::getVersion(deviceList[i]);
			}
			catch (int e)
			{
				std::cerr << "unable to retrieve firmware version number from Lightpack device(" << e << "): " << libusb_error_name(e) << std::endl;
			}

			std::cout << "Lightpack device found: bus=" << busNumber << " address=" << addressNumber << " serial=" << serialNumber << " version=" << version.majorVersion << "." << version.minorVersion << std::endl;

			// check if this is the device we are looking for
			if (_serialNumber.empty() || _serialNumber == serialNumber)
			{
				// This is it!
				try
				{
					_deviceHandle = openDevice(deviceList[i]);
					_serialNumber = serialNumber;
					_busNumber = busNumber;
					_addressNumber = addressNumber;

					std::cout << "Lightpack device successfully opened" << std::endl;

					// break from the search loop
					break;
				}
				catch(int e)
				{
					std::cerr << "unable to retrieve open Lightpack device(" << e << "): " << libusb_error_name(e) << std::endl;
				}
			}
		}
	}

	// free the device list
	libusb_free_device_list(deviceList, 1);

	if (_deviceHandle != nullptr)
	{
		// FOR TESTING PURPOSE: FORCE MAJOR VERSION TO 6
		_firmwareVersion.majorVersion = 6;

		// disable smoothing of the chosen device
		disableSmoothing();

		// determine the number of leds
		if (_firmwareVersion.majorVersion == 4)
		{
			_ledCount = 8;
		}
		else
		{
			_ledCount = 10;
		}

		// determine the bits per channel
		if (_firmwareVersion.majorVersion == 6)
		{
			// maybe also or version 7? The firmware suggest this is only for 6... (2013-11-13)
			_bitsPerChannel = 12;
		}
		else
		{
			_bitsPerChannel = 8;
		}

		// set the led buffer size (command + 6 bytes per led)
		_ledBuffer.resize(1 + _ledCount * 6, 0);
		_ledBuffer[0] = CMD_UPDATE_LEDS;
	}

	return _deviceHandle == nullptr ? -1 : 0;
}

int LedDeviceLightpack::write(const std::vector<ColorRgb> &ledValues)
{
	int count = std::min(_ledCount, (int) ledValues.size());

	for (int i = 0; i < count ; ++i)
	{
		const ColorRgb & color = ledValues[i];

		// copy the most significant bits of the rgb values to the first three bytes
		// offset 1 to accomodate for the command byte
		_ledBuffer[6*i+1] = color.red;
		_ledBuffer[6*i+2] = color.green;
		_ledBuffer[6*i+3] = color.blue;

		// leave the next three bytes on zero...
		// 12-bit values have zeros in the lowest 4 bits which is almost correct, but it saves extra
		// switches to determine what to do and some bit shuffling
	}

	return writeBytes(_ledBuffer.data(), _ledBuffer.size());
}

int LedDeviceLightpack::switchOff()
{
	unsigned char buf[1] = {CMD_OFF_ALL};
	return writeBytes(buf, sizeof(buf)) == sizeof(buf);
}

int LedDeviceLightpack::writeBytes(uint8_t *data, int size)
{
	return libusb_control_transfer(_deviceHandle,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS	| LIBUSB_RECIPIENT_INTERFACE,
		0x09,
		(2 << 8),
		0x00,
		data, size, 100);
}

int LedDeviceLightpack::disableSmoothing()
{
	unsigned char buf[2] = {CMD_SET_SMOOTH_SLOWDOWN, 0};
	return writeBytes(buf, sizeof(buf)) == sizeof(buf);
}

libusb_device_handle * LedDeviceLightpack::openDevice(libusb_device *device)
{
	libusb_device_handle * handle = nullptr;

	int error = libusb_open(device, &handle);
	if (error != LIBUSB_SUCCESS)
	{
		throw error;
	}

	error = libusb_detach_kernel_driver(handle, LIGHTPACK_INTERFACE);
	if (error != LIBUSB_SUCCESS)
	{
		throw error;
	}

	error = libusb_claim_interface(handle, LIGHTPACK_INTERFACE);
	if (error != LIBUSB_SUCCESS)
	{
		throw error;
	}

	return handle;
}

std::string LedDeviceLightpack::getString(libusb_device * device, int stringDescriptorIndex)
{
	libusb_device_handle * deviceHandle = openDevice(device);

	char buffer[256];
	int error = libusb_get_string_descriptor_ascii(deviceHandle, stringDescriptorIndex, reinterpret_cast<unsigned char *>(buffer), sizeof(buffer));
	if (error <= 0)
	{
		throw error;
	}

	libusb_close(deviceHandle);
	return std::string(buffer, error);
}

LedDeviceLightpack::Version LedDeviceLightpack::getVersion(libusb_device *device)
{
	libusb_device_handle * deviceHandle = openDevice(device);

	uint8_t buffer[256];
	int error = libusb_get_descriptor(deviceHandle, LIBUSB_DT_REPORT, 0, buffer, sizeof(buffer));
	if (error <= 3)
	{
		throw error;
	}

	libusb_close(deviceHandle);
	return Version{buffer[INDEX_FW_VER_MAJOR], buffer[INDEX_FW_VER_MINOR]};
}
