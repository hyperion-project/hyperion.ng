#include "LedDeviceRobobloq.h"

#include <QByteArray>
#include <QJsonArray>
#include <QStringList>
#include <QtGlobal>

#include <algorithm>
#include <limits>
#include <utility>

namespace
{
	constexpr unsigned short VENDOR_ID = 0x1A86;
	constexpr unsigned short PRODUCT_ID = 0xFE07;
	constexpr unsigned short USAGE_PAGE = 0xFF00;
	constexpr unsigned short USAGE = 0x0001;
	constexpr int HID_REPORT_SIZE = 64;
	constexpr int MAX_LED_COUNT = 254;
	constexpr int DEVICE_INFO_ATTEMPTS = 3;
	constexpr int DEVICE_INFO_READ_ATTEMPTS = 3;
	constexpr int DEVICE_INFO_READ_TIMEOUT_MS = 200;
	// Reverse-engineered clients start at 2. RB and SC share one uint8_t
	// sequence, whose wrap from 255 to 0 is intentional.
	constexpr uint8_t INITIAL_MESSAGE_ID = 2;

	constexpr uint8_t ACTION_SET_SYNC_SCREEN = 0x80;
	constexpr uint8_t ACTION_READ_DEVICE_INFO = 0x82;
	constexpr uint8_t ACTION_SET_COLOR_RANGES = 0x86;
	constexpr uint8_t ACTION_SET_BRIGHTNESS = 0x87;
	constexpr uint8_t ACTION_SET_OPEN_URL = 0x93;
	constexpr uint8_t ACTION_TURN_OFF_LIGHT = 0x97;
}

LedDeviceRobobloq::LedDeviceRobobloq(const QJsonObject& deviceConfig)
	: ProviderHID(deviceConfig, VENDOR_ID, PRODUCT_ID, USAGE_PAGE, USAGE)
	, _hardwareBrightness(255)
	, _nextMessageId(INITIAL_MESSAGE_ID)
{
}

LedDevice* LedDeviceRobobloq::construct(const QJsonObject& deviceConfig)
{
	return new LedDeviceRobobloq(deviceConfig);
}

QJsonObject LedDeviceRobobloq::discover(const QJsonObject& params)
{
	QJsonObject result = ProviderHID::discover(params);
	QJsonArray enrichedDevices;
	for (const auto device : result["devices"].toArray())
	{
		enrichedDevices.append(buildDeviceProperties(device.toObject()));
	}

	result.insert("devices", enrichedDevices);
	return result;
}

QJsonObject LedDeviceRobobloq::getProperties(const QJsonObject& params)
{
	QJsonObject result;
	QJsonObject properties;

	if (const QString devicePath = params["output"].toString(); !devicePath.isEmpty())
	{
		const QByteArray encodedPath = devicePath.toUtf8();
		if (hid_device* handle = hid_open_path(encodedPath.constData()); handle != nullptr)
		{
			if (const hid_device_info* deviceInfo = hid_get_device_info(handle);
				deviceInfo != nullptr && isExpectedDevice(*deviceInfo))
			{
				properties = buildDeviceProperties(hidDeviceInfoToJson(*deviceInfo), handle);
			}
			hid_close(handle);
		}
	}

	result.insert("properties", properties);
	return result;
}

bool LedDeviceRobobloq::init(const QJsonObject& deviceConfig)
{
	if (!ProviderHID::init(deviceConfig))
	{
		return false;
	}

	if (_devicePath.isEmpty())
	{
		setInError(QStringLiteral("No Robobloq HID device selected"), false);
		return false;
	}

	_hardwareBrightness = qBound(1, deviceConfig["hardwareBrightness"].toInt(255), 255);
	_nextMessageId = INITIAL_MESSAGE_ID;

	if (_ledCount == 0 || _ledCount > MAX_LED_COUNT)
	{
		setInError(
			QStringLiteral("Robobloq RB/SC protocol supports 1 to %1 LEDs; configured hardwareLedCount is %2")
				.arg(MAX_LED_COUNT)
				.arg(_ledCount),
			false);
		return false;
	}

	Info(_log, "Robobloq init: LEDs=%u, HID path='%s', hardware brightness=%d",
		_ledCount, QSTRING_CSTR(_devicePath), _hardwareBrightness);
	return true;
}

