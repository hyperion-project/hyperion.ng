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
	  , _device(nullptr)
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

			if ( _log->getMinLevel() == Logger::LogLevel::DEBUG )
			{
				int logLevel = LIBUSB_LOG_LEVEL_INFO;
				#if LIBUSB_API_VERSION >= 0x01000106
					libusb_set_option(_libusbContext, LIBUSB_OPTION_LOG_LEVEL, logLevel);
				#else
					libusb_set_debug(_libusbContext, logLevel);
				#endif
			}

			// retrieve the list of USB devices
			libusb_device ** deviceList;
			ssize_t deviceCount = libusb_get_device_list(_libusbContext, &deviceList);

			bool deviceFound = true;
			// iterate the list of devices
			for (ssize_t i = 0 ; i < deviceCount; ++i)
			{
				// try to open and initialize the device
				deviceFound = searchDevice(deviceList[i], _serialNumber);
				if ( deviceFound )
				{
					_device = deviceList[i];
					// a device was successfully opened. break from list
					break;
				}
			}

			// free the device list
			libusb_free_device_list(deviceList, 1);

			if (!deviceFound)
			{
				QString errortext;
				if (_serialNumber.isEmpty())
				{
					errortext = QString ("No working Lightpack devices were found");
				}
				else
				{
					errortext = QString ("No working Lightpack device found with serial %1").arg( _serialNumber);
				}
				this->setInError( errortext );
			}
			else
			{
				// set the led buffer size (command + 6 bytes per led)
				_ledBuffer = std::vector<uint8_t>(1 + _hwLedCount * 6, 0);
				_ledBuffer[0] = CMD_UPDATE_LEDS;

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

	if ( _device != nullptr)
	{
		openDevice(_device, &_deviceHandle);

		if ( _deviceHandle == nullptr )
		{
			QString errortext = QString ("Failed to open device with serial [%1]").arg(_serialNumber);
			this->setInError(errortext);
			retval = -1;
		}
		else
		{
			disableSmoothing();
			{
				// Everything is OK
				_isDeviceReady = true;
				_isOpen = true;

				Info(_log, "Lightpack device successfully opened");
				retval = 0;
			}
		}
	}
	return retval;
}

int LedDeviceLightpack::close()
{
	int retval = 0;
	_isDeviceReady = false;
	_isOpen = false;

	if ( _deviceHandle != nullptr)
	{
		closeDevice(_deviceHandle);
		_deviceHandle = nullptr;
	}

	return retval;
}

