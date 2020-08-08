#ifndef LEDEVICETPM2_H
#define LEDEVICETPM2_H

// hyperion includes
#include "ProviderRs232.h"

///
/// Implementation of the LedDevice interface for writing to serial device using tpm2 protocol.
///
class LedDeviceTpm2 : public ProviderRs232
{
public:

	///
	/// @brief Constructs a TPM 2 LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceTpm2(const QJsonObject &deviceConfig);

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
};

#endif // LEDEVICETPM2_H
