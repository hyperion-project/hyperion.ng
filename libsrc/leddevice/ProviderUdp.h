#pragma once

#include <QUdpSocket>

// Hyperion includes
#include <leddevice/LedDevice.h>
#include <utils/Logger.h>

///
/// The ProviderUdp implements an abstract base-class for LedDevices using UDP packets.
///
class ProviderUdp : public LedDevice
{
public:
	///
	/// Constructs specific LedDevice
	///
	ProviderUdp();

	///
	/// Destructor of the LedDevice; closes the output device if it is open
	///
	virtual ~ProviderUdp();

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	virtual bool init(const QJsonObject &deviceConfig);

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
	QString      _defaultHost;
};
