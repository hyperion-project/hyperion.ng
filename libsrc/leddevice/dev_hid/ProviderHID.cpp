
// STL includes
#include <cstring>
#include <iostream>

// Qt includes
#include <QTimer>

// Local Hyperion includes
#include "ProviderHID.h"

namespace
{
	QString formatHexValue(unsigned short value)
	{
		return QStringLiteral("0x%1").arg(value, 4, 16, QLatin1Char('0'));
	}
}

QString ProviderHID::fromWide(const wchar_t* value)
{
	return value == nullptr ? QString() : QString::fromWCharArray(value);
}

ProviderHID::ProviderHID(const QJsonObject &deviceConfig)
	:   LedDevice(deviceConfig)
	  , _VendorId(0)
	  , _ProductId(0)
	  , _useFeature(false)
	  , _deviceHandle(nullptr)
	  , _delayAfterConnect_ms (0)
	  , _blockedForDelay(false)
{
}

ProviderHID::~ProviderHID()
{
	if (_deviceHandle != nullptr)
	{
		hid_close(_deviceHandle);
	}
	hid_exit();
}

bool ProviderHID::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = false;

	// Initialise sub-class
	if ( LedDevice::init(deviceConfig) )
	{
		_delayAfterConnect_ms = deviceConfig["delayAfterConnect"].toInt(0);
		auto VendorIdString   = deviceConfig["VID"].toString("0x2341").toStdString();
		auto ProductIdString  = deviceConfig["PID"].toString("0x8036").toStdString();

		// Convert HEX values to integer
		_VendorId = static_cast<unsigned short>(std::stoul(VendorIdString, nullptr, 16));
		_ProductId = static_cast<unsigned short>(std::stoul(ProductIdString, nullptr, 16));

		// Initialize the USB context
		if ( hid_init() != 0)
		{
			this->setInError("Error initializing the HIDAPI context");
			isInitOK = false;
		}
		else
		{
			Debug(_log,"HIDAPI initialized");
			isInitOK = true;
		}
	}
	return isInitOK;
}

int ProviderHID::open()
{
	_isDeviceReady = false;

	// Open the device
	Info(_log, "Opening device: VID %04hx PID %04hx\n", _VendorId, _ProductId);
	_deviceHandle = hid_open(_VendorId, _ProductId, nullptr);

	if (_deviceHandle == nullptr)
	{
		// Failed to open the device
		this->setInError("Failed to open HID device. Maybe your PID/VID setting is wrong? Make sure to add a udev rule/use sudo.");

		if (leddevice_properties().isDebugEnabled())
		{
			// http://www.signal11.us/oss/hidapi/
			qCDebug(leddevice_properties()) << "HIDAPI Device List:";
			auto devs = hid_enumerate(0x00, 0x00);
			auto cur_dev = devs;
			while (cur_dev)
			{
				qCDebug(leddevice_properties()) << "Device Found";
				qCDebug(leddevice_properties()) << "  type:"
												<< QString("%1").arg(cur_dev->vendor_id, 4, 16, QLatin1Char('0'))
												<< QString("%1").arg(cur_dev->product_id, 4, 16, QLatin1Char('0'));
				qCDebug(leddevice_properties()) << "  path:" << cur_dev->path;
				qCDebug(leddevice_properties()) << "  serial_number:" << fromWide(cur_dev->serial_number);
				qCDebug(leddevice_properties()) << "  Manufacturer:" << fromWide(cur_dev->manufacturer_string);
				qCDebug(leddevice_properties()) << "  Product:" << fromWide(cur_dev->product_string);

				cur_dev = cur_dev->next;
			}
			hid_free_enumeration(devs);
		}
		return -1;
	}

	Info(_log,"Opened HID device successful");
	// Everything is OK -> enable device
	_isDeviceReady = true;

	// Wait after device got opened if enabled
	if (_delayAfterConnect_ms > 0)
	{
		_blockedForDelay = true;
		QTimer::singleShot(_delayAfterConnect_ms, this, &ProviderHID::unblockAfterDelay );
		Debug(_log, "Device blocked for %d  ms", _delayAfterConnect_ms);
	}

	return 0;
}

