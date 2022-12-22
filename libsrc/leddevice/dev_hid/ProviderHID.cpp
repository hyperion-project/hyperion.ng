
// STL includes
#include <cstring>
#include <iostream>

// Qt includes
#include <QTimer>

// Local Hyperion includes
#include "ProviderHID.h"

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
		_VendorId = std::stoul(VendorIdString, nullptr, 16);
		_ProductId = std::stoul(ProductIdString, nullptr, 16);

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
	int retval = -1;
	_isDeviceReady = false;

	// Open the device
	Info(_log, "Opening device: VID %04hx PID %04hx\n", _VendorId, _ProductId);
	_deviceHandle = hid_open(_VendorId, _ProductId, nullptr);

	if (_deviceHandle == nullptr)
	{
		// Failed to open the device
		this->setInError( "Failed to open HID device. Maybe your PID/VID setting is wrong? Make sure to add a udev rule/use sudo." );

#if 0
		// http://www.signal11.us/oss/hidapi/
				std::cout << "Showing a list of all available HID devices:" << std::endl;
				auto devs = hid_enumerate(0x00, 0x00);
				auto cur_dev = devs;
				while (cur_dev) {
					printf("Device Found\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls",
						cur_dev->vendor_id, cur_dev->product_id, cur_dev->path, cur_dev->serial_number);
					printf("\n");
					printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
					printf("  Product:      %ls\n", cur_dev->product_string);
					printf("\n");
					cur_dev = cur_dev->next;
				}
				hid_free_enumeration(devs);
#endif

	}
	else
	{
		Info(_log,"Opened HID device successful");
		// Everything is OK -> enable device
		_isDeviceReady = true;
		retval = 0;
	}

	// Wait after device got opened if enabled
	if (_delayAfterConnect_ms > 0)
	{
		_blockedForDelay = true;
		QTimer::singleShot(_delayAfterConnect_ms, this, &ProviderHID::unblockAfterDelay );
		Debug(_log, "Device blocked for %d  ms", _delayAfterConnect_ms);
	}

	return retval;
}

int ProviderHID::close()
{
	int retval = 0;
	_isDeviceReady = false;

	// LedDevice specific closing activities
	if (_deviceHandle != nullptr)
	{
		hid_close(_deviceHandle);
		_deviceHandle = nullptr;
	}
	return retval;
}

int ProviderHID::writeBytes(unsigned size, const uint8_t * data)
{
	if (_blockedForDelay) {
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
	uint8_t ledData[size + 1];
	ledData[0] = 0; // Report ID
	memcpy(ledData + 1, data, size_t(size));

	// Send data via feature or out report
	int ret;
	if(_useFeature){
		ret = hid_send_feature_report(_deviceHandle, ledData, size + 1);
	}
	else{
		ret = hid_write(_deviceHandle, ledData, size + 1);
	}

	// Handle first error
	if(ret < 0)
	{
		Error(_log,"Failed to write to HID device.");

		// Try again
		if(_useFeature)
		{
			ret = hid_send_feature_report(_deviceHandle, ledData, size + 1);
		}
		else
		{
			ret = hid_write(_deviceHandle, ledData, size + 1);
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

	QJsonArray deviceList;

	// Discover HID Devices
	auto devs = hid_enumerate(0x00, 0x00);

	if ( devs != nullptr )
	{
		auto cur_dev = devs;
		while (cur_dev)
		{
			QJsonObject deviceInfo;
			deviceInfo.insert("manufacturer",QString::fromWCharArray(cur_dev->manufacturer_string));
			deviceInfo.insert("path",cur_dev->path);
			deviceInfo.insert("productIdentifier", QString("0x%1").arg(static_cast<ushort>(cur_dev->product_id),0,16));
			deviceInfo.insert("release_number",QString("0x%1").arg(static_cast<ushort>(cur_dev->release_number),0,16));
			deviceInfo.insert("serialNumber",QString::fromWCharArray(cur_dev->serial_number));
			deviceInfo.insert("usage_page", QString("0x%1").arg(static_cast<ushort>(cur_dev->usage_page),0,16));
			deviceInfo.insert("vendorIdentifier", QString("0x%1").arg(static_cast<ushort>(cur_dev->vendor_id),0,16));
			deviceInfo.insert("interface_number",cur_dev->interface_number);
			deviceList.append(deviceInfo);

			cur_dev = cur_dev->next;
		}
		hid_free_enumeration(devs);
	}

	devicesDiscovered.insert("devices", deviceList);
	return devicesDiscovered;
}
