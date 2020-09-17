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

LedDeviceMultiLightpack::LedDeviceMultiLightpack(const QJsonObject &deviceConfig)
	: LedDevice(deviceConfig)
	, _lightpacks()
{
}

LedDeviceMultiLightpack::~LedDeviceMultiLightpack()
{
	for (LedDeviceLightpack * device : _lightpacks)
	{
		delete device;
	}
}

LedDevice* LedDeviceMultiLightpack::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceMultiLightpack(deviceConfig);
}

bool LedDeviceMultiLightpack::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = false;

	// Initialise sub-class
	if ( LedDevice::init(deviceConfig) )
	{
		// retrieve a list with Lightpack serials
		QStringList serialList = getLightpackSerials();

		// sort the list of Lightpacks based on the serial to get a fixed order
		std::sort(_lightpacks.begin(), _lightpacks.end(), compareLightpacks);

		// open each Lightpack device
		for (auto serial : serialList)
		{
			QJsonObject devConfig;
			devConfig["serial"] = serial;
			devConfig["latchTime"] = deviceConfig["latchTime"];
			devConfig["rewriteTime"] = deviceConfig["rewriteTime"];

			LedDeviceLightpack * device = new LedDeviceLightpack(devConfig);

			device->start();
			if (device->open() == 0)
			{
				_lightpacks.push_back(device);
			}
			else
			{
				Error(_log, "Error while creating Lightpack device with serial %s", QSTRING_CSTR(serial));
				delete device;
			}
		}

		if (_lightpacks.empty())
		{
			//Warning(_log, "No Lightpack devices were found");
			QString errortext = QString ("No Lightpack devices were found");
			this->setInError(errortext);
			isInitOK = false;
		}
		else
		{
			Info(_log, "%d Lightpack devices were found", _lightpacks.size());
			isInitOK = true;
		}
	}
	return isInitOK;
}

int LedDeviceMultiLightpack::open()
{
	int retval = -1;
	_isDeviceReady = false;

	int lightsInError = 0;
	// open each Lightpack device
	for (LedDeviceLightpack * device : _lightpacks)
	{
		if (device->open() < 0)
		{
			Error( _log, "Failed to open [%s]", QSTRING_CSTR(device->getSerialNumber()) );
			++lightsInError;
		}
	}

	if ( lightsInError < static_cast<int>(_lightpacks.size()) )
	{
		// Everything is OK -> enable device
		_isDeviceReady = true;
		retval = 0;
	}
	else
	{
		this->setInError( "All Lightpacks failed to be opened!" );
	}
	return retval;
}

int LedDeviceMultiLightpack::close()
{
	_isDeviceReady = false;

	for (LedDeviceLightpack * device : _lightpacks)
	{
			device->close();
	}
	return 0;
}

int LedDeviceMultiLightpack::write(const std::vector<ColorRgb> &ledValues)
{
	const ColorRgb * data = ledValues.data();
	int size = ledValues.size();

	for (LedDeviceLightpack * device : _lightpacks)
	{
		int count = qMin(static_cast<int>( device->getLedCount()), size);

		if (count > 0)
		{
			if ( device->isOpen() )
			{
				device->write(data, count);
			}

			data += count;
			size -= count;
		}
		else
		{
			Warning(_log, "Unable to write data to Lightpack device: no more led data available");
		}
	}

	return 0;
}

bool LedDeviceMultiLightpack::powerOff()
{
	for (LedDeviceLightpack * device : _lightpacks)
	{
		if ( device->isOpen() )
		{
			device->powerOff();
		}
	}
	return true;
}

QStringList LedDeviceMultiLightpack::getLightpackSerials()
{
	QStringList serialList;
	Logger * log = Logger::getInstance("LedDevice");
	Debug(log, "Getting list of Lightpack serials");

	// initialize the USB context
	libusb_context * libusbContext;
	int error = libusb_init(&libusbContext);
	if (error != LIBUSB_SUCCESS)
	{
		Error(log,"Error while initializing USB context(%d): %s", error, libusb_error_name(error));
		libusbContext = nullptr;
		return serialList;
	}
	//libusb_set_debug(_libusbContext, 3);
	Info(log, "USB context initialized in multi Lightpack device");

	// retrieve the list of USB devices
	libusb_device ** deviceList;
	ssize_t deviceCount = libusb_get_device_list(libusbContext, &deviceList);

	// iterate the list of devices
	for (ssize_t i = 0 ; i < deviceCount; ++i)
	{
		libusb_device_descriptor deviceDescriptor;
		error = libusb_get_device_descriptor(deviceList[i], &deviceDescriptor);
		if (error != LIBUSB_SUCCESS)
		{
			Error(log, "Error while retrieving device descriptor(%d): %s", error, libusb_error_name(error));
			continue;
		}

		if ((deviceDescriptor.idVendor == USB_VENDOR_ID && deviceDescriptor.idProduct == USB_PRODUCT_ID) ||
			(deviceDescriptor.idVendor == USB_OLD_VENDOR_ID && deviceDescriptor.idProduct == USB_OLD_PRODUCT_ID))
		{
			Info(log, "Found a Lightpack device. Retrieving serial...");

			// get the serial number
			QString serialNumber;
			if (deviceDescriptor.iSerialNumber != 0)
			{
				// TODO: Check, if exceptions via try/catch need to be replaced in Qt environment
				try
				{
					serialNumber = LedDeviceMultiLightpack::getString(deviceList[i], deviceDescriptor.iSerialNumber);
				}
				catch (int e)
				{
					Error(log,"Unable to retrieve serial number(%d): %s", e, libusb_error_name(e));
					continue;
				}
			}

			Info(log, "Lightpack device found with serial %s", QSTRING_CSTR(serialNumber));
			serialList.append(serialNumber);
		}
	}

	// free the device list
	libusb_free_device_list(deviceList, 1);
	libusb_exit(libusbContext);

	return serialList;
}

QString LedDeviceMultiLightpack::getString(libusb_device * device, int stringDescriptorIndex)
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
