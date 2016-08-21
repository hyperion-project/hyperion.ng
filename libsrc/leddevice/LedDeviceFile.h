#pragma once

// STL includes0
#include <fstream>

// Leddevice includes
#include <leddevice/LedDevice.h>

///
/// Implementation of the LedDevice that write the led-colors to an
/// ASCII-textfile('/home/pi/LedDevice.out')
///
class LedDeviceFile : public LedDevice
{
public:
	///
	/// Constructs the test-device, which opens an output stream to the file
	///
	LedDeviceFile(const Json::Value &deviceConfig);

	///
	/// Destructor of this test-device
	///
	virtual ~LedDeviceFile();

	/// create leddevice when type in config is set to this type
	static LedDevice* construct(const Json::Value &deviceConfig);

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	virtual bool setConfig(const Json::Value &deviceConfig);
	
	///
	/// Writes the given led-color values to the output stream
	///
	/// @param ledValues The color-value per led
	///
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector<ColorRgb> & ledValues);

	/// Switch the leds off
	virtual int switchOff();

private:
	/// The outputstream
	std::ofstream _ofs;
};
