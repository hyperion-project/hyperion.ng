#ifndef LEDEVICEAPA104_H
#define LEDEVICEAPA104_H

// hyperion includes
#include "ProviderSpi.h"

///
/// Implementation of the LedDevice interface for writing to APA104 led device via spi.
///
class LedDeviceAPA104 : public ProviderSpi
{
public:

	///
	/// @brief Constructs an APA104 LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceAPA104(const QJsonObject &deviceConfig);

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

	const int SPI_BYTES_PER_COLOUR;
	const int SPI_FRAME_END_LATCH_BYTES;

	uint8_t bitpair_to_byte[4];
};

#endif // LEDEVICEAPA104_H
