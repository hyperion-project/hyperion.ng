#ifndef LEDEVICESK6812SPI_H
#define LEDEVICESK6812SPI_H

// hyperion includes
#include "ProviderSpi.h"

///
/// Implementation of the LedDevice interface for writing to Sk6801 LED-device via SPI.
///
class LedDeviceSk6812SPI : public ProviderSpi
{
public:

	///
	/// @brief Constructs a Sk6801 LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceSk6812SPI(const QJsonObject &deviceConfig);

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

	RGBW::WhiteAlgorithm _whiteAlgorithm;

	const int SPI_BYTES_PER_COLOUR;
	uint8_t bitpair_to_byte[4];

	ColorRgbw _temp_rgbw;
};

#endif // LEDEVICESK6812SPI_H
