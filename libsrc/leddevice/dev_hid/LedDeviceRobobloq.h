#ifndef LEDEVICEROBOBLOQ_H
#define LEDEVICEROBOBLOQ_H

#include "ProviderHID.h"

#include <QJsonObject>
#include <QString>
#include <QVector>

#include <cstdint>

/// LED-device implementation for Robobloq and compatible DX-Light monitor
/// RGB strips using the RB/SC protocol (VID 1a86, PID fe07).
class LedDeviceRobobloq : public ProviderHID
{
public:
	explicit LedDeviceRobobloq(const QJsonObject& deviceConfig);

	static LedDevice* construct(const QJsonObject& deviceConfig);

	QJsonObject discover(const QJsonObject& params) override;
	QJsonObject getProperties(const QJsonObject& params) override;

protected:
	bool init(const QJsonObject& deviceConfig) override;
	int open() override;
	int write(const QVector<ColorRgb>& ledValues) override;
	bool powerOff() override;

private:
	struct DeviceInfo
	{
		QString _deviceId;
		QString _uuid;
		QString _firmwareVersion;
		uint8_t _physicalSize{0};
		uint8_t _ledCount{0};

		[[nodiscard]] QJsonObject toJson() const;
	};

	QString _devicePath;
	int _hardwareBrightness;
	/// Next message ID for the persistent streaming connection.
	uint8_t _nextMessageId;

	QJsonObject buildDeviceProperties(QJsonObject properties) const;
	/// The caller owns the counter: the active connection passes _nextMessageId,
	/// while discovery and property queries use an independent local sequence.
	bool readDeviceInfo(hid_device* handle, uint8_t& nextMessageId, DeviceInfo& deviceInfo) const;
	int sendRb(uint8_t action, const QVector<uint8_t>& payload = {});
	int sendSc(uint8_t action, const QVector<uint8_t>& payload);

	static bool parseDeviceInfoReply(const uint8_t* data, int size, DeviceInfo& deviceInfo, uint8_t& messageId);
	static uint8_t allocateMessageId(uint8_t& nextMessageId);
	static QVector<uint8_t> createRbMessage(uint8_t action, const QVector<uint8_t>& payload, uint8_t messageId);
	static QVector<uint8_t> padToHidReports(const QVector<uint8_t>& message);
	static QVector<uint8_t> encodeColors(const QVector<ColorRgb>& ledValues);
	static uint8_t checksum(const uint8_t* data, int size);
};

#endif // LEDEVICEROBOBLOQ_H
