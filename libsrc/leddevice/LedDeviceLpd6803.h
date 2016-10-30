#pragma once

// Local hyperion incluse
#include "ProviderSpi.h"

///
/// Implementation of the LedDevice interface for writing to LDP6803 led device.
///
/// 00000000 00000000 00000000 00000000 1RRRRRGG GGGBBBBB 1RRRRRGG GGGBBBBB ...
/// |---------------------------------| |---------------| |---------------|
/// 32 zeros to start the frame Led1 Led2 ...
///
/// For each led, the first bit is always 1, and then you have 5 bits each for red, green and blue
/// (R, G and B in the above illustration) making 16 bits per led. Total bytes = 4 + (2 x number of
/// leds)
///
class LedDeviceLpd6803 : public ProviderSpi
{
public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	LedDeviceLpd6803(const QJsonObject &deviceConfig);

	/// constructs leddevice
	static LedDevice* construct(const QJsonObject &deviceConfig);

	virtual bool init(const QJsonObject &deviceConfig);

private:
	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb> &ledValues);
};
