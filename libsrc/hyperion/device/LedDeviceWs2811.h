#pragma once

// Local hyperion includes
#include "LedRs232Device.h"

class LedDeviceWs2811 : public LedRs232Device
{
public:
	///
	/// Constructs the LedDevice with Ws2811 attached via a serial port
	///
	/// @param outputDevice The name of the output device (eg '/dev/ttyS0')
	/// @param fastDevice The used baudrate for writing to the output device
	///
	LedDeviceWs2811(const std::string& outputDevice, const bool fastDevice);

	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb> & ledValues);

	/// Switch the leds off
	virtual int switchOff();


private:

	void fillEncodeTable();

	/** Translation table of byte to signal */
	std::vector<unsigned> _byteToSignalTable;

	/// The buffer containing the packed RGB values
	std::vector<unsigned> _ledBuffer;
};
