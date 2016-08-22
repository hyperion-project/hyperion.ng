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
	/// Constructs the LedDevice sendig data via udp
	///
	/// @param[in] outputDevice string hostname:port
	/// @param[in] latchTime_ns The latch-time to latch in the values across the SPI-device (negative
	/// means no latch required) [ns]
	///
	LedUdpDevice(const Json::Value &deviceConfig);

	///
	/// Destructor of the LedDevice; closes the output device if it is open
	///
	virtual ~LedUdpDevice();

	bool setConfig(const Json::Value &deviceConfig);

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
	/// @param[in] size The length of the data
	/// @param[in] data The data
	///
	/// @return Zero on succes else negative
	///
	int writeBytes(const unsigned size, const uint8_t *data);

	/// The time which the device should be untouched after a write
	int _LatchTime_ns;

	///
	QUdpSocket * _udpSocket;
	QHostAddress _address;
	quint16      _port;
};

