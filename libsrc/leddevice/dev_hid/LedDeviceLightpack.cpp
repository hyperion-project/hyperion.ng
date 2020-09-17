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

LedDeviceLightpack::LedDeviceLightpack(const QJsonObject &deviceConfig)
	: LedDevice(deviceConfig)
	  , _libusbContext(nullptr)
	  , _deviceHandle(nullptr)
	  , _busNumber(-1)
	  , _addressNumber(-1)
	  , _firmwareVersion({-1,-1})
	  , _bitsPerChannel(-1)
	  , _hwLedCount(-1)
	  , _isOpen(false)
{
}

LedDeviceLightpack::~LedDeviceLightpack()
{
	if (_libusbContext != nullptr)
	{
		libusb_exit(_libusbContext);
	}
}

LedDevice* LedDeviceLightpack::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceLightpack(deviceConfig);
}

bool LedDeviceLightpack::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = false;

	// Initialise sub-class
	if ( LedDevice::init(deviceConfig) )
	{
		_serialNumber = deviceConfig["serial"].toString("");

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
			//libusb_set_debug(_libusbContext, 3);

			// retrieve the list of USB devices
			libusb_device ** deviceList;
			ssize_t deviceCount = libusb_get_device_list(_libusbContext, &deviceList);

			// iterate the list of devices
			for (ssize_t i = 0 ; i < deviceCount; ++i)
			{
				// try to open and initialize the device
				if (testAndOpen(deviceList[i], _serialNumber) == 0)
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
				if (_serialNumber.isEmpty())
				{
					errortext = QString ("No Lightpack devices were found");
				}
				else
				{
					errortext = QString ("No Lightpack device has been found with serial %1").arg( _serialNumber);
				}
				this->setInError( errortext );
			}
			else
			{
				isInitOK = true;
			}
		}
	}
	return isInitOK;
}

