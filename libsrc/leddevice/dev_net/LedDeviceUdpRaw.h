#pragma once

// hyperion includes
#include "ProviderUdp.h"

#define RAW_DEFAULT_PORT	5568

///
/// Implementation of the LedDevice interface for sending led colors via udp.
///
class LedDeviceUdpRaw : public ProviderUdp
{
public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	explicit LedDeviceUdpRaw(const QJsonObject &deviceConfig);

	/// constructs leddevice
	static LedDevice* construct(const QJsonObject &deviceConfig);

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	bool init(const QJsonObject &deviceConfig) override;

private:
	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb> &ledValues) override;
};
