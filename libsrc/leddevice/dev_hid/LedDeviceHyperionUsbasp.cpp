// stl includes
#include <exception>
#include <cstring>

// Local Hyperion includes
#include "LedDeviceHyperionUsbasp.h"

// Static constants which define the Hyperion Usbasp device
uint16_t LedDeviceHyperionUsbasp::_usbVendorId = 0x16c0;
uint16_t LedDeviceHyperionUsbasp::_usbProductId = 0x05dc;
QString  LedDeviceHyperionUsbasp::_usbProductDescription = "Hyperion led controller";


LedDeviceHyperionUsbasp::LedDeviceHyperionUsbasp(const QJsonObject &deviceConfig)
	: LedDevice()
	, _libusbContext(nullptr)
	, _deviceHandle(nullptr)
{
	init(deviceConfig);
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

bool LedDeviceHyperionUsbasp::init(const QJsonObject &deviceConfig)
{
	LedDevice::init(deviceConfig);

	QString ledType = deviceConfig["ledType"].toString("ws2801");
	if (ledType != "ws2801" && ledType != "ws2812")
	{
		throw std::runtime_error("HyperionUsbasp: invalid ledType; must be 'ws2801' or 'ws2812'.");
	}

	_writeLedsCommand = (ledType == "ws2801") ? CMD_WRITE_WS2801 : CMD_WRITE_WS2812;

	return true;
}

LedDevice* LedDeviceHyperionUsbasp::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceHyperionUsbasp(deviceConfig);
}


int LedDeviceHyperionUsbasp::open()
{
	int error;

	// initialize the usb context
	if ((error = libusb_init(&_libusbContext)) != LIBUSB_SUCCESS)
	{
		Error(_log, "Error while initializing USB context(%d):%s", error, libusb_error_name(error));
		_libusbContext = nullptr;
		return -1;
	}
	//libusb_set_debug(_libusbContext, 3);
	Debug(_log, "USB context initialized");

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
		Error(_log, "No %s has been found", QSTRING_CSTR(_usbProductDescription));
	}

	return _deviceHandle == nullptr ? -1 : 0;
}

int LedDeviceHyperionUsbasp::testAndOpen(libusb_device * device)
{
	libusb_device_descriptor deviceDescriptor;
	int error = libusb_get_device_descriptor(device, &deviceDescriptor);
	if (error != LIBUSB_SUCCESS)
	{
		Error(_log, "Error while retrieving device descriptor(%d): %s", error, libusb_error_name(error));
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

		Info(_log, "%s found: bus=%d address=%d", QSTRING_CSTR(_usbProductDescription), busNumber, addressNumber);

		try
		{
			_deviceHandle = openDevice(device);
			Info(_log, "%s successfully opened", QSTRING_CSTR(_usbProductDescription) );
			return 0;
		}
		catch(int e)
		{
			_deviceHandle = nullptr;
			Error(_log, "Unable to open %s. Searching for other device(%d): %s", QSTRING_CSTR(_usbProductDescription), e, libusb_error_name(e));
		}
	}

	return -1;
}

int LedDeviceHyperionUsbasp::write(const std::vector<ColorRgb> &ledValues)
{
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
		Error(_log, "Error while writing data to %s (%s)",  QSTRING_CSTR(_usbProductDescription), libusb_error_name(nbytes));
		return -1;
	}

	return 0;
}

libusb_device_handle * LedDeviceHyperionUsbasp::openDevice(libusb_device *device)
{
	Logger * log = Logger::getInstance("LedDevice");
	libusb_device_handle * handle = nullptr;

	int error = libusb_open(device, &handle);
	if (error != LIBUSB_SUCCESS)
	{
		Error(log, "unable to open device(%d): %s",error,libusb_error_name(error));
		throw error;
	}

	// detach kernel driver if it is active
	if (libusb_kernel_driver_active(handle, 0) == 1)
	{
		error = libusb_detach_kernel_driver(handle, 0);
		if (error != LIBUSB_SUCCESS)
		{
			Error(log, "unable to detach kernel driver(%d): %s",error,libusb_error_name(error));
			libusb_close(handle);
			throw error;
		}
	}

	error = libusb_claim_interface(handle, 0);
	if (error != LIBUSB_SUCCESS)
	{
		Error(log, "unable to claim interface(%d): %s", error, libusb_error_name(error));
		libusb_attach_kernel_driver(handle, 0);
		libusb_close(handle);
		throw error;
	}

	return handle;
}

QString LedDeviceHyperionUsbasp::getString(libusb_device * device, int stringDescriptorIndex)
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
	return QString(QByteArray(buffer, error));
}