int LedDeviceLightpack::open()
{
	int retval = -1;
	_isDeviceReady = false;

	if ( libusb_open(_device, &_deviceHandle) != LIBUSB_SUCCESS )
	{
		QString errortext = QString ("Failed to open [%1]").arg(_serialNumber);
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

int LedDeviceLightpack::close()
{
	int retval = 0;
	_isDeviceReady = false;

	// LedDevice specific closing activities
	if (_deviceHandle != nullptr)
	{
		_isOpen = false;
		libusb_release_interface(_deviceHandle, LIGHTPACK_INTERFACE);
		libusb_attach_kernel_driver(_deviceHandle, LIGHTPACK_INTERFACE);
		libusb_close(_deviceHandle);

		_deviceHandle = nullptr;
	}
	return retval;
}

int LedDeviceLightpack::testAndOpen(libusb_device * device, const QString & requestedSerialNumber)
{
	libusb_device_descriptor deviceDescriptor;
	int error = libusb_get_device_descriptor(device, &deviceDescriptor);
	if (error != LIBUSB_SUCCESS)
	{
		Error(_log, "Error while retrieving device descriptor(%d): %s", error, libusb_error_name(error));
		return -1;
	}

	if ((deviceDescriptor.idVendor == USB_VENDOR_ID && deviceDescriptor.idProduct == USB_PRODUCT_ID) ||
		(deviceDescriptor.idVendor == USB_OLD_VENDOR_ID && deviceDescriptor.idProduct == USB_OLD_PRODUCT_ID))
	{
		Info(_log, "Found a Lightpack device. Retrieving more information...");

		// get the hardware address
		int busNumber = libusb_get_bus_number(device);
		int addressNumber = libusb_get_device_address(device);

		// get the serial number
		QString serialNumber;
		if (deviceDescriptor.iSerialNumber != 0)
		{
			// TODO: Check, if exceptions via try/catch need to be replaced in Qt environment
			try
			{
				serialNumber = LedDeviceLightpack::getString(device, deviceDescriptor.iSerialNumber);
			}
			catch (int e)
			{
				Error(_log, "unable to retrieve serial number from Lightpack device(%d): %s", e, libusb_error_name(e));
				serialNumber = "";
			}
		}

		Debug(_log,"Lightpack device found: bus=%d address=%d serial=%s", busNumber, addressNumber, QSTRING_CSTR(serialNumber));

		// check if this is the device we are looking for
		if (requestedSerialNumber.isEmpty() || requestedSerialNumber == serialNumber)
		{
			// This is it!
			// TODO: Check, if exceptions via try/catch need to be replaced in Qt environment
			try
			{
				_deviceHandle = openDevice(device);
				_serialNumber = serialNumber;
				_busNumber = busNumber;
				_addressNumber = addressNumber;

				Info(_log, "Lightpack device successfully opened");

				// get the firmware version
				uint8_t buffer[256];
				error = libusb_control_transfer(
							_deviceHandle,
							static_cast<uint8_t>( LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE),
							0x01,
							0x0100,
							0,
							buffer, sizeof(buffer), 1000);
				if (error < 3)
				{
					Error(_log, "Unable to retrieve firmware version number from Lightpack device(%d): %s", error, libusb_error_name(error));
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
					_hwLedCount = 8;
				}
				else
				{
					_hwLedCount = 10;
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
				_ledBuffer = std::vector<uint8_t>(1 + _hwLedCount * 6, 0);
				_ledBuffer[0] = CMD_UPDATE_LEDS;

				// return success
				Debug(_log, "Lightpack device opened: bus=%d address=%d serial=%s version=%d.%d.", _busNumber, _addressNumber, QSTRING_CSTR(_serialNumber), _firmwareVersion.majorVersion, _firmwareVersion.minorVersion );
				return 0;
			}
			catch(int e)
			{
				_deviceHandle = nullptr;
				Warning(_log, "Unable to open Lightpack device. Searching for other device(%d): %s", e, libusb_error_name(e));
			}
		}
	}

	return -1;
}

int LedDeviceLightpack::write(const std::vector<ColorRgb> &ledValues)
{
	return write(ledValues.data(), static_cast<int>(ledValues.size()));
}

int LedDeviceLightpack::write(const ColorRgb * ledValues, int size)
{
	int count = qMin(_hwLedCount, static_cast<int>( size ));

	for (int i = 0; i < count ; ++i)
	{
		const ColorRgb & color = ledValues[i];

		// copy the most significant bits of the RGB values to the first three bytes
		// offset 1 to accommodate for the command byte
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

bool LedDeviceLightpack::powerOff()
{
	bool rc = false;

	unsigned char buf[1] = {CMD_OFF_ALL};
	rc = writeBytes(buf, sizeof(buf)) == sizeof(buf);

	return rc;
}

const QString &LedDeviceLightpack::getSerialNumber() const
{
	return _serialNumber;
}

int LedDeviceLightpack::writeBytes(uint8_t *data, int size)
{
//	std::cout << "Writing " << size << " bytes: ";
//	for (int i = 0; i < size ; ++i) printf("%02x ", data[i]);
//	std::cout << std::endl;

	int error = libusb_control_transfer(_deviceHandle,
									 static_cast<uint8_t>( LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE ),
		0x09,
		(2 << 8),
		0x00,
		data, size, 1000);

	if (error == size)
	{
		return 0;
	}

	Error(_log, "Unable to write %d bytes to Lightpack device(%d): %s", size, error, libusb_error_name(error));
	return error;
}

int LedDeviceLightpack::disableSmoothing()
{
	unsigned char buf[2] = {CMD_SET_SMOOTH_SLOWDOWN, 0};

	int rc = 0;
	if (  writeBytes(buf, sizeof(buf)) == sizeof(buf) )
	{
		rc = 1;
	}
	return rc;
}

libusb_device_handle * LedDeviceLightpack::openDevice(libusb_device *device)
{
	libusb_device_handle * handle = nullptr;
	Logger * log = Logger::getInstance("LedDevice");
	int error = libusb_open(device, &handle);
	if (error != LIBUSB_SUCCESS)
	{
		Error(log, "unable to open device(%d): %s", error, libusb_error_name(error));
		throw error;
	}

	// detach kernel driver if it is active
	if (libusb_kernel_driver_active(handle, LIGHTPACK_INTERFACE) == 1)
	{
		error = libusb_detach_kernel_driver(handle, LIGHTPACK_INTERFACE);
		if (error != LIBUSB_SUCCESS)
		{
			Error(log, "unable to detach kernel driver(%d): %s", error, libusb_error_name(error));
			libusb_close(handle);
			throw error;
		}
	}

	error = libusb_claim_interface(handle, LIGHTPACK_INTERFACE);
	if (error != LIBUSB_SUCCESS)
	{
		Error(log, "unable to claim interface(%d): %s", error, libusb_error_name(error));
		libusb_attach_kernel_driver(handle, LIGHTPACK_INTERFACE);
		libusb_close(handle);
		throw error;
	}

	return handle;
}

QString LedDeviceLightpack::getString(libusb_device * device, int stringDescriptorIndex)
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
