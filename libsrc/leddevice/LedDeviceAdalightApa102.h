#pragma once

// STL includes
#include <string>

// hyperion include
#include "LedDeviceAdalight.h"
#include <json/json.h>

///
/// Implementation of the LedDevice interface for writing to an Adalight led device for APA102.
///
class LedDeviceAdalightApa102 : public LedDeviceAdalight
{
	Q_OBJECT

public:
	///
	/// Constructs the LedDevice for attached Adalight device
	///
	/// @param outputDevice The name of the output device (eg '/dev/ttyS0')
	/// @param baudrate The used baudrate for writing to the output device
	///
	LedDeviceAdalightApa102(const Json::Value &deviceConfig);

	/// create leddevice when type in config is set to this type
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
};