bool LedDeviceLightpack::searchDevice(libusb_device * device, const QString & requestedSerialNumber)
{
	bool lightPackFound = false;

	libusb_device_descriptor deviceDescriptor;
	int error = libusb_get_device_descriptor(device, &deviceDescriptor);
	if (error != LIBUSB_SUCCESS)
	{
		Error(_log, "Error while retrieving device descriptor(%d): %s", error, libusb_error_name(error));
		return false;
	}

	if ((deviceDescriptor.idVendor == USB_VENDOR_ID && deviceDescriptor.idProduct == USB_PRODUCT_ID) ||
		 (deviceDescriptor.idVendor == USB_OLD_VENDOR_ID && deviceDescriptor.idProduct == USB_OLD_PRODUCT_ID))
	{
		Info(_log, "Found a Lightpack device. Retrieving more information...");

		Debug(_log, "vendorIdentifier : %s", QSTRING_CSTR(QString("0x%1").arg(static_cast<ushort>(deviceDescriptor.idVendor),0,16)));
		Debug(_log, "productIdentifier: %s", QSTRING_CSTR(QString("0x%1").arg(static_cast<ushort>(deviceDescriptor.idProduct),0,16)));
		Debug(_log, "release_number   : %s", QSTRING_CSTR(QString("0x%1").arg(static_cast<ushort>(deviceDescriptor.bcdDevice),0,16)));
		Debug(_log, "manufacturer     : %s", QSTRING_CSTR(getProperty(device, deviceDescriptor.iManufacturer)));

		// get the hardware address
		int busNumber = libusb_get_bus_number(device);
		int addressNumber = libusb_get_device_address(device);

		// get the serial number
		QString serialNumber = LedDeviceLightpack::getProperty(device, deviceDescriptor.iSerialNumber);
		Debug(_log,"Lightpack device found: bus=%d address=%d serial=%s", busNumber, addressNumber, QSTRING_CSTR(serialNumber));

		// check if this is the device we are looking for
		if (requestedSerialNumber.isEmpty() || requestedSerialNumber == serialNumber)
		{
			libusb_device_handle * deviceHandle;
			if ( openDevice(device, &deviceHandle ) == 0 )
			{
				_serialNumber = serialNumber;
				_busNumber = busNumber;
				_addressNumber = addressNumber;

				// get the firmware version
				uint8_t buffer[256];
				error = libusb_control_transfer(
							deviceHandle,
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

				#if 0
				// FOR TESTING PURPOSE: FORCE MAJOR VERSION TO 6
				_firmwareVersion.majorVersion = 6;
				#endif

				// determine the number of LEDs
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
				closeDevice(deviceHandle);

				Debug(_log, "Lightpack device found: bus=%d address=%d serial=%s version=%d.%d.", _busNumber, _addressNumber, QSTRING_CSTR(_serialNumber), _firmwareVersion.majorVersion, _firmwareVersion.minorVersion );
				lightPackFound = true;

			}
			else
			{
				Warning(_log, "Unable to open Lightpack device. Searching for other device");
			}
		}
	}

	return lightPackFound;
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
	int rc = 0;
	int error = libusb_control_transfer(_deviceHandle,
								 static_cast<uint8_t>( LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE ),
	0x09,
	(2 << 8),
	0x00,
	data, size, 1000);

	if (error != size)
	{
		rc = -1;
		Error(_log, "Unable to write %d bytes to Lightpack device(%d): %s", size, error, libusb_error_name(error));
	}

	return rc;
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

int LedDeviceLightpack::openDevice(libusb_device *device, libusb_device_handle ** deviceHandle)
{
	int rc = 0;

	libusb_device_handle * handle = nullptr;
	int error = libusb_open(device, &handle);
	if (error != LIBUSB_SUCCESS)
	{
		Error(_log, "unable to open device(%d): %s", error, libusb_error_name(error));
		rc = -1;
	}
	else
	{
		// detach kernel driver if it is active
		if (libusb_kernel_driver_active(handle, LIGHTPACK_INTERFACE) == 1)
		{
			error = libusb_detach_kernel_driver(handle, LIGHTPACK_INTERFACE);
			if (error != LIBUSB_SUCCESS)
			{
				Error(_log, "unable to detach kernel driver(%d): %s", error, libusb_error_name(error));
				libusb_close(handle);
				rc = -1;
			}
		}

		error = libusb_claim_interface(handle, LIGHTPACK_INTERFACE);
		if (error != LIBUSB_SUCCESS)
		{
			Error(_log, "unable to claim interface(%d): %s", error, libusb_error_name(error));
			libusb_attach_kernel_driver(handle, LIGHTPACK_INTERFACE);
			libusb_close(handle);
			rc = -1;
		}
	}

	*deviceHandle = handle;
	return rc;
}

int LedDeviceLightpack::closeDevice(libusb_device_handle * deviceHandle)
{
	int rc = 0;

	int error = libusb_release_interface(deviceHandle, LIGHTPACK_INTERFACE);
	if (error != LIBUSB_SUCCESS)
	{
		Debug(_log, "Error while releasing interface (%d): %s", error, libusb_error_name(error));
		rc = -1;
	}

	error = libusb_attach_kernel_driver(deviceHandle, LIGHTPACK_INTERFACE);
	if (error != LIBUSB_SUCCESS)
	{
		Debug(_log, "Error while attaching kernel driver (%d): %s", error, libusb_error_name(error));
		rc = -1;
	}

	libusb_close(deviceHandle);

	return rc;
}

QString LedDeviceLightpack::getProperty(libusb_device * device, int stringDescriptorIndex)
{
	QString value;

	if ( stringDescriptorIndex != 0 )
	{
		libusb_device_handle * handle = nullptr;
		if ( libusb_open(device, &handle) == LIBUSB_SUCCESS )
		{
			char buffer[256];
			int error = libusb_get_string_descriptor_ascii(handle, stringDescriptorIndex, reinterpret_cast<unsigned char *>(buffer), sizeof(buffer));
			if (error > 0)
			{
				value = QString(QByteArray(buffer, error));
			}
			libusb_close(handle);
		}
	}
	return value;
}