int LedDeviceRobobloq::open()
{
	if (ProviderHID::open() < 0)
	{
		return -1;
	}

	const hid_device_info* hidInfo = hid_get_device_info(_deviceHandle);
	if (hidInfo == nullptr || !isExpectedDevice(*hidInfo))
	{
		setInError(
			QStringLiteral("The selected HID path does not identify the Robobloq protocol collection "
				"(%1:%2, usage %3:%4).")
				.arg(formatHexValue(VENDOR_ID), formatHexValue(PRODUCT_ID),
					formatHexValue(USAGE_PAGE), formatHexValue(USAGE)));
		ProviderHID::close();
		return -1;
	}

	Info(_log, "Opened Robobloq HID device: product='%s', serial='%s', interface=%d, usage=%04hx:%04hx",
		QSTRING_CSTR(QString::fromWCharArray(hidInfo->product_string)),
		QSTRING_CSTR(QString::fromWCharArray(hidInfo->serial_number)),
		hidInfo->interface_number, hidInfo->usage_page, hidInfo->usage);

	// Device metadata is optional; keep using the configured LED count if it cannot be read
	DeviceInfo deviceInfo;
	if (readDeviceInfo(_deviceHandle, _nextMessageId, deviceInfo))
	{
		Info(_log, "Robobloq device info: size=%d\", LEDs=%d, firmware=%s, deviceId=%s, uuid=%s",
			static_cast<int>(deviceInfo.physicalSize), static_cast<int>(deviceInfo.hardwareLedCount),
			QSTRING_CSTR(deviceInfo.firmwareVersion),
			QSTRING_CSTR(deviceInfo.deviceId), QSTRING_CSTR(deviceInfo.uuid));

		if (deviceInfo.hardwareLedCount > 0 && deviceInfo.hardwareLedCount != _ledCount)
		{
			Warning(_log, "Robobloq LED count mismatch: configured=%u, device=%d",
				_ledCount, static_cast<int>(deviceInfo.hardwareLedCount));
		}
	}
	else
	{
		Warning(_log, "Could not read Robobloq device info; using the configured LED count");
	}

	// Disable the device's URL shortcut before starting screen synchronization.
	if (sendRb(ACTION_SET_OPEN_URL, {0}) < 0)
	{
		Warning(_log, "Failed to disable the Robobloq URL shortcut");
		if (_deviceHandle == nullptr)
		{
			setInError(QStringLiteral("Robobloq HID connection was lost while disabling the URL shortcut"));
			return -1;
		}
	}

	return 0;
}

int LedDeviceRobobloq::write(const QVector<ColorRgb>& ledValues)
{
	if (ledValues.isEmpty())
	{
		return 0;
	}

	if (ledValues.size() > MAX_LED_COUNT)
	{
		Error(_log, "Robobloq frame has %d LEDs; the protocol supports at most %d",
			static_cast<int>(ledValues.size()), MAX_LED_COUNT);
		return -1;
	}

	return sendSc(ACTION_SET_SYNC_SCREEN, encodeColors(ledValues));
}

bool LedDeviceRobobloq::powerOn()
{
	if (!LedDevice::powerOn())
	{
		return false;
	}

	if (sendRb(ACTION_SET_BRIGHTNESS, {static_cast<uint8_t>(_hardwareBrightness)}) < 0)
	{
		Warning(_log, "Failed to set Robobloq hardware brightness to %d", _hardwareBrightness);
		if (_deviceHandle == nullptr)
		{
			setInError(QStringLiteral("Robobloq HID connection was lost while setting hardware brightness"));
		}
		return false;
	}

	return true;
}

