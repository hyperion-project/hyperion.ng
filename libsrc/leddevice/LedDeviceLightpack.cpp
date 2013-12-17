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

LedDeviceLightpack::LedDeviceLightpack() :
	LedDevice(),
	_libusbContext(nullptr),
	_deviceHandle(nullptr),
	_busNumber(-1),
	_addressNumber(-1),
	_serialNumber(""),
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

int LedDeviceLightpack::open(const std::string & serialNumber)
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
		error = testAndOpen(deviceList[i], serialNumber);

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
		if (_serialNumber.empty())
		{
			std::cerr << "No Lightpack device has been found" << std::endl;
		}
		else
		{
			std::cerr << "No Lightpack device has been found with serial " << _serialNumber << std::endl;
		}
	}

	return _deviceHandle == nullptr ? -1 : 0;
}

int LedDeviceLightpack::testAndOpen(libusb_device * device, const std::string & requestedSerialNumber)
{
	libusb_device_descriptor deviceDescriptor;
	int error = libusb_get_device_descriptor(device, &deviceDescriptor);
	if (error != LIBUSB_SUCCESS)
	{
		std::cerr << "Error while retrieving device descriptor(" << error << "): " << libusb_error_name(error) << std::endl;
		return -1;
	}

	if ((deviceDescriptor.idVendor == USB_VENDOR_ID && deviceDescriptor.idProduct == USB_PRODUCT_ID) ||
		(deviceDescriptor.idVendor == USB_OLD_VENDOR_ID && deviceDescriptor.idProduct == USB_OLD_PRODUCT_ID))
	{
		std::cout << "Found a lightpack device. Retrieving more information..." << std::endl;

		// get the hardware address
		int busNumber = libusb_get_bus_number(device);
		int addressNumber = libusb_get_device_address(device);

		// get the serial number
		std::string serialNumber;
		if (deviceDescriptor.iSerialNumber != 0)
		{
			try
			{
				serialNumber = LedDeviceLightpack::getString(device, deviceDescriptor.iSerialNumber);
			}
			catch (int e)
			{
				std::cerr << "unable to retrieve serial number from Lightpack device(" << e << "): " << libusb_error_name(e) << std::endl;
				serialNumber = "";
			}
		}

		std::cout << "Lightpack device found: bus=" << busNumber << " address=" << addressNumber << " serial=" << serialNumber << std::endl;

		// check if this is the device we are looking for
		if (requestedSerialNumber.empty() || requestedSerialNumber == serialNumber)
		{
			// This is it!
			try
			{
				_deviceHandle = openDevice(device);
				_serialNumber = serialNumber;
				_busNumber = busNumber;
				_addressNumber = addressNumber;

				std::cout << "Lightpack device successfully opened" << std::endl;

				// get the firmware version
				uint8_t buffer[256];
				error = libusb_control_transfer(
							_deviceHandle,
							LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
							0x01,
							0x0100,
							0,
							buffer, sizeof(buffer), 1000);
				if (error < 3)
				{
					std::cerr << "Unable to retrieve firmware version number from Lightpack device(" << error << "): " << libusb_error_name(error) << std::endl;
				}
				else
				{
					_firmwareVersion.majorVersion = buffer[INDEX_FW_VER_MAJOR];
					_firmwareVersion.minorVersion = buffer[INDEX_FW_VER_MINOR];
				}

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
				_ledBuffer = std::vector<uint8_t>(1 + _ledCount * 6, 0);
				_ledBuffer[0] = CMD_UPDATE_LEDS;

				// return success
				std::cout << "Lightpack device opened: bus=" << _busNumber << " address=" << _addressNumber << " serial=" << _serialNumber << " version=" << _firmwareVersion.majorVersion << "." << _firmwareVersion.minorVersion << std::endl;
				return 0;
			}
			catch(int e)
			{
				_deviceHandle = nullptr;
				std::cerr << "Unable to open Lightpack device. Searching for other device(" << e << "): " << libusb_error_name(e) << std::endl;
			}
		}
	}

	return -1;
}

int LedDeviceLightpack::write(const std::vector<ColorRgb> &ledValues)
{
	return write(ledValues.data(), ledValues.size());
}

int LedDeviceLightpack::write(const ColorRgb * ledValues, int size)
{
	int count = std::min(_ledCount, size);

	for (int i = 0; i < count ; ++i)
	{
		const ColorRgb & color = ledValues[i];

		// copy the most significant bits of the rgb values to the first three bytes
		// offset 1 to accomodate for the command byte
		_ledBuffer[6*i+1] = color.red;
		_ledBuffer[6*i+2] = color.green;
		_ledBuffer[6*i+3] = color.blue;

		// leave the next three bytes on zero...
		// 12-bit values having zeros in the lowest 4 bits which is almost correct, but it saves extra
		// switches to determine what to do and some bit shuffling
	}

	int error = writeBytes(_ledBuffer.data(), _ledBuffer.size());
	return error >= 0 ? 0 : error;
}

int LedDeviceLightpack::switchOff()
{
	unsigned char buf[1] = {CMD_OFF_ALL};
	return writeBytes(buf, sizeof(buf)) == sizeof(buf);
}

const std::string &LedDeviceLightpack::getSerialNumber() const
{
	return _serialNumber;
}

int LedDeviceLightpack::getLedCount() const
{
	return _ledCount;
}

int LedDeviceLightpack::writeBytes(uint8_t *data, int size)
{
//	std::cout << "Writing " << size << " bytes: ";
//	for (int i = 0; i < size ; ++i) printf("%02x ", data[i]);
//	std::cout << std::endl;

	int error = libusb_control_transfer(_deviceHandle,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
		0x09,
		(2 << 8),
		0x00,
		data, size, 1000);

	if (error == size)
	{
		return 0;
	}

	std::cerr << "Unable to write " << size << " bytes to Lightpack device(" << error << "): " << libusb_error_name(error) << std::endl;
	return error;
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
		std::cerr << "unable to open device(" << error << "): " << libusb_error_name(error) << std::endl;
		throw error;
	}

	// detach kernel driver if it is active
	if (libusb_kernel_driver_active(handle, LIGHTPACK_INTERFACE) == 1)
	{
		error = libusb_detach_kernel_driver(handle, LIGHTPACK_INTERFACE);
		if (error != LIBUSB_SUCCESS)
		{
			std::cerr << "unable to detach kernel driver(" << error << "): " << libusb_error_name(error) << std::endl;
			libusb_close(handle);
			throw error;
		}
	}

	error = libusb_claim_interface(handle, LIGHTPACK_INTERFACE);
	if (error != LIBUSB_SUCCESS)
	{
		std::cerr << "unable to claim interface(" << error << "): " << libusb_error_name(error) << std::endl;
		libusb_attach_kernel_driver(handle, LIGHTPACK_INTERFACE);
		libusb_close(handle);
		throw error;
	}

	return handle;
}

std::string LedDeviceLightpack::getString(libusb_device * device, int stringDescriptorIndex)
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
