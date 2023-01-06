#ifndef PROVIDERRS232_H
#define PROVIDERRS232_H

// LedDevice includes
#include <leddevice/LedDevice.h>

// qt includes
#include <QSerialPort>

///
/// The ProviderRs232 implements an abstract base-class for LedDevices using a RS232-device.
///
class ProviderRs232 : public LedDevice
{
	Q_OBJECT

public:

	///
	/// @brief Constructs a RS232 LED-device
	///
	ProviderRs232(const QJsonObject &deviceConfig);

	///
	/// @brief Destructor of the UDP LED-device
	///
	~ProviderRs232() override;

	///
	/// @brief Send an update to the RS232 device to identify it.
	///
	/// Following parameters are required
	/// @code
	/// {
	///     "deviceConfig"  : 
	/// }
	///@endcode
	///
	/// @param[in] params Parameters to configure device
	///
	void identify(const QJsonObject& params) override;

protected:

	///
	/// @brief Initialise the RS232 device's configuration and network address details
	///
	/// @param[in] deviceConfig the JSON device configuration
	/// @return True, if success
	///
	bool init(const QJsonObject &deviceConfig) override;

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
	/// @brief Power-/turn off a RS232-device
	///
	/// The off-state is simulated by writing "Black to LED"
	///
	/// @return True, if success
	///
	bool powerOff() override;

	///
	/// @brief Discover first device of serial devices available (for configuration)
	///
	/// @return A string of the device found
	///
	QString discoverFirst() override;

	///
	/// @brief Discover serial devices available (for configuration).
	///
	/// Following parameters can be provided optional
	/// @code
	/// {
	///     "discoverAll"  : true/false , "true", in case devices without vendor-id are to be included in the discovery result
	/// }
	///@endcode
	///
	/// @param[in] params Parameters used to overwrite discovery default behaviour
	///
	/// @return A JSON structure holding a list of devices found
	///
	QJsonObject discover(const QJsonObject& params) override;

	///
	/// @brief Write the given bytes to the RS232-device
	///
	/// @param[in[ size The length of the data
	/// @param[in] data The data
	/// @return Zero on success, else negative
	///
	int writeBytes(const qint64 size, const uint8_t *data);

	/// The name of the output device
	QString _deviceName;
	/// The system location of the output device
	QString _location;
	/// The RS232 serial-device
	QSerialPort _rs232Port;
	/// The used baud-rate of the output device
	qint32 _baudRate_Hz;

protected slots:

	///
	/// @brief Set device in error state
	///
	/// @param[in] errorMsg The error message to be logged
	/// @param[in] isRecoverable If False, no further retries will be done
	///
	void setInError( const QString& errorMsg, bool isRecoverable=true) override;

	///
	/// @brief Handle any feedback provided by the device
	///
	virtual void readFeedback();

private:

	///
	/// @brief Try to open device if not opened
	///
	/// @return True,if on success
	///
	bool tryOpen(int delayAfterConnect_ms);

	/// Try to auto-discover device name?
	bool _isAutoDeviceName;

	/// Sleep after the connect before continuing
	int _delayAfterConnect_ms;

	/// Frames dropped, as write failed
	int _frameDropCounter;
};

#endif // PROVIDERRS232_H
