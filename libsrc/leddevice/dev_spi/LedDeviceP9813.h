#pragma once

// hyperion includes
#include "ProviderSpi.h"

///
/// Implementation of the LedDevice interface for writing to P9813 led device.
///
class LedDeviceP9813 : public ProviderSpi
{
public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	explicit LedDeviceP9813(const QJsonObject &deviceConfig);

	/// constructs leddevice
	static LedDevice* construct(const QJsonObject &deviceConfig);

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	virtual bool init(const QJsonObject &deviceConfig) override;

private:
	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb> &ledValues) override;

	///
	/// Calculates the required checksum for one led
	///
	/// @param color The color of the led
	/// @return The checksum for the led
	///
	uint8_t calculateChecksum(const ColorRgb & color) const;
};
