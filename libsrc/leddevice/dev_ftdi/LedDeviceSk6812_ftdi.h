#ifndef LEDEVICESK6812ftdi_H
#define LEDEVICESK6812ftdi_H

#include "ProviderFtdi.h"

///
/// Implementation of the LedDevice interface for writing to Sk6801 LED-device via SPI.
///
class LedDeviceSk6812_ftdi : public ProviderFtdi
{
public:

	///
	/// @brief Constructs a Sk6801 LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceSk6812_ftdi(const QJsonObject& deviceConfig);

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	static LedDevice* construct(const QJsonObject& deviceConfig);

private:

	///
	/// @brief Initialise the device's configuration
	///
	/// @param[in] deviceConfig the JSON device configuration
	/// @return True, if success
	///
	bool init(const QJsonObject& deviceConfig) override;

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	int write(const std::vector<ColorRgb>& ledValues) override;

	inline __attribute__((always_inline)) uint8_t scale(uint8_t i, uint8_t scale);

	RGBW::WhiteAlgorithm _whiteAlgorithm;

	const int SPI_BYTES_PER_COLOUR;
	uint8_t bitpair_to_byte[4];

	int _brightnessControlMaxLevel;
};

#endif // LEDEVICESK6812ftdi_H
