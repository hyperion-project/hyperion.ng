#pragma once

// Qt includes
#include <QTimer>

// hyperion include
#include "ProviderHID.h"

///
/// Implementation of the LedDevice interface for writing to an RawHID led device.
///
class LedDeviceRawHID : public ProviderHID
{

public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	LedDeviceRawHID(const QJsonObject &deviceConfig);

	/// constructs leddevice
	static LedDevice* construct(const QJsonObject &deviceConfig);

private slots:
	/// Write the last data to the leds again
	void rewriteLeds();

private:
	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb> & ledValues);
};
