// code currently disabled. must be ported to new structure
#if 0
// stl includes
#include <exception>
#include <cstring>
#include <wchar.h>

// Local Hyperion includes
#include "LedDeviceLightpack-hidapi.h"

// from USB_ID.h (http://code.google.com/p/light-pack/source/browse/CommonHeaders/USB_ID.h)
#define USB_OLD_VENDOR_ID  0x03EB
#define USB_OLD_PRODUCT_ID 0x204F
#define USB_VENDOR_ID      0x1D50
#define USB_PRODUCT_ID     0x6022

#define LIGHTPACK_INTERFACE 0

// from commands.h (http://code.google.com/p/light-pack/source/browse/CommonHeaders/commands.h)
// Commands to device, sends it in first byte of data[]
enum COMMANDS {
	CMD_UPDATE_LEDS = 1,
	CMD_OFF_ALL,
	CMD_SET_TIMER_OPTIONS,
	CMD_SET_PWM_LEVEL_MAX_VALUE, /* deprecated */
	CMD_SET_SMOOTH_SLOWDOWN,
	CMD_SET_BRIGHTNESS,

	CMD_NOP = 0x0F
};

// from commands.h (http://code.google.com/p/light-pack/source/browse/CommonHeaders/commands.h)
enum DATA_VERSION_INDEXES {
	INDEX_FW_VER_MAJOR = 1,
	INDEX_FW_VER_MINOR
};

LedDeviceLightpackHidapi::LedDeviceLightpackHidapi()
	: LedDevice()
	, _deviceHandle(nullptr)
	, _serialNumber("")
	, _firmwareVersion({-1,-1})
	, _bitsPerChannel(-1)
	, _hwLedCount(-1)
{
}

LedDeviceLightpackHidapi::~LedDeviceLightpackHidapi()
{
	if (_deviceHandle != nullptr)
	{
		hid_close(_deviceHandle);
		_deviceHandle = nullptr;
	}

	// TODO: Should be called to avoid memory leak, but only at the end of the application
	//hid_exit();
}

int LedDeviceLightpackHidapi::open(const std::string & serialNumber)
{
	// initialize the usb context
	int error = hid_init();
	if (error != 0)
	{
		Error(_log, "Error while initializing the hidapi context");
		return -1;
	}
	Info("Hidapi initialized");

	// retrieve the list of usb devices
	hid_device_info * deviceList = hid_enumerate(0x0, 0x0);

	// iterate the list of devices
	for (hid_device_info * deviceInfo = deviceList; deviceInfo != nullptr; deviceInfo = deviceInfo->next)
	{
		// try to open and initialize the device
		error = testAndOpen(deviceInfo, serialNumber);

		if (error == 0)
		{
			// a device was sucessfully opened. break from list
			break;
		}
	}

	// free the device list
	hid_free_enumeration(deviceList);

	if (_deviceHandle == nullptr)
	{
		if (_serialNumber.empty())
		{
			Error(_log, "No Lightpack device has been found");
		}
		else
		{
			Error(_log, "No Lightpack device has been found with serial %s", _serialNumber);
		}
	}

	return _deviceHandle == nullptr ? -1 : 0;
}

