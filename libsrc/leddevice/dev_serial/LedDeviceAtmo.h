#ifndef LEDEVICEATMO_H
#define LEDEVICEATMO_H

// hyperion includes
#include "ProviderRs232.h"

///
/// Implementation of the LedDevice interface for writing to serial device using tpm2 protocol.
///
class LedDeviceAtmo : public ProviderRs232
{
public:

	///
	/// @brief Constructs an Atmo LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceAtmo(const QJsonObject &deviceConfig);

	///
	/// @brief Destructor of the LedDevice
	///
	static LedDevice* construct(const QJsonObject &deviceConfig);

private:

	///
	/// @brief Initialise the device's configuration
	///
	/// @param[in] deviceConfig the JSON device configuration
	/// @return True, if success
	virtual bool init(const QJsonObject &deviceConfig) override;

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	virtual int write(const std::vector<ColorRgb> &ledValues) override;
};

#endif // LEDEVICEATMO_H