bool LedDeviceRobobloq::powerOff()
{
	trackDevice(leddevice_flow, "Power OFF")
		<< ", is device ready: " << (_isDeviceReady ? "YES" : "NO");

	if (_isStayOnAfterStreaming)
	{
		return true;
	}

	if (!_isDeviceReady)
	{
		return false;
	}

	// The protocol-specific shutdown stops the current effect and clears both
	// the configured LEDs and the remaining addressable range through 0xFE.
	const bool effectStopped = sendRb(ACTION_TURN_OFF_LIGHT) == 0;
	const auto lastLed = static_cast<uint8_t>(_ledCount);
	QVector<uint8_t> blackRanges{1, 0, 0, 0, lastLed};
	if (lastLed < MAX_LED_COUNT)
	{
		blackRanges.append({static_cast<uint8_t>(lastLed + 1), 0, 0, 0, MAX_LED_COUNT});
	}
	const bool colorsCleared = sendRb(ACTION_SET_COLOR_RANGES, blackRanges) == 0;
	return effectStopped && colorsCleared;
}

QJsonObject LedDeviceRobobloq::DeviceInfo::toJson() const
{
	QJsonObject result;
	result.insert("physicalSize", physicalSize);
	result.insert("ledCount", hardwareLedCount);
	result.insert("firmwareVersion", firmwareVersion);
	result.insert("deviceId", deviceId);
	result.insert("uuid", uuid);
	return result;
}

int LedDeviceRobobloq::sendRb(const uint8_t action, const QVector<uint8_t>& payload)
{
	if (payload.size() > HID_REPORT_SIZE - 6)
	{
		Error(_log, "Robobloq RB payload is too large: %d bytes", static_cast<int>(payload.size()));
		return -1;
	}

	const QVector<uint8_t> message = createRbMessage(action, payload, allocateMessageId(_nextMessageId));
	return writeBytes(static_cast<unsigned>(message.size()), message.constData()) < 0 ? -1 : 0;
}

int LedDeviceRobobloq::sendSc(const uint8_t action, const QVector<uint8_t>& payload)
{
	const auto logicalLength = 7 + payload.size();
	if (logicalLength > std::numeric_limits<uint16_t>::max())
	{
		Error(_log, "Robobloq SC frame is too large: %d bytes", static_cast<int>(logicalLength));
		return -1;
	}

	const auto paddedReportSize = ((logicalLength + HID_REPORT_SIZE - 1) / HID_REPORT_SIZE) * HID_REPORT_SIZE;
	QVector<uint8_t> message(paddedReportSize, 0);
	// SC: magic, big-endian uint16 length, message ID, action, payload, checksum.
	message[0] = 'S';
	message[1] = 'C';
	message[2] = static_cast<uint8_t>((logicalLength >> 8) & 0xFF);
	message[3] = static_cast<uint8_t>(logicalLength & 0xFF);
	message[4] = allocateMessageId(_nextMessageId);
	message[5] = action;
	std::copy(payload.cbegin(), payload.cend(), message.begin() + 6);
	message[logicalLength - 1] = checksum(message.constData(), static_cast<int>(logicalLength - 1));

	for (int offset = 0; offset < message.size(); offset += HID_REPORT_SIZE)
	{
		if (writeBytes(HID_REPORT_SIZE, message.constData() + offset) < 0)
		{
			return -1;
		}
	}

	return 0;
}

QJsonObject LedDeviceRobobloq::buildDeviceProperties(QJsonObject properties)
{
	const QByteArray encodedPath = properties["path"].toString().toUtf8();
	if (hid_device* handle = hid_open_path(encodedPath.constData()); handle != nullptr)
	{
		properties = buildDeviceProperties(std::move(properties), handle);
		hid_close(handle);
	}
	else
	{
		properties.insert("displayName", formatDisplayName(properties));
	}
	return properties;
}

QJsonObject LedDeviceRobobloq::buildDeviceProperties(QJsonObject properties, hid_device* handle)
{
	if (handle != nullptr)
	{
		uint8_t nextMessageId = INITIAL_MESSAGE_ID;
		if (DeviceInfo deviceInfo; readDeviceInfo(handle, nextMessageId, deviceInfo))
		{
			const QJsonObject deviceProperties = deviceInfo.toJson();
			for (auto property = deviceProperties.constBegin(); property != deviceProperties.constEnd(); ++property)
			{
				properties.insert(property.key(), property.value());
			}
		}
	}

	properties.insert("displayName", formatDisplayName(properties));
	return properties;
}

