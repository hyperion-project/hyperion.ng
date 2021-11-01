#ifndef LEDEVICEAPA102_H
#define LEDEVICEAPA102_H

// hyperion includes
#include "ProviderSpi.h"

/// The maximal level supported by the APA  brightness control field, 31
const int APA102_BRIGHTNESS_MAX_LEVEL = 31;

///
/// Implementation of the LedDevice interface for writing to APA102 led device.
///
class LedDeviceAPA102 : public ProviderSpi
{
public:

	///
	/// @brief Constructs an APA102 LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceAPA102(const QJsonObject &deviceConfig);

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

	///
	/// @brief Writes the RGB-Color values to the SPI Tx buffer setting considering a given brightness level
	///
	/// @param[in,out] txBuf The packed spi transfer buffer of the LED's color values
	/// @param[in] ledValues The RGB-color per LED
	/// @param[in] brightness The current brightness level 1 .. 31
	///
	void bufferWithBrightness(std::vector<uint8_t> &txBuf, const std::vector<ColorRgb> & ledValues, const int brightness = APA102_BRIGHTNESS_MAX_LEVEL);

	/// The brighness level. Possibile values 1 .. 31.
	int _brightnessControlMaxLevel;
};

#endif // LEDEVICEAPA102_H
