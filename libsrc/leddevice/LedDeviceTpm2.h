#pragma once

// STL includes
#include <string>

// hyperion incluse
#include "LedRs232Device.h"
#include <json/json.h>

///
/// Implementation of the LedDevice interface for writing to serial device using tpm2 protocol.
///
class LedDeviceTpm2 : public LedRs232Device
{
public:
	///
	/// Constructs the LedDevice for attached serial device using supporting tpm2 protocol
	/// All LEDs in the stripe are handled as one frame
	///
	/// @param outputDevice The name of the output device (eg '/dev/ttyAMA0')
	/// @param baudrate The used baudrate for writing to the output device
	///
	LedDeviceTpm2(const Json::Value &deviceConfig);

	/// create leddevice when type in config is set to this type
	static LedDevice* construct(const Json::Value &deviceConfig);

	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb> &ledValues);

	/// Switch the leds off
	virtual int switchOff();
};
