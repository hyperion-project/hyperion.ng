#ifndef LEDEVICEWS2812_H
#define LEDEVICEWS2812_H

// hyperion includes
#include "ProviderSpi.h"

///
/// Implementation of the LedDevice interface for writing to Ws2812 led device.
///
class LedDeviceWs2812SPI : public ProviderSpi
{
public:

	///
	/// @brief Constructs a Ws2812 LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceWs2812SPI(const QJsonObject &deviceConfig);

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

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	virtual int write(const std::vector<ColorRgb> & ledValues) override;

	const int SPI_BYTES_PER_COLOUR;
	const int SPI_FRAME_END_LATCH_BYTES;

	uint8_t bitpair_to_byte[4];
};

#endif // LEDEVICEWS2812_H
