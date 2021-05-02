#pragma once

// Linux-SPI includes
#include <linux/spi/spidev.h>

// Hyperion includes
#include <leddevice/LedDevice.h>

///
/// The ProviderSpi implements an abstract base-class for LedDevices using the SPI-device.
///
class ProviderSpi : public LedDevice
{
public:
	///
	/// Constructs specific LedDevice
	///
	ProviderSpi(const QJsonObject &deviceConfig);

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	bool init(const QJsonObject &deviceConfig) override;

	///
	/// Destructor of the LedDevice; closes the output device if it is open
	///
	~ProviderSpi() override;

	///
	/// Opens and configures the output device
	///
	/// @return Zero on succes else negative
	///
	int open() override;

	/// @param[in] params Parameters used to overwrite discovery default behaviour
	///
	/// @return A JSON structure holding a list of devices found
	///
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
	int writeBytes(unsigned size, const uint8_t *data);

	/// The name of the output device
	QString _deviceName;

	/// The used baudrate of the output device
	int _baudRate_Hz;

	/// The File Identifier of the opened output device (or -1 if not opened)
	int _fid;

	/// which spi clock mode do we use? (0..3)
	int _spiMode;

	/// 1=>invert the data pattern
	bool _spiDataInvert;

	/// The transfer structure for writing to the spi-device
	spi_ioc_transfer _spi;
};
