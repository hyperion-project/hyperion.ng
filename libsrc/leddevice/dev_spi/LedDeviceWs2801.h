#ifndef LEDEVICEWS2801_H
#define LEDEVICEWS2801_H

// hyperion includes
#include "ProviderSpi.h"

///
/// Implementation of the LedDevice interface for writing to Ws2801 led device.
///
class LedDeviceWs2801 : public ProviderSpi
{
public:

	///
	/// @brief Constructs a Ws2801 LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceWs2801(const QJsonObject &deviceConfig);

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
};

#endif // LEDEVICEWS2801_H