bool LedDeviceRobobloq::readDeviceInfo(hid_device* handle, uint8_t& nextMessageId, DeviceInfo& deviceInfo)
{
	if (handle == nullptr)
	{
		return false;
	}

	uint8_t staleReport[HID_REPORT_SIZE + 1] = {};
	while (hid_read_timeout(handle, staleReport, sizeof(staleReport), 0) > 0)
	{
		// Discard queued reports before sending a new device-info request.
	}

	QVector<uint8_t> requestIds;
	requestIds.reserve(DEVICE_INFO_ATTEMPTS);

	for (int attempt = 0; attempt < DEVICE_INFO_ATTEMPTS; ++attempt)
	{
		const uint8_t requestId = allocateMessageId(nextMessageId);
		requestIds.append(requestId);

		const QVector<uint8_t> request = createRbMessage(ACTION_READ_DEVICE_INFO, {}, requestId);
		QVector<uint8_t> outputReport(HID_REPORT_SIZE + 1, 0);
		std::copy(request.cbegin(), request.cend(), outputReport.begin() + 1);

		if (hid_write(handle, outputReport.constData(), outputReport.size()) < 0)
		{
			return false;
		}

		for (int readAttempt = 0; readAttempt < DEVICE_INFO_READ_ATTEMPTS; ++readAttempt)
		{
			uint8_t inputReport[HID_REPORT_SIZE + 1] = {};
			const int received = hid_read_timeout(
				handle, inputReport, sizeof(inputReport), DEVICE_INFO_READ_TIMEOUT_MS);

			if (received < 0)
			{
				return false;
			}
			if (received == 0)
			{
				break;
			}

			qCDebug(leddevice_properties).noquote()
				<< QStringLiteral("Robobloq device-info reply (%1 bytes): %2")
					.arg(received)
					.arg(uint8_t_to_hex_string(inputReport, received));

			DeviceInfo replyInfo;
			uint8_t replyId = 0;
			if (parseDeviceInfoReply(inputReport, received, replyInfo, replyId) && requestIds.contains(replyId))
			{
				deviceInfo = replyInfo;
				return true;
			}
		}
	}

	return false;
}

bool LedDeviceRobobloq::isExpectedDevice(const hid_device_info& deviceInfo)
{
	return deviceInfo.vendor_id == VENDOR_ID &&
		deviceInfo.product_id == PRODUCT_ID &&
		deviceInfo.usage_page == USAGE_PAGE &&
		deviceInfo.usage == USAGE;
}

bool LedDeviceRobobloq::parseDeviceInfoReply(const uint8_t* data, const int size, DeviceInfo& deviceInfo, uint8_t& messageId)
{
	// The shortest valid RB packet contains magic, length, ID, action, and checksum.
	if (data == nullptr || size < 6)
	{
		return false;
	}

	// HIDAPI backends may return the leading report ID (0x00) or start directly
	// with the RB packet. Offset all protocol fields when the report ID is present.
	int offset = 0;
	if (size >= 7 && data[0] == 0x00 && data[1] == 'R' && data[2] == 'B')
	{
		offset = 1;
	}

	// Validate the RB magic after the optional report ID.
	if (size - offset < 6 || data[offset] != 'R' || data[offset + 1] != 'B')
	{
		return false;
	}

	const int logicalLength = data[offset + 2];
	// A device-info reply is at least 24 logical bytes, must fit the received
	// report, and must echo the read-device-info action.
	if (logicalLength < 24 || logicalLength > size - offset || data[offset + 4] != ACTION_READ_DEVICE_INFO)
	{
		return false;
	}

	messageId = data[offset + 3];

	// The response layout is: device ID, size, reserved bytes, LED count,
	// UUID, reserved byte, and a three-byte firmware version.
	deviceInfo.deviceId = uint8_t_to_hex_string(data + offset + 5, 3);
	deviceInfo.physicalSize = data[offset + 8];
	deviceInfo.hardwareLedCount = data[offset + 11];
	deviceInfo.uuid = uint8_t_to_hex_string(data + offset + 12, 8);
	deviceInfo.firmwareVersion = QStringLiteral("%1.%2.%3")
		.arg(data[offset + 21])
		.arg(data[offset + 22])
		.arg(data[offset + 23]);
	return true;
}

