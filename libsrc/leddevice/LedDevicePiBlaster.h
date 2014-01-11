
#pragma once

// STL includes
#include <cstdio>

// Hyperion-Leddevice includes
#include <leddevice/LedDevice.h>

class LedDevicePiBlaster : public LedDevice
{
public:
	///
	/// Constructs the PiBlaster device which writes to the indicated device and for the assigned
	/// channels
	/// @param deviceName The name of the output device
	/// @param channelAssignment The RGB-Channel assignment (8 characters long)
	///
	LedDevicePiBlaster(const std::string & deviceName, const std::string & channelAssignment);

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

	/// String with eight characters with the rgb-channel assignment per pwm-channel
	/// ('r' = red, 'g' = green, 'b' = blue, ' ' = empty)
	const std::string _channelAssignment;

	/// File-Pointer to the PiBlaster device
	FILE * _fid;
};
