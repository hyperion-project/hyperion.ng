#ifndef LEDEVICELPD6803_H
#define LEDEVICELPD6803_H

// Local hyperion includes
#include "ProviderSpi.h"

///
/// Implementation of the LedDevice interface for writing to LDP6803 LED-device.
///
/// 00000000 00000000 00000000 00000000 1RRRRRGG GGGBBBBB 1RRRRRGG GGGBBBBB ...
/// |---------------------------------| |---------------| |---------------|
/// 32 zeros to start the frame LED1 LED2 ...
///
/// For each led, the first bit is always 1, and then you have 5 bits each for red, green and blue
/// (R, G and B in the above illustration) making 16 bits per led. Total bytes = 4 + (2 x number of LEDs)
///
class LedDeviceLpd6803 : public ProviderSpi
{
public:

	///
	/// @brief Constructs a LDP6803 LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceLpd6803(const QJsonObject &deviceConfig);

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

#endif // LEDEVICELPD6803_H
