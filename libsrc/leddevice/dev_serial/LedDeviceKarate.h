#ifndef LEDEVICEKARATE_H
#define LEDEVICEKARATE_H

// hyperion includes
#include "ProviderRs232.h"

///
/// Implementation of the LedDevice interface for writing to serial device
///
class LedDeviceKarate : public ProviderRs232
{
	Q_OBJECT
public:

	///
	/// @brief Constructs a Karate LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceKarate(const QJsonObject &deviceConfig);

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
	virtual bool init(const QJsonObject &deviceConfig) override;

	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	virtual int write(const std::vector<ColorRgb> &ledValues) override;
};

#endif // LEDEVICEKARATE_H
