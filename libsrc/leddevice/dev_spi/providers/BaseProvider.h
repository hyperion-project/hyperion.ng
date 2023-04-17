#pragma once
#include "QObject"
#include "utils/Logger.h"

///
/// The ProviderSpi implements an abstract base-class for LedDevices using the SPI-device.
///
class BaseProvider
{
public:
	///
	/// Constructs specific LedDevice
	///
    BaseProvider(const QJsonObject& deviceConfig);
	///
	/// Destructor of the LedDevice; closes the output device if it is open
	///
	~BaseProvider();

	///
	/// Opens and configures the output device
	///
	/// @return Zero on success else negative
	///
    virtual int open();

    virtual QJsonArray discover(const QJsonObject& params);

    virtual int close();

    virtual int writeBytes(unsigned size, const uint8_t *data);
protected:

    /// The common Logger instance for all LED-devices
    Logger* _log;

    /// Current device's type
    QString _activeDeviceType;

    /// The name of the output device
    QString _deviceName;

    /// The used baudrate of the output device
    int _baudRate_Hz;
};
