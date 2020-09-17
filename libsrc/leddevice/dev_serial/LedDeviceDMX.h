#ifndef LEDEVICEDMX_H
#define LEDEVICEDMX_H

// hyperion includes
#include "ProviderRs232.h"

///
/// Implementation of the LedDevice interface for writing to DMX512 rs232 LED-device.
///
class LedDeviceDMX : public ProviderRs232
{
public:

	///
	/// @brief Constructs a DMX LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceDMX(const QJsonObject &deviceConfig);

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
	int write(const std::vector<ColorRgb> &ledValues) override;

	int _dmxDeviceType = 0;
	int _dmxStart = 1;
	int _dmxSlotsPerLed = 3;
	int _dmxLedCount = 0;
	unsigned int _dmxChannelCount = 0;
};

#endif // LEDEVICEDMX_H
