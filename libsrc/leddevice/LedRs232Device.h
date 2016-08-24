#pragma once

#include <QObject>
#include <QSerialPort>

// Leddevice includes
#include <leddevice/LedDevice.h>

///
/// The LedRs232Device implements an abstract base-class for LedDevices using a RS232-device.
///
class LedRs232Device : public LedDevice
{
	Q_OBJECT

public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	LedRs232Device(const Json::Value &deviceConfig);

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	virtual bool setConfig(const Json::Value &deviceConfig);

	///
	/// Destructor of the LedDevice; closes the output device if it is open
	///
	virtual ~LedRs232Device();

	///
	/// Opens and configures the output device
	///
	/// @return Zero on succes else negative
	///
	int open();

protected:
	/**
	 * Writes the given bytes to the RS232-device and
	 *
	 * @param[in[ size The length of the data
	 * @param[in] data The data
	 *
	 * @return Zero on succes else negative
	 */
	int writeBytes(const unsigned size, const uint8_t *data);

	void closeDevice();

private slots:
	/// Unblock the device after a connection delay
	void unblockAfterDelay();
	void error(QSerialPort::SerialPortError error);

private:
	// tries to open device if not opened
	bool tryOpen();
	
	/// The name of the output device
	std::string _deviceName;

	/// The used baudrate of the output device
	qint32 _baudRate_Hz;

	/// Sleep after the connect before continuing
	int _delayAfterConnect_ms;

	/// The RS232 serial-device
	QSerialPort _rs232Port;

	bool _blockedForDelay;
	
	bool _stateChanged;
};
