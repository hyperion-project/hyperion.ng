#pragma once

#include "ProviderRs232.h"

///
/// Implementation of the LedDevice interface for writing to an Adalight led device.
///
class LedDeviceAdalight : public ProviderRs232
{
	Q_OBJECT

public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	LedDeviceAdalight(const QJsonObject &deviceConfig);

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
	virtual int write(const std::vector<ColorRgb> & ledValues);
	
	const short _headerSize;
	bool _ligthBerryAPA102Mode;
};

