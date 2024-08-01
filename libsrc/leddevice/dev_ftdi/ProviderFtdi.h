#ifndef PROVIDERFtdi_H
#define PROVIDERFtdi_H

// LedDevice includes
#include <leddevice/LedDevice.h>

#include <ftdi.h>

///
/// The ProviderFtdi implements an abstract base-class for LedDevices using a Ftdi-device.
///
class ProviderFtdi : public LedDevice
{
	Q_OBJECT

public:

	///
	/// @brief Constructs a Ftdi LED-device
	///
	ProviderFtdi(const QJsonObject& deviceConfig);

	static const QString AUTO_SETTING;

protected:
	///
	/// @brief Opens the output device.
	///
	/// @return Zero on success (i.e. device is ready), else negative
	///
	int open() override;

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	bool init(const QJsonObject& deviceConfig) override;

	///
	/// @brief Closes the UDP device.
	///
	/// @return Zero on success (i.e. device is closed), else negative
	///
	int close() override;


	/// @brief Write the given bytes to the Ftdi-device
	///
	/// @param[in[ size The length of the data
	/// @param[in] data The data
	/// @return Zero on success, else negative
	///
	int writeBytes(const qint64 size, const uint8_t* data);

	
	QJsonObject discover(const QJsonObject& params) override;

	/// The Ftdi serial-device
	struct ftdi_context *_ftdic;

	/// The used baud-rate of the output device
	qint32 _baudRate_Hz;
	QString _deviceName;

protected slots:

	///
	/// @brief Set device in error state
	///
	/// @param errorMsg The error message to be logged
	///
	void setInError(const QString& errorMsg, bool isRecoverable=true) override;
};

#endif // PROVIDERFtdi_H
