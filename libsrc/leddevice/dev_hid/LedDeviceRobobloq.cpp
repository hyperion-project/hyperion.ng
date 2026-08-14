#include "LedDeviceRobobloq.h"

#include <QByteArray>
#include <QJsonArray>
#include <QStringList>
#include <QtGlobal>

#include <algorithm>
#include <limits>

namespace
{
	constexpr unsigned short vendorId = 0x1A86;
	constexpr unsigned short productId = 0xFE07;
	constexpr unsigned short usagePage = 0xFF00;
	constexpr unsigned short usage = 0x0001;
	constexpr int hidReportSize = 64;
	constexpr int maxLedCount = 254;
	constexpr int deviceInfoAttempts = 3;
	constexpr int deviceInfoReadAttempts = 3;
	constexpr int deviceInfoReadTimeoutMs = 200;
	// Reverse-engineered clients start at 2. RB and SC share one uint8_t
	// sequence, whose wrap from 255 to 0 is intentional.
	constexpr uint8_t initialMessageId = 2;

	constexpr uint8_t actionSetSyncScreen = 0x80;
	constexpr uint8_t actionReadDeviceInfo = 0x82;
	constexpr uint8_t actionSetColorRanges = 0x86;
	constexpr uint8_t actionSetBrightness = 0x87;
	constexpr uint8_t actionSetOpenUrl = 0x93;
	constexpr uint8_t actionTurnOffLight = 0x97;

	QString bytesToHex(const uint8_t* data, const int size)
	{
		return QString::fromLatin1(QByteArray(reinterpret_cast<const char*>(data), size).toHex());
	}

