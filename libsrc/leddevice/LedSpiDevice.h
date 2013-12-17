#pragma once

// Linux-SPI includes
#include <linux/spi/spidev.h>

// Hyperion includes
#include <leddevice/LedDevice.h>

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
	/// @param[in] latchTime_ns The latch-time to latch in the values across the SPI-device (negative
	/// means no latch required) [ns]
	///
	LedSpiDevice(const std::string& outputDevice, const unsigned baudrate, const int latchTime_ns = -1);

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

protected:
	///
	/// Writes the given bytes/bits to the SPI-device and sleeps the latch time to ensure that the
	/// values are latched.
	///
	/// @param[in[ size The length of the data
	/// @param[in] data The data
	///
	/// @return Zero on succes else negative
	///
	int writeBytes(const unsigned size, const uint8_t *data);

private:
	/// The name of the output device
	const std::string mDeviceName;
	/// The used baudrate of the output device
	const int mBaudRate_Hz;
	/// The time which the device should be untouched after a write
	const int mLatchTime_ns;

	/// The File Identifier of the opened output device (or -1 if not opened)
	int mFid;
	/// The transfer structure for writing to the spi-device
	spi_ioc_transfer spi;
};
