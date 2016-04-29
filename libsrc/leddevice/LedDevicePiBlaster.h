
#pragma once

// STL includes
#include <cstdio>

// jsoncpp includes
#include <json/json.h>

// Hyperion-Leddevice includes
#include <leddevice/LedDevice.h>

class LedDevicePiBlaster : public LedDevice
{
public:
	///
	/// Constructs the PiBlaster device which writes to the indicated device and for the assigned
	/// channels
	/// @param deviceName The name of the output device
	/// @param gpioMapping The RGB-Channel assignment json object
	///
	LedDevicePiBlaster(const std::string & deviceName, const Json::Value & gpioMapping);

	virtual ~LedDevicePiBlaster();

	///
	/// Attempts to open the piblaster-device. This will only succeed if the device is not yet open
	/// and the device is available.
	///
	/// @param report If true errors are writen to the standard error else silent
	/// @return Zero on succes else negative
	///
	int open(bool report = true);

	///
	/// Writes the colors to the PiBlaster device
	///
	/// @param ledValues The color value for each led
	///
	/// @return Zero on success else negative
	///
	int write(const std::vector<ColorRgb> &ledValues);

	///
	/// Switches off the leds
	///
	/// @return Zero on success else negative
	///
	int switchOff();

private:

	/// The name of the output device (very likely '/dev/pi-blaster')
	const std::string _deviceName;

	int _gpio_to_led[64];
	char _gpio_to_color[64];

	/// File-Pointer to the PiBlaster device
	FILE * _fid;
};