uint8_t LedDeviceRobobloq::allocateMessageId(uint8_t& nextMessageId)
{
	// RB commands and SC frames consume the same modulo-256 sequence.
	const uint8_t messageId = nextMessageId;
	++nextMessageId;
	return messageId;
}

QVector<uint8_t> LedDeviceRobobloq::createRbMessage(const uint8_t action, const QVector<uint8_t>& payload, uint8_t messageId)
{
	const int logicalLength = 6 + static_cast<int>(payload.size());
	QVector<uint8_t> message(HID_REPORT_SIZE, 0);
	// RB: magic, uint8 length, message ID, action, payload, additive checksum.
	message[0] = 'R';
	message[1] = 'B';
	message[2] = static_cast<uint8_t>(logicalLength);
	message[3] = messageId;
	message[4] = action;
	std::copy(payload.cbegin(), payload.cend(), message.begin() + 5);
	message[logicalLength - 1] = checksum(message.constData(), logicalLength - 1);
	return message;
}

QVector<uint8_t> LedDeviceRobobloq::encodeColors(const QVector<ColorRgb>& ledValues)
{
	QVector<uint8_t> payload;
	payload.reserve(ledValues.size() * 5);

	for (int index = 0; index < ledValues.size(); ++index)
	{
		const ColorRgb& color = ledValues[index];
		const auto ledIndex = static_cast<uint8_t>(index + 1);

		payload.append(ledIndex);
		payload.append(color.red);
		payload.append(color.green);
		payload.append(color.blue);
		payload.append(ledIndex);
	}

	return payload;
}

uint8_t LedDeviceRobobloq::checksum(const uint8_t* data, const int size)
{
	uint32_t sum = 0;
	for (int index = 0; index < size; ++index)
	{
		sum += data[index];
	}
	return static_cast<uint8_t>(sum & 0xFFU);
}

QString LedDeviceRobobloq::formatDisplayName(const QJsonObject& properties)
{
	QStringList identifiers{
		formatHexValue(VENDOR_ID).mid(2),
		formatHexValue(PRODUCT_ID).mid(2)
	};
	if (const QString deviceId = properties["deviceId"].toString(); !deviceId.isEmpty())
	{
		identifiers.append(deviceId);
	}

	QString displayName = QStringLiteral("[%1]").arg(identifiers.join(QLatin1Char(':')));

	const int physicalSize = properties["physicalSize"].toInt();
	const int ledCount = properties["ledCount"].toInt();
	if (physicalSize > 0 && ledCount > 0)
	{
		displayName += QStringLiteral(" %1\", %2 LEDs").arg(physicalSize).arg(ledCount);
	}
	else
	{
		QString productName = properties["product"].toString();
		if (productName.isEmpty())
		{
			productName = properties["manufacturer"].toString();
		}
		if (productName.isEmpty())
		{
			productName = QStringLiteral("USB HID");
		}
		displayName += QLatin1Char(' ') + productName;
	}

	QStringList details;
	if (const QString uuid = properties["uuid"].toString(); !uuid.isEmpty())
	{
		details.append(QStringLiteral("UUID %1").arg(uuid));
	}
	if (const QString firmwareVersion = properties["firmwareVersion"].toString(); !firmwareVersion.isEmpty())
	{
		details.append(QStringLiteral("FW %1").arg(firmwareVersion));
	}
	if (const QString serialNumber = properties["serialNumber"].toString(); !serialNumber.isEmpty())
	{
		details.append(QStringLiteral("S/N %1").arg(serialNumber));
	}
	if (!details.isEmpty())
	{
		displayName += QStringLiteral(" (%1)").arg(details.join(QStringLiteral(", ")));
	}

	return displayName;
}
