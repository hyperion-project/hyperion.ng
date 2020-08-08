#ifndef PROVIDERUDP_H
#define PROVIDERUDP_H

// LedDevice includes
#include <leddevice/LedDevice.h>

// Hyperion includes
#include <utils/Logger.h>

// Qt includes
#include <QHostAddress>
#include <QUdpSocket>

///
/// The ProviderUdp implements an abstract base-class for LedDevices using UDP packets.
///
class ProviderUdp : public LedDevice
{
public:

	///
	/// @brief Constructs an UDP LED-device
	///
	ProviderUdp();

	///
	/// @brief Destructor of the UDP LED-device
	///
	virtual ~ProviderUdp() override;

protected:

	///
	/// @brief Initialise the UDP device's configuration and network address details
	///
	/// @param[in] deviceConfig the JSON device configuration
	/// @return True, if success
	///
	virtual bool init(const QJsonObject &deviceConfig) override;

	///
	/// @brief Opens the output device.
	///
	/// @return Zero on success (i.e. device is ready), else negative
	///
	virtual int open() override;

	///
	/// @brief Closes the UDP device.
	///
	/// @return Zero on success (i.e. device is closed), else negative
	///
	virtual int close() override;

	///
	/// @brief Writes the given bytes/bits to the UDP-device and sleeps the latch time to ensure that the
	/// values are latched.
	///
	/// @param[in] size The length of the data
	/// @param[in] data The data
	///
	/// @return Zero on success, else negative
	///
	int writeBytes(unsigned size, const uint8_t *data);

	///
	QUdpSocket * _udpSocket;
	QHostAddress _address;
	ushort       _port;
	QString      _defaultHost;
};

#endif // PROVIDERUDP_H
