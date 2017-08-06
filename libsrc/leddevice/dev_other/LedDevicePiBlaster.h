#pragma once

// Hyperion-Leddevice includes
#include <leddevice/LedDevice.h>

///
/// Implementation of the LedDevice interface for writing to pi-blaster based PWM LEDs
///

class LedDevicePiBlaster : public LedDevice
{
public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	LedDevicePiBlaster(const QJsonObject &deviceConfig);

	virtual ~LedDevicePiBlaster();

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	bool init(const QJsonObject &deviceConfig);

	/// constructs leddevice
	static LedDevice* construct(const QJsonObject &deviceConfig);
	
	///
	/// Attempts to open the piblaster-device. This will only succeed if the device is not yet open
	/// and the device is available.
	///
	/// @return Zero on succes else negative
	///
	int open();

private:
	///
	/// Writes the colors to the PiBlaster device
	///
	/// @param ledValues The color value for each led
	///
	/// @return Zero on success else negative
	///
	int write(const std::vector<ColorRgb> &ledValues);

	/// The name of the output device (very likely '/dev/pi-blaster')
	QString _deviceName;

	int _gpio_to_led[64];
	char _gpio_to_color[64];

	/// File-Pointer to the PiBlaster device
	FILE * _fid;

};
