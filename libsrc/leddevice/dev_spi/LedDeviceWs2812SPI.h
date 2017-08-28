#pragma once

// hyperion incluse
#include "ProviderSpi.h"

///
/// Implementation of the LedDevice interface for writing to Ws2812 led device via spi.
///
class LedDeviceWs2812SPI : public ProviderSpi
{
public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	LedDeviceWs2812SPI(const QJsonObject &deviceConfig);

	/// constructs leddevice
	static LedDevice* construct(const QJsonObject &deviceConfig);

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	virtual bool init(const QJsonObject &deviceConfig);

private:
	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb> &ledValues);

        const int SPI_BYTES_PER_COLOUR;

	const int SPI_FRAME_END_LATCH_BYTES;

	uint8_t bitpair_to_byte[4];
};
