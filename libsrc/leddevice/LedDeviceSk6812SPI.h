#pragma once

// hyperion incluse
#include "ProviderSpi.h"

///
/// Implementation of the LedDevice interface for writing to Sk6801 led device via SPI.
///
class LedDeviceSk6812SPI : public ProviderSpi
{
public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	LedDeviceSk6812SPI(const QJsonObject &deviceConfig);

	/// constructs leddevice
	static LedDevice* construct(const QJsonObject &deviceConfig);

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	bool init(const QJsonObject &deviceConfig);
	
private:
	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb> &ledValues);

	RGBW::WhiteAlgorithm _whiteAlgorithm;
	
        const int SPI_BYTES_PER_COLOUR;

	uint8_t bitpair_to_byte[4];
	
	ColorRgbw _temp_rgbw;
};
