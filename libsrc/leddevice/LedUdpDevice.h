#pragma once

#include <QUdpSocket>

// Hyperion includes
#include <leddevice/LedDevice.h>
#include <utils/Logger.h>

///
/// The LedUdpDevice implements an abstract base-class for LedDevices using the SPI-device.
///
class LedUdpDevice : public LedDevice
{
public:
	///
	/// Constructs the LedDevice attached to a SPI-device
	///
	/// @param[in] outputDevice The name of the output device (eg '/etc/UdpDev.0.0')
	/// @param[in] baudrate The used baudrate for writing to the output device
	/// @param[in] latchTime_ns The latch-time to latch in the values across the SPI-device (negative
	/// means no latch required) [ns]
	///
	LedUdpDevice(const std::string& outputDevice, const unsigned baudrate, const int latchTime_ns = -1);

	///
	/// Destructor of the LedDevice; closes the output device if it is open
	///
	virtual ~LedUdpDevice();

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
	/// The UDP destination as "host:port"
	const std::string _target;
	/// The used baudrate of the output device for rate limiting
	const int _BaudRate_Hz;
	/// The time which the device should be untouched after a write
	const int _LatchTime_ns;

	///
	QUdpSocket *udpSocket;
	QHostAddress _address;
	quint16 _port;
};

