#ifndef LEDEVICEP9813_H
#define LEDEVICEP9813_H

// hyperion includes
#include "ProviderSpi.h"

///
/// Implementation of the LedDevice interface for writing to P9813 LED-device.
///
class LedDeviceP9813 : public ProviderSpi
{
public:
	///
	/// @brief Constructs a P9813 LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceP9813(const QJsonObject &deviceConfig);

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	static LedDevice* construct(const QJsonObject &deviceConfig);

private:

	///
	/// @brief Initialise the device's configuration
	///
	/// @param[in] deviceConfig the JSON device configuration
	/// @return True, if success
	///
	bool init(const QJsonObject &deviceConfig) override;

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	int write(const std::vector<ColorRgb> & ledValues) override;

	///
	/// Calculates the required checksum for one LED
	///
	/// @param color The color of the led
	/// @return The checksum for the led
	///
	uint8_t calculateChecksum(const ColorRgb & color) const;
};

#endif // LEDEVICEP9813_H
