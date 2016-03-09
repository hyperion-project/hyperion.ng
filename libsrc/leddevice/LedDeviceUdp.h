#pragma once

// STL includes0
#include <fstream>

// Leddevice includes
#include <leddevice/LedDevice.h>

///
/// Implementation of the LedDevice that write the led-colors to an
/// ASCII-textfile('/home/pi/LedDevice.out')
///
class LedDeviceUdp : public LedDevice
{
public:
	///
	/// Constructs the test-device, which opens an output stream to the file
	///
	LedDeviceUdp(const std::string& output,  const unsigned baudrate, const unsigned protocol, const unsigned maxPacket);

	///
	/// Destructor of this test-device
	///
	virtual ~LedDeviceUdp();

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
//	std::ofstream _ofs;

        /// the number of leds (needed when switching off)
        size_t mLedCount;
};