int ProviderHID::close()
{
	_isDeviceReady = false;

	// LedDevice specific closing activities
	if (_deviceHandle != nullptr)
	{
		hid_close(_deviceHandle);
		_deviceHandle = nullptr;
	}
	return 0;
}

int ProviderHID::writeBytes(unsigned size, const uint8_t * data)
{
	if (_blockedForDelay) 
	{
		return 0;
	}

	if (_deviceHandle == nullptr)
	{
		// try to reopen
		auto status = open();
		if(status < 0){
			// Try again in 3 seconds
			int delay_ms = 3000;
			_blockedForDelay = true;
			QTimer::singleShot(delay_ms, this, &ProviderHID::unblockAfterDelay );
			Debug(_log,"Device blocked for %d ms", delay_ms);
		}
		// Return here, to not write led data if the device should be blocked after connect
		return status;
	}

	// Prepend report ID to the buffer
	std::vector<uint8_t> ledData(size + 1);
	ledData[0] = 0; // Report ID
	std::memcpy(ledData.data() + 1, data, size);

	// Send data via feature or out report
	int ret;
	if(_useFeature){
		ret = hid_send_feature_report(_deviceHandle, ledData.data(), size + 1);
	}
	else{
		ret = hid_write(_deviceHandle, ledData.data(), size + 1);
	}

	// Handle first error
	if(ret < 0)
	{
		Error(_log,"Failed to write to HID device.");

		// Try again
		if(_useFeature)
		{
			ret = hid_send_feature_report(_deviceHandle, ledData.data(), size + 1);
		}
		else
		{
			ret = hid_write(_deviceHandle, ledData.data(), size + 1);
		}

		// Writing failed again, device might have disconnected
		if(ret < 0){
			Error(_log,"Failed to write to HID device.");

			hid_close(_deviceHandle);
			_deviceHandle = nullptr;
		}
	}
	return ret;
}

void ProviderHID::unblockAfterDelay()
{
	Debug(_log,"Device unblocked");
	_blockedForDelay = false;
}

QJsonObject ProviderHID::discover(const QJsonObject& /*params*/)
{
	QJsonObject devicesDiscovered;
	devicesDiscovered.insert("ledDeviceType", _activeDeviceType );
	devicesDiscovered.insert("devices", enumerateHidDevices());
	return devicesDiscovered;
}

QJsonArray ProviderHID::enumerateHidDevices(unsigned short vendorId, unsigned short productId) const
{
	return enumerateHidDevices(vendorId, productId, 0, 0, false);
}

QJsonArray ProviderHID::enumerateHidDevices(
	unsigned short vendorId, unsigned short productId,
	unsigned short usagePage, unsigned short usage) const
{
	return enumerateHidDevices(vendorId, productId, usagePage, usage, true);
}

QJsonArray ProviderHID::enumerateHidDevices(
	unsigned short vendorId, unsigned short productId,
	unsigned short usagePage, unsigned short usage, bool filterByUsage) const
{
	QJsonArray deviceList;
	hid_device_info* devices = hid_enumerate(vendorId, productId);

	for (const hid_device_info* current = devices; current != nullptr; current = current->next)
	{
		if (current->path == nullptr ||
			(filterByUsage && (current->usage_page != usagePage || current->usage != usage)))
		{
			continue;
		}

		QJsonObject device;
		device.insert("manufacturer", fromWide(current->manufacturer_string));
		device.insert("product", fromWide(current->product_string));
		device.insert("serialNumber", fromWide(current->serial_number));
		device.insert("path", QString::fromUtf8(current->path));
		device.insert("vendorIdentifier", formatHexValue(current->vendor_id));
		device.insert("productIdentifier", formatHexValue(current->product_id));
		device.insert("release_number", formatHexValue(current->release_number));
		device.insert("usage_page", formatHexValue(current->usage_page));
		device.insert("usage", formatHexValue(current->usage));
		device.insert("interface_number", current->interface_number);
		deviceList.append(device);
	}

	hid_free_enumeration(devices);
	return deviceList;
}
