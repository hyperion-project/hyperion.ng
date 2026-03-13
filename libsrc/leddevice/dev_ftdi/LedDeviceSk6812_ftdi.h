#ifndef LEDEVICESK6812ftdi_H
#define LEDEVICESK6812ftdi_H

#ifdef WIN32
#define INLINE __forceinline
#else
#define INLINE inline __attribute__((always_inline))
#endif

#include "ProviderFtdi.h"
#include <QVector>

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
	int write(const QVector<ColorRgb>& ledValues) override;

	inline uint8_t scale(uint8_t i, uint8_t scale) const {
		return (((uint16_t)i) * (1 + (uint16_t)(scale))) >> 8;
	}
	
	RGBW::WhiteAlgorithm _whiteAlgorithm;

	const int SPI_BYTES_PER_COLOUR;
	uint8_t bitpair_to_byte[4];

	int _brightnessControlMaxLevel;
};

#endif // LEDEVICESK6812ftdi_H
