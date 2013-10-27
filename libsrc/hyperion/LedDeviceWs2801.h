#pragma once

// STL includes
#include <string>

// Linux-SPI includes
#include <linux/spi/spidev.h>

// hyperion incluse
#include <hyperion/LedDevice.h>

///
/// Implementation of the LedDevice interface for writing to Ws2801 led device.
///
class LedDeviceWs2801 : public LedDevice
{
public:
	///
	/// Constructs the LedDevice for a string containing leds of the type Ws2801
	///
	/// @param outputDevice The name of the output device (eg '/etc/SpiDev.0.0')
	/// @param baudrate The used baudrate for writing to the output device
	///
	LedDeviceWs2801(const std::string& outputDevice,
					const unsigned baudrate);

	///
	/// Destructor of the LedDevice; closes the output device if it is open
	///
	virtual ~LedDeviceWs2801();

	///
	/// Opens and configures the output device
	///
	/// @return Zero on succes else negative
	///
	int open();

	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<RgbColor> &ledValues);

	/// Switch the leds off
	virtual int switchOff();

private:
	/// The name of the output device
	const std::string mDeviceName;
	/// The used baudrate of the output device
	const int mBaudRate_Hz;

	/// The File Identifier of the opened output device (or -1 if not opened)
	int mFid;
	/// The transfer structure for writing to the spi-device
	spi_ioc_transfer spi;
	/// The 'latch' time for latching the shifted-value into the leds
	timespec latchTime;

	/// the number of leds (needed when switching off)
	size_t mLedCount;
};
