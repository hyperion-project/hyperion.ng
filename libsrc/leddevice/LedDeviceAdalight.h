#pragma once

// STL includes
#include <string>

// Qt includes
#include <QTimer>

// hyperion include
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
	LedDeviceAdalight(const Json::Value &deviceConfig);

	/// constructs leddevice
	static LedDevice* construct(const Json::Value &deviceConfig);

	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb> & ledValues);

	/// Switch the leds off
	virtual int switchOff();

private slots:
	/// Write the last data to the leds again
	void rewriteLeds();

protected:
	/// Timer object which makes sure that led data is written at a minimum rate
	/// The Adalight device will switch off when it does not receive data at least
	/// every 15 seconds
	QTimer _timer;
};

