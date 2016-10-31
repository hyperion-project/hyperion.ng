#pragma once

// hyperion incluse
#include "ProviderSpi.h"


///
/// Implementation of the LedDevice interface for writing to APA102 led device.
///
class LedDeviceAPA102 : public ProviderSpi
{
public:
	///
	/// Constructs specific LedDevice
	///
	LedDeviceAPA102(const QJsonObject &deviceConfig);

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
