#ifndef LEDEVICESK9822_H
#define LEDEVICESK9822_H

// hyperion includes
#include "ProviderSpi.h"

///
/// Implementation of the LedDevice interface for writing to SK9822 led device via SPI.
///
class LedDeviceSK9822 : public ProviderSpi
{
public:

	///
	/// @brief Constructs an SK9822 LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceSK9822(const QJsonObject &deviceConfig);

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	///
	static LedDevice* construct(const QJsonObject &deviceConfig);

protected:
	///
	/// @brief Writes the RGB-Color values to the SPI Tx buffer setting SK9822 current level to maximal value.
	///
	/// @param[in,out] txBuf The packed spi transfer buffer of the LED's color values
	/// @param[in] ledValues The RGB-color per LED
	/// @param[in] maxLevel The maximal current level 1 .. 31 to use
	///
	void bufferWithMaxCurrent(std::vector<uint8_t> &txBuf, const std::vector<ColorRgb> & ledValues, const int maxLevel);

	///
	/// @brief Writes the RGB-Color values to the SPI Tx buffer using an adjusted SK9822 current level for LED maximal rgb-grayscale values not exceeding the threshold, uses maximal level otherwise.
	///
	/// @param[in,out] txBuf The packed spi transfer buffer of the LED's color values
	/// @param[in] ledValues The RGB-color per LED
	/// @param[in] threshold The threshold 0 .. 255 that defines whether to use adjusted SK9822 current level per LED
	/// @param[in] maxLevel The maximal current level 1 .. 31 to use
	///
	void bufferWithAdjustedCurrent(std::vector<uint8_t> &txBuf, const std::vector<ColorRgb> & ledValues, const int threshold, const int maxLevel);

	/// The threshold that defines use of SK9822 global brightness control for maximal rgb grayscale values below.
	/// i.e. global brightness control is used for rgb-values when max(r,g,b) < threshold.
	int _globalBrightnessControlThreshold;

	/// The maximal current level
	int _globalBrightnessControlMaxLevel;

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

#endif // LEDEVICESK9822_H
