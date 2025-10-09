// stl includes
#include <exception>
#include <cstring>

// Local Hyperion includes
#include "LedDeviceHyperionUsbasp.h"

// Constants which define the Hyperion USBasp device
namespace {
uint16_t _usbVendorId = 0x16c0;
uint16_t _usbProductId = 0x05dc;
QString  _usbProductDescription = "Hyperion led controller";
}

LedDeviceHyperionUsbasp::LedDeviceHyperionUsbasp(const QJsonObject &deviceConfig)
	: LedDevice(deviceConfig)
	, _libusbContext(nullptr)
	, _deviceHandle(nullptr)
{
}

LedDeviceHyperionUsbasp::~LedDeviceHyperionUsbasp()
{
	if (_libusbContext != nullptr)
	{
		libusb_exit(_libusbContext);
	}
}

LedDevice* LedDeviceHyperionUsbasp::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceHyperionUsbasp(deviceConfig);
}

bool LedDeviceHyperionUsbasp::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = false;

	// Initialise sub-class
	if ( LedDevice::init(deviceConfig) )
	{
		QString ledType = deviceConfig["ledType"].toString("ws2801");
		if (ledType != "ws2801" && ledType != "ws2812")
		{
			QString errortext = QString ("Invalid ledType; must be 'ws2801' or 'ws2812'.");
			this->setInError(errortext);
			isInitOK = false;
		}
		else
		{
			_writeLedsCommand = (ledType == "ws2801") ? CMD_WRITE_WS2801 : CMD_WRITE_WS2812;

			int error;
			// initialize the USB context
			if ( (error = libusb_init(&_libusbContext)) != LIBUSB_SUCCESS )
			{
				_libusbContext = nullptr;

				QString errortext = QString ("Error while initializing USB context(%1):%2").arg(error).arg(libusb_error_name(error));
				this->setInError(errortext);
				isInitOK = false;
			}
			else
			{
				Debug(_log, "USB context initialized");
#if 0
				libusb_set_debug(_libusbContext, 3);
#endif

				// retrieve the list of USB devices
				libusb_device ** deviceList;
				ssize_t deviceCount = libusb_get_device_list(_libusbContext, &deviceList);

				// iterate the list of devices
				for (ssize_t i = 0 ; i < deviceCount; ++i)
				{
					// try to open and initialize the device
					if ( testAndOpen(deviceList[i]) == 0 )
					{
						_device = deviceList[i];
						// a device was successfully opened. break from list
						break;
					}
				}

				// free the device list
				libusb_free_device_list(deviceList, 1);

				if (_deviceHandle == nullptr)
				{
					QString errortext;
					errortext = QString ("No %1 has been found").arg( _usbProductDescription);
					this->setInError( errortext );
				}
				else
				{
					isInitOK = true;
				}
			}
		}
	}

	return isInitOK;
}

int LedDeviceHyperionUsbasp::open()
{
	int retval = -1;
	_isDeviceReady = false;

	if ( libusb_open(_device, &_deviceHandle) != LIBUSB_SUCCESS )
	{
		QString errortext = QString ("Failed to open [%1]").arg(_usbProductDescription);
		this->setInError(errortext);
	}
	else
	{
		// Everything is OK -> enable device
		_isDeviceReady = true;
		retval = 0;
	}

	return retval;
}

int LedDeviceHyperionUsbasp::close()
{
	int retval = 0;
	_isDeviceReady = false;

	// LedDevice specific closing activities
	if (_deviceHandle != nullptr)
	{
		libusb_release_interface(_deviceHandle, 0);
		libusb_attach_kernel_driver(_deviceHandle, 0);
		libusb_close(_deviceHandle);

		_deviceHandle = nullptr;
	}
	return retval;
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

		// TODO: Check, if exceptions via try/catch need to be replaced in Qt environment
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
				static_cast<uint8_t>( LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT ), // request type
				_writeLedsCommand, // request
				0, // value
				0, // index
				(uint8_t *) ledValues.data(), // data
				(3*_ledCount) & 0xffff, // length
				5000); // timeout

	// Disabling interrupts for a little while on the device results in a PIPE error. All seems to keep functioning though...
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
