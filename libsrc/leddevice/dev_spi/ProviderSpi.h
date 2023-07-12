#pragma once

// HyperHDR includes
#include <leddevice/LedDevice.h>
#include <providers/BaseProvider.h>

enum SpiImplementation { SPIDEV, FTDI };

///
/// The ProviderSpi implements an abstract base-class for LedDevices using the SPI-device.
///
class ProviderSpi : public LedDevice
{
public:
	///
	/// Constructs specific LedDevice
	///
	ProviderSpi(const QJsonObject& deviceConfig);

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	bool init(const QJsonObject& deviceConfig) override;

	///
	/// Destructor of the LedDevice; closes the output device if it is open
	///
	~ProviderSpi() override;

	///
	/// Opens and configures the output device
	///
	/// @return Zero on success else negative
	///
	int open() override;

	QJsonObject discover(const QJsonObject& params) override;

public slots:
	///
	/// Closes the output device.
	/// Includes switching-off the device and stopping refreshes
	///
	int close() override;

protected:
	///
	/// Writes the given bytes/bits to the SPI-device and sleeps the latch time to ensure that the
	/// values are latched.
	///
	/// @param[in[ size The length of the data
	/// @param[in] data The data
	///
	/// @return Zero on success, else negative
	///
	int writeBytes(unsigned size, const uint8_t* data);

	/// The name of the output device
	QString _deviceName;

    BaseProvider * _spiProvider;
};
