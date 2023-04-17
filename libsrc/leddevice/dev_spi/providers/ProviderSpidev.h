#pragma once
#include "providers/BaseProvider.h"
// Linux-SPI includes
#include <linux/spi/spidev.h>
#include "QObject"

///
/// The ProviderSpi implements an abstract base-class for LedDevices using the SPI-device.
///
class ProviderSpidev : public BaseProvider
{
public:
	///
	/// Constructs specific LedDevice
	///
    ProviderSpidev(const QJsonObject& deviceConfig);

	///
	/// Destructor of the LedDevice; closes the output device if it is open
	///
	~ProviderSpidev();

	///
	/// Opens and configures the output device
	///
	/// @return Zero on succes else negative
	///
	int open() override;
    QJsonArray discover(const QJsonObject& params) override;
	int close() override;
    int writeBytes(unsigned size, const uint8_t* data) override;

protected:
	/// The File Identifier of the opened output device (or -1 if not opened)
	int _fid;

	/// which spi clock mode do we use? (0..3)
	int _spiMode;

	/// 1=>invert the data pattern
	bool _spiDataInvert;

	/// The transfer structure for writing to the spi-device
	spi_ioc_transfer _spi;
};
