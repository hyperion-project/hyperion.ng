
// STL includes
#include <cstring>

// Qt includes
#include <QTimer>

// Local Hyperion includes
#include "ProviderHID.h"

namespace {
	constexpr unsigned short DEFAULT_VENDOR_ID = 0x2341;
	constexpr unsigned short DEFAULT_PRODUCT_ID = 0x8036;

	constexpr int WRITE_DELAY_MS = 3000;

	unsigned short parseHexValue(const QJsonValue& value, const unsigned short fallback = 0)
	{
		bool ok = false;
		const unsigned short result = value.toString().toUShort(&ok, 0);
		return ok ? result : fallback;
	}
}

ProviderHID::ProviderHID(const QJsonObject& deviceConfig, const unsigned short vendorId, const unsigned short productId)
	: LedDevice(deviceConfig)
	, _vendorId(vendorId)
	, _productId(productId)
	, _useFeature(false)
	, _deviceHandle(nullptr)
	, _delayAfterConnect_ms(0)
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

QJsonObject ProviderHID::discover(const QJsonObject& params)
{
	const auto vendorId = _vendorId != 0 ? _vendorId : parseHexValue(params.value("VID"));
	const auto productId = _productId != 0 ? _productId : parseHexValue(params.value("PID"));
	const auto usagePage = parseHexValue(params.value("usagePage"));
	const auto usage = parseHexValue(params.value("usage"));

	QJsonObject devicesDiscovered;
	devicesDiscovered.insert("ledDeviceType", _activeDeviceType );
	devicesDiscovered.insert("devices", enumerateHidDevices(
		vendorId, productId, usagePage, usage, usagePage != 0 && usage != 0));
	return devicesDiscovered;
}

bool ProviderHID::init(const QJsonObject& deviceConfig)
{
	bool isInitOK = false;

	// Initialise sub-class
	if (LedDevice::init(deviceConfig))
	{
		_delayAfterConnect_ms = deviceConfig["delayAfterConnect"].toInt(0);
		_devicePath = deviceConfig["output"].toString();

		// Read configurable identifiers only when the subclass did not provide fixed values.
		_vendorId = _vendorId != 0 ? _vendorId : parseHexValue(
			deviceConfig.value("VID"), DEFAULT_VENDOR_ID);
		_productId = _productId != 0 ? _productId : parseHexValue(
			deviceConfig.value("PID"), DEFAULT_PRODUCT_ID);

		// Initialize the USB context
		if (hid_init() != 0)
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
	close();

	if (_devicePath.isEmpty())
	{
		Info(_log, "Opening device: VID %04hx PID %04hx", _vendorId, _productId);
		_deviceHandle = hid_open(_vendorId, _productId, nullptr);
	}
	else
	{
		Info(_log, "Opening HID device at path '%s'", QSTRING_CSTR(_devicePath));
		const QByteArray encodedPath = _devicePath.toUtf8();
		_deviceHandle = hid_open_path(encodedPath.constData());
	}

	if (_deviceHandle == nullptr)
	{
		// Failed to open the device
		if (_devicePath.isEmpty())
		{
			setInError("Failed to open HID device. Maybe your PID/VID setting is wrong? Make sure to add a udev rule/use sudo.");
		}
		else
		{
			setInError(QStringLiteral("Failed to open HID device at path '%1'. Check the selected device and permissions.")
				.arg(_devicePath));
		}

		if (leddevice_properties().isDebugEnabled())
		{
			// http://www.signal11.us/oss/hidapi/
			qCDebug(leddevice_properties) << "HIDAPI Device List:";
			const auto devices = hid_enumerate(0x00, 0x00);
			for (auto current = devices; current != nullptr; current = current->next)
			{
				qCDebug(leddevice_properties) << "Device Found";
				qCDebug(leddevice_properties) << "  type:"
												<< formatHexValue(current->vendor_id)
												<< formatHexValue(current->product_id);
				qCDebug(leddevice_properties) << "  path:"
					<< (current->path != nullptr ? current->path : "NULL");
				qCDebug(leddevice_properties) << "  serial_number:"
					<< QString::fromWCharArray(current->serial_number);
				qCDebug(leddevice_properties) << "  Manufacturer:"
					<< QString::fromWCharArray(current->manufacturer_string);
				qCDebug(leddevice_properties) << "  Product:"
					<< QString::fromWCharArray(current->product_string);
			}
			hid_free_enumeration(devices);
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

int ProviderHID::writeBytes(const unsigned size, const uint8_t* data)
{
	if (_blockedForDelay) 
	{
		return 0;
	}

	if (_deviceHandle == nullptr)
	{
		// try to reopen
		const auto status = open();
		if (status < 0){
			// Try again in 3 seconds
			_blockedForDelay = true;
			QTimer::singleShot(WRITE_DELAY_MS, this, &ProviderHID::unblockAfterDelay );
			Debug(_log,"Device blocked for %d ms", WRITE_DELAY_MS);
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
	if (_useFeature){
		ret = hid_send_feature_report(_deviceHandle, ledData.data(), size + 1);
	}
	else
	{
		ret = hid_write(_deviceHandle, ledData.data(), size + 1);
	}

	// Handle first error
	if (ret < 0)
	{
		Error(_log, "Failed to write to HID device.");

		// Try again
		if (_useFeature)
		{
			ret = hid_send_feature_report(_deviceHandle, ledData.data(), size + 1);
		}
		else
		{
			ret = hid_write(_deviceHandle, ledData.data(), size + 1);
		}

		// Writing failed again, device might have disconnected
		if (ret < 0){
			Error(_log,"Failed to write to HID device.");

			hid_close(_deviceHandle);
			_deviceHandle = nullptr;
		}
	}
	return ret;
}

QString ProviderHID::formatHexValue(const unsigned short value)
{
	return QStringLiteral("0x%1").arg(value, 4, 16, QLatin1Char('0'));
}

QJsonObject ProviderHID::hidDeviceInfoToJson(const hid_device_info& deviceInfo)
{
	QJsonObject properties;
	properties.insert("manufacturer", QString::fromWCharArray(deviceInfo.manufacturer_string));
	properties.insert("product", QString::fromWCharArray(deviceInfo.product_string));
	properties.insert("serialNumber", QString::fromWCharArray(deviceInfo.serial_number));
	properties.insert("path", deviceInfo.path != nullptr ? QString::fromUtf8(deviceInfo.path) : QString());
	properties.insert("vendorIdentifier", formatHexValue(deviceInfo.vendor_id));
	properties.insert("productIdentifier", formatHexValue(deviceInfo.product_id));
	properties.insert("release_number", formatHexValue(deviceInfo.release_number));
	properties.insert("usage_page", formatHexValue(deviceInfo.usage_page));
	properties.insert("usage", formatHexValue(deviceInfo.usage));
	properties.insert("interface_number", deviceInfo.interface_number);
	return properties;
}

QJsonArray ProviderHID::enumerateHidDevices(
	const unsigned short vendorId, const unsigned short productId,
	const unsigned short usagePage, const unsigned short usage, const bool filterByUsage)
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

		deviceList.append(hidDeviceInfoToJson(*current));
	}

	hid_free_enumeration(devices);
	return deviceList;
}

void ProviderHID::unblockAfterDelay()
{
	Debug(_log, "Device unblocked");
	_blockedForDelay = false;
}
