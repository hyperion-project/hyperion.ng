#pragma once

// Serial includes
#include <serial/serial.h>

// Hyperion includes
#include <hyperion/LedDevice.h>

///
/// The LedRs232Device implements an abstract base-class for LedDevices using a RS232-device.
///
class LedRs232Device : public LedDevice
{
public:
	///
	/// Constructs the LedDevice attached to a RS232-device
	///
	/// @param[in] outputDevice The name of the output device (eg '/etc/ttyS0')
	/// @param[in] baudrate The used baudrate for writing to the output device
	///
	LedRs232Device(const std::string& outputDevice, const unsigned baudrate);

	///
	/// Destructor of the LedDevice; closes the output device if it is open
	///
	virtual ~LedRs232Device();

	///
	/// Opens and configures the output device
	///
	/// @return Zero on succes else negative
	///
	int open();

protected:
	/**
	 * Writes the given bytes to the RS232-device and
	 *
	 * @param[in[ size The length of the data
	 * @param[in] data The data
	 *
	 * @return Zero on succes else negative
	 */
	int writeBytes(const unsigned size, const uint8_t *data);

private:
	/// The name of the output device
	const std::string mDeviceName;
	/// The used baudrate of the output device
	const int mBaudRate_Hz;

	/// The RS232 serial-device
	serial::Serial _rs232Port;
};