int LedDeviceLightpackHidapi::testAndOpen(hid_device_info *device, const std::string & requestedSerialNumber)
{
	if ((device->vendor_id == USB_VENDOR_ID && device->product_id == USB_PRODUCT_ID) ||
		(device->vendor_id == USB_OLD_VENDOR_ID && device->product_id == USB_OLD_PRODUCT_ID))
	{
		Debug(_log, "Found a lightpack device. Retrieving more information...");

		// get the serial number
		std::string serialNumber = "";
		if (device->serial_number != nullptr)
		{
			// the serial number needs to be converted to a char array instead of wchar
			size_t size = wcslen(device->serial_number);
			serialNumber.resize(size, '.');
			for (size_t i = 0; i < size; ++i)
			{
				int c = wctob(device->serial_number[i]);
				if (c != EOF)
				{
					serialNumber[i] = c;
				}
			}
		}
		else
		{
			Error(_log, "No serial number for Lightpack device");
		}

		Debug(_log, "Lightpack device found: path=%s serial=%s", device->path.c_str(), serialNumber.c_str());

		// check if this is the device we are looking for
		if (requestedSerialNumber.empty() || requestedSerialNumber == serialNumber)
		{
			// This is it!
			_deviceHandle = hid_open_path(device->path);

			if (_deviceHandle != nullptr)
			{
				_serialNumber = serialNumber;

				Info(_log, "Lightpack device successfully opened");

				// get the firmware version
				uint8_t buffer[256];
				buffer[0] = 0; // report id
				int error = hid_get_feature_report(_deviceHandle, buffer, sizeof(buffer));
				if (error < 4)
				{
					Error(_log, "Unable to retrieve firmware version number from Lightpack device");
				}
				else
				{
					_firmwareVersion.majorVersion = buffer[INDEX_FW_VER_MAJOR+1];
					_firmwareVersion.minorVersion = buffer[INDEX_FW_VER_MINOR+1];
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

				// set the led buffer size (repport id + command + 6 bytes per led)
				_ledBuffer = std::vector<uint8_t>(2 + _hwLedCount * 6, 0);
				_ledBuffer[0] = 0x0; // report id
				_ledBuffer[1] = CMD_UPDATE_LEDS;

				// return success
				Debug(_log,"Lightpack device opened: path=%s serial=%s version=%s.%s.%s", device->path.c_str(), _serialNumber.c_str(), _firmwareVersion.majorVersion.c_str(),  _firmwareVersion.minorVersion.c_str());
				return 0;
			}
			else
			{
				Warning(_log, "Unable to open Lightpack device. Searching for other device");
			}
		}
	}

	return -1;
}

int LedDeviceLightpack::write(const std::vector<ColorRgb> &ledValues)
{
	return write(ledValues.data(), _ledCount);
}

int LedDeviceLightpack::write(const ColorRgb * ledValues, int size)
{
	int count = qMin(_hwLedCount,size);
	for (int i=0; i<count; i++)
	{
		const ColorRgb & color = ledValues[i];

		// copy the most significant bits of the rgb values to the first three bytes
		// offset 1 to accomodate for the report id and command byte
		_ledBuffer[6*i+2] = color.red;
		_ledBuffer[6*i+3] = color.green;
		_ledBuffer[6*i+4] = color.blue;

		// leave the next three bytes on zero...
		// 12-bit values having zeros in the lowest 4 bits which is almost correct, but it saves extra
		// switches to determine what to do and some bit shuffling
	}

	int error = writeBytes(_ledBuffer.data(), _ledBuffer.size());
	return error >= 0 ? 0 : error;
}

int LedDeviceLightpackHidapi::switchOff()
{
	unsigned char buf[2] = {0x0, CMD_OFF_ALL};
	return writeBytes(buf, sizeof(buf)) == sizeof(buf);
}

const std::string &LedDeviceLightpackHidapi::getSerialNumber() const
{
	return _serialNumber;
}

int LedDeviceLightpackHidapi::writeBytes(uint8_t *data, int size)
{
//	std::cout << "Writing " << size << " bytes: ";
//	for (int i = 0; i < size ; ++i) printf("%02x ", data[i]);
//	std::cout << std::endl;

	int error = hid_send_feature_report(_deviceHandle, data, size);
	if (error == size)
	{
		return 0;
	}

	Error(_log, "Unable to write %d bytes to Lightpack device(%d)", size, error);
	return error;
}

int LedDeviceLightpackHidapi::disableSmoothing()
{
	unsigned char buf[2] = {CMD_SET_SMOOTH_SLOWDOWN, 0};
	return writeBytes(buf, sizeof(buf)) == sizeof(buf);
}
#endif
