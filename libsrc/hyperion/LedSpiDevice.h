#pragma once

// Linux-SPI includes
#include <linux/spi/spidev.h>

// Hyperion includes
#include <hyperion/LedDevice.h>

///
/// The LedSpiDevice implements an abstract base-class for LedDevices using the SPI-device.
///
class LedSpiDevice : public LedDevice
{
public:
	///
	/// Constructs the LedDevice attached to a SPI-device
	///
	/// @param[in] outputDevice The name of the output device (eg '/etc/SpiDev.0.0')
	/// @param[in] baudrate The used baudrate for writing to the output device
	///
	LedSpiDevice(const std::string& outputDevice, const unsigned baudrate);

	///
	/// Destructor of the LedDevice; closes the output device if it is open
	///
	virtual ~LedSpiDevice();

	///
	/// Opens and configures the output device
	///
	/// @return Zero on succes else negative
	///
	int open();

private:
	/// The name of the output device
	const std::string mDeviceName;
	/// The used baudrate of the output device
	const int mBaudRate_Hz;

protected:
	/// The File Identifier of the opened output device (or -1 if not opened)
	int mFid;
	/// The transfer structure for writing to the spi-device
	spi_ioc_transfer spi;
};