	QString formatDisplayName(const QJsonObject& properties)
	{
		QStringList identifiers{
			QStringLiteral("%1").arg(vendorId, 4, 16, QLatin1Char('0')),
			QStringLiteral("%1").arg(productId, 4, 16, QLatin1Char('0'))
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
}

LedDeviceRobobloq::LedDeviceRobobloq(const QJsonObject& deviceConfig)
	: ProviderHID(deviceConfig)
	, _hardwareBrightness(255)
	, _nextMessageId(initialMessageId)
{
}

LedDevice* LedDeviceRobobloq::construct(const QJsonObject& deviceConfig)
{
	return new LedDeviceRobobloq(deviceConfig);
}

QJsonObject LedDeviceRobobloq::discover(const QJsonObject& /*params*/)
{
	QJsonObject result;
	result.insert("ledDeviceType", _activeDeviceType);

	QJsonArray deviceList = enumerateHidDevices(vendorId, productId, usagePage, usage);
	for (auto device : deviceList)
	{
		device = buildDeviceProperties(device.toObject());
	}

	result.insert("devices", deviceList);
	return result;
}

QJsonObject LedDeviceRobobloq::getProperties(const QJsonObject& params)
{
	QJsonObject result;
	QJsonObject properties;

	if (const QString devicePath = params["output"].toString(); !devicePath.isEmpty())
	{
		const QJsonArray devices = enumerateHidDevices(vendorId, productId, usagePage, usage);
		for (const auto& device : devices)
		{
			if (const auto deviceProperties = device.toObject(); deviceProperties["path"].toString() == devicePath)
			{
				properties = buildDeviceProperties(deviceProperties);
				break;
			}
		}
	}

	result.insert("properties", properties);
	return result;
}

bool LedDeviceRobobloq::init(const QJsonObject& deviceConfig)
{
	QJsonObject hidConfig(deviceConfig);
	hidConfig["VID"] = QStringLiteral("0x%1").arg(vendorId, 4, 16, QLatin1Char('0'));
	hidConfig["PID"] = QStringLiteral("0x%1").arg(productId, 4, 16, QLatin1Char('0'));

	if (!ProviderHID::init(hidConfig))
	{
		return false;
	}

	_devicePath = deviceConfig["output"].toString();
	if (_devicePath.isEmpty())
	{
		setInError(QStringLiteral("No Robobloq HID device selected"), false);
		return false;
	}

	_hardwareBrightness = qBound(1, deviceConfig["hardwareBrightness"].toInt(255), 255);
	_nextMessageId = initialMessageId;

	if (_ledCount == 0 || _ledCount > maxLedCount)
	{
		setInError(
			QStringLiteral("Robobloq RB/SC protocol supports 1 to %1 LEDs; configured hardwareLedCount is %2")
				.arg(maxLedCount)
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
	ProviderHID::close();

	Info(_log, "Opening Robobloq HID device at path '%s'", QSTRING_CSTR(_devicePath));

	// Locate the protocol HID collection matching the path selected during discovery
	hid_device_info* devices = hid_enumerate(vendorId, productId);
	QByteArray selectedPath;
	QString product;
	QString serialNumber;
	int candidateCount = 0;
	int protocolCollectionCount = 0;
	int interfaceNumber = 0;

	for (const hid_device_info* current = devices; current != nullptr; current = current->next)
	{
		++candidateCount;

		const QString path = current->path == nullptr ? QString() : QString::fromUtf8(current->path);
		Debug(_log,
			"Robobloq HID candidate: path='%s', usage=%04hx:%04hx, interface=%d, serial='%s', product='%s'",
			QSTRING_CSTR(path), current->usage_page, current->usage, current->interface_number,
			QSTRING_CSTR(fromWide(current->serial_number)), QSTRING_CSTR(fromWide(current->product_string)));

		if (current->usage_page != usagePage || current->usage != usage)
		{
			continue;
		}

		++protocolCollectionCount;
		if (current->path == nullptr || path != _devicePath)
		{
			continue;
		}

		selectedPath = QByteArray(current->path);
		product = fromWide(current->product_string);
		serialNumber = fromWide(current->serial_number);
		interfaceNumber = current->interface_number;
		break;
	}

	hid_free_enumeration(devices);

	if (selectedPath.isEmpty())
	{
		setInError(
			QStringLiteral(
				"No matching Robobloq HID collection found. HIDAPI saw %1 candidate(s) for 1a86:fe07, "
				"%2 with usage ff00:0001. Configured path='%3'. Check the selected device and permissions.")
				.arg(candidateCount)
				.arg(protocolCollectionCount)
				.arg(_devicePath));
		return -1;
	}

	_deviceHandle = hid_open_path(selectedPath.constData());
	if (_deviceHandle == nullptr)
	{
		setInError(
			QStringLiteral("Failed to open Robobloq HID path '%1'. Check device permissions.").arg(_devicePath));
		return -1;
	}

	_isDeviceReady = true;
	Info(_log, "Opened Robobloq HID device: product='%s', serial='%s', interface=%d, usage=ff00:0001",
		QSTRING_CSTR(product), QSTRING_CSTR(serialNumber), interfaceNumber);

	// Device metadata is optional; keep using the configured LED count if it cannot be read
	DeviceInfo deviceInfo;
	if (readDeviceInfo(_deviceHandle, _nextMessageId, deviceInfo))
	{
		Info(_log, "Robobloq device info: size=%d\", LEDs=%d, firmware=%s, deviceId=%s, uuid=%s",
			static_cast<int>(deviceInfo._physicalSize), static_cast<int>(deviceInfo._ledCount),
			QSTRING_CSTR(deviceInfo._firmwareVersion),
			QSTRING_CSTR(deviceInfo._deviceId), QSTRING_CSTR(deviceInfo._uuid));

		if (deviceInfo._ledCount > 0 && deviceInfo._ledCount != _ledCount)
		{
			Warning(_log, "Robobloq LED count mismatch: configured=%u, device=%d",
				_ledCount, static_cast<int>(deviceInfo._ledCount));
		}
	}
	else
	{
		Warning(_log, "Could not read Robobloq device info; using the configured LED count");
	}

	// Disable the device's URL shortcut before starting screen synchronization.
	if (sendRb(actionSetOpenUrl, {0}) < 0)
	{
		Warning(_log, "Failed to disable the Robobloq URL shortcut");
		if (_deviceHandle == nullptr)
		{
			setInError(QStringLiteral("Robobloq HID connection was lost while disabling the URL shortcut"));
			return -1;
		}
	}

	// Apply the configured hardware settings before accepting LED updates
	if (sendRb(actionSetBrightness, {static_cast<uint8_t>(_hardwareBrightness)}) < 0)
	{
		Warning(_log, "Failed to set Robobloq hardware brightness to %d", _hardwareBrightness);
		if (_deviceHandle == nullptr)
		{
			setInError(QStringLiteral("Robobloq HID connection was lost while setting hardware brightness"));
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

	if (ledValues.size() > maxLedCount)
	{
		Error(_log, "Robobloq frame has %d LEDs; the protocol supports at most %d",
			static_cast<int>(ledValues.size()), maxLedCount);
		return -1;
	}

	return sendSc(actionSetSyncScreen, encodeColors(ledValues));
}

bool LedDeviceRobobloq::powerOff()
{
	if (_isStayOnAfterStreaming)
	{
		return true;
	}

	const bool effectStopped = sendRb(actionTurnOffLight) == 0;
	const auto lastLed = static_cast<uint8_t>(_ledCount);
	QVector<uint8_t> blackRanges{1, 0, 0, 0, lastLed};
	if (lastLed < maxLedCount)
	{
		blackRanges.append({static_cast<uint8_t>(lastLed + 1), 0, 0, 0, maxLedCount});
	}
	const bool colorsCleared = sendRb(actionSetColorRanges, blackRanges) == 0;
	return effectStopped && colorsCleared;
}

QJsonObject LedDeviceRobobloq::buildDeviceProperties(QJsonObject properties) const
{
	const QByteArray encodedPath = properties["path"].toString().toUtf8();
	if (hid_device* handle = hid_open_path(encodedPath.constData()); handle != nullptr)
	{
		uint8_t nextMessageId = initialMessageId;
		if (DeviceInfo deviceInfo; readDeviceInfo(handle, nextMessageId, deviceInfo))
		{
			const QJsonObject deviceProperties = deviceInfo.toJson();
			for (auto property = deviceProperties.constBegin(); property != deviceProperties.constEnd(); ++property)
			{
				properties.insert(property.key(), property.value());
			}
		}
		hid_close(handle);
	}

	properties.insert("displayName", formatDisplayName(properties));
	return properties;
}

QJsonObject LedDeviceRobobloq::DeviceInfo::toJson() const
{
	QJsonObject result;
	result.insert("physicalSize", _physicalSize);
	result.insert("ledCount", _ledCount);
	result.insert("firmwareVersion", _firmwareVersion);
	result.insert("deviceId", _deviceId);
	result.insert("uuid", _uuid);
	return result;
}

bool LedDeviceRobobloq::readDeviceInfo(hid_device* handle, uint8_t& nextMessageId, DeviceInfo& deviceInfo) const
{
	if (handle == nullptr)
	{
		return false;
	}

	uint8_t staleReport[hidReportSize + 1] = {};
	while (hid_read_timeout(handle, staleReport, sizeof(staleReport), 0) > 0)
	{
	}

	QVector<uint8_t> requestIds;
	requestIds.reserve(deviceInfoAttempts);

	for (int attempt = 0; attempt < deviceInfoAttempts; ++attempt)
	{
		const uint8_t requestId = allocateMessageId(nextMessageId);
		requestIds.append(requestId);

		const QVector<uint8_t> request = padToHidReports(
			createRbMessage(actionReadDeviceInfo, {}, requestId));
		QVector<uint8_t> outputReport(hidReportSize + 1, 0);
		std::copy(request.cbegin(), request.cend(), outputReport.begin() + 1);

		if (hid_write(handle, outputReport.constData(), outputReport.size()) < 0)
		{
			return false;
		}

		for (int readAttempt = 0; readAttempt < deviceInfoReadAttempts; ++readAttempt)
		{
			uint8_t inputReport[hidReportSize + 1] = {};
			const int received = hid_read_timeout(
				handle, inputReport, sizeof(inputReport), deviceInfoReadTimeoutMs);

			if (received < 0)
			{
				return false;
			}
			if (received == 0)
			{
				break;
			}

			Debug(_log, "Robobloq device-info reply (%d bytes): %s",
				received, QSTRING_CSTR(bytesToHex(inputReport, received)));

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

int LedDeviceRobobloq::sendRb(uint8_t action, const QVector<uint8_t>& payload)
{
	if (payload.size() > hidReportSize - 6)
	{
		Error(_log, "Robobloq RB payload is too large: %d bytes", static_cast<int>(payload.size()));
		return -1;
	}

	const QVector<uint8_t> message = padToHidReports(
		createRbMessage(action, payload, allocateMessageId(_nextMessageId)));
	return writeBytes(static_cast<unsigned>(message.size()), message.constData()) < 0 ? -1 : 0;
}

int LedDeviceRobobloq::sendSc(uint8_t action, const QVector<uint8_t>& payload)
{
	const int logicalLength = 7 + payload.size();
	if (logicalLength > std::numeric_limits<uint16_t>::max())
	{
		Error(_log, "Robobloq SC frame is too large: %d bytes", logicalLength);
		return -1;
	}

	QVector<uint8_t> message(logicalLength, 0);
	// SC: magic, big-endian uint16 length, message ID, action, payload, checksum.
	message[0] = 'S';
	message[1] = 'C';
	message[2] = static_cast<uint8_t>((logicalLength >> 8) & 0xFF);
	message[3] = static_cast<uint8_t>(logicalLength & 0xFF);
	message[4] = allocateMessageId(_nextMessageId);
	message[5] = action;
	std::copy(payload.cbegin(), payload.cend(), message.begin() + 6);
	message[logicalLength - 1] = checksum(message.constData(), logicalLength - 1);
	const QVector<uint8_t> reports = padToHidReports(message);

	for (int offset = 0; offset < reports.size(); offset += hidReportSize)
	{
		if (writeBytes(hidReportSize, reports.constData() + offset) < 0)
		{
			return -1;
		}
	}

	return 0;
}

bool LedDeviceRobobloq::parseDeviceInfoReply(const uint8_t* data, int size, DeviceInfo& deviceInfo, uint8_t& messageId)
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
	if (logicalLength < 24 || logicalLength > size - offset || data[offset + 4] != actionReadDeviceInfo)
	{
		return false;
	}

	messageId = data[offset + 3];

	// The response layout is: device ID, size, reserved bytes, LED count,
	// UUID, reserved byte, and a three-byte firmware version.
	deviceInfo._deviceId = bytesToHex(data + offset + 5, 3);
	deviceInfo._physicalSize = data[offset + 8];
	deviceInfo._ledCount = data[offset + 11];
	deviceInfo._uuid = bytesToHex(data + offset + 12, 8);
	deviceInfo._firmwareVersion = QStringLiteral("%1.%2.%3")
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

QVector<uint8_t> LedDeviceRobobloq::createRbMessage(uint8_t action, const QVector<uint8_t>& payload, uint8_t messageId)
{
	const int logicalLength = 6 + static_cast<int>(payload.size());
	QVector<uint8_t> message(logicalLength, 0);
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

QVector<uint8_t> LedDeviceRobobloq::padToHidReports(const QVector<uint8_t>& message)
{
	const int paddedSize = ((static_cast<int>(message.size()) + hidReportSize - 1) / hidReportSize) * hidReportSize;
	QVector<uint8_t> reports(paddedSize, 0);
	std::copy(message.cbegin(), message.cend(), reports.begin());
	return reports;
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

uint8_t LedDeviceRobobloq::checksum(const uint8_t* data, int size)
{
	uint32_t sum = 0;
	for (int index = 0; index < size; ++index)
	{
		sum += data[index];
	}
	return static_cast<uint8_t>(sum & 0xFFU);
}
