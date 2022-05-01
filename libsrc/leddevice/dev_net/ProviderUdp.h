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
	ProviderUdp(const QJsonObject& deviceConfig);

	///
	/// @brief Destructor of the UDP LED-device
	///
	~ProviderUdp() override;

	QHostAddress getAddress() const { return _address; }

protected:

	///
	/// @brief Opens the output device.
	///
	/// @return Zero on success (i.e. device is ready), else negative
	///
	int open() override;

	///
	/// @brief Closes the UDP device.
	///
	/// @return Zero on success (i.e. device is closed), else negative
	///
	int close() override;

	///
	/// @brief Writes the given bytes to the UDP-device
	///
	/// @param[in] size The length of the data
	/// @param[in] data The data
	///
	/// @return Zero on success, else negative
	///
	int writeBytes(const unsigned size, const uint8_t* data);

	///
	/// @brief Writes the given bytes to the UDP-device
	///
	/// @param[in] data The data
	///
	/// @return Zero on success, else negative
	///
	int writeBytes(const QByteArray& bytes);

	///
	QUdpSocket*  _udpSocket;
	QString      _hostName;
	QHostAddress _address;
	int       _port;
};

#endif // PROVIDERUDP_H
