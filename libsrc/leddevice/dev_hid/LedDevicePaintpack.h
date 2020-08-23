#ifndef LEDEVICEPAINTTPACK_H
#define LEDEVICEPAINTTPACK_H

// Hyperion includes
#include "ProviderHID.h"

///
/// LedDevice implementation for a paintpack LED-device
///
class LedDevicePaintpack : public ProviderHID
{
public:

	///
	/// @brief Constructs a Paintpack LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDevicePaintpack(const QJsonObject &deviceConfig);

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	///
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

#endif // LEDEVICEPAINTTPACK_H
