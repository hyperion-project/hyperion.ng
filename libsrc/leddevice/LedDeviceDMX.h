#pragma once

// hyperion incluse
#include "ProviderRs232.h"

///
/// Implementation of the LedDevice interface for writing to DMX512 rs232 led device.
///
class LedDeviceDMX : public ProviderRs232
{
public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	LedDeviceDMX(const Json::Value &deviceConfig);

	/// constructs leddevice
	static LedDevice* construct(const Json::Value &deviceConfig);

private:
	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb> &ledValues);
};
