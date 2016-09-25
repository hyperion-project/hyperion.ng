
// STL includes
#include <cstring>
#include <iostream>

// Qt includes
#include <QTimer>

// Local Hyperion includes
#include "ProviderRs232.h"

ProviderRs232::ProviderRs232(const Json::Value &deviceConfig)
	: _rs232Port(this)
	, _blockedForDelay(false)
	, _stateChanged(true)
{
	setConfig(deviceConfig);
	connect(&_rs232Port, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(error(QSerialPort::SerialPortError)));
}

bool ProviderRs232::setConfig(const Json::Value &deviceConfig)
{
	closeDevice();
	_deviceName           = deviceConfig["output"].asString();
	_baudRate_Hz          = deviceConfig["rate"].asInt();
	_delayAfterConnect_ms = deviceConfig.get("delayAfterConnect",250).asInt();

	return true;
}

void ProviderRs232::error(QSerialPort::SerialPortError error)
{
	if ( error != QSerialPort::NoError )
	{
		switch (error)
		{
			case QSerialPort::DeviceNotFoundError:
				Error(_log, "An error occurred while attempting to open an non-existing device."); break;
			case QSerialPort::PermissionError:
				Error(_log, "An error occurred while attempting to open an already opened device by another process or a user not having enough permission and credentials to open."); break;
			case QSerialPort::OpenError:
				Error(_log, "An error occurred while attempting to open an already opened device in this object."); break;
			case QSerialPort::NotOpenError:
				Error(_log, "This error occurs when an operation is executed that can only be successfully performed if the device is open."); break;
			case QSerialPort::ParityError:
				Error(_log, "Parity error detected by the hardware while reading data. This value is obsolete. We strongly advise against using it in new code."); break;
			case QSerialPort::FramingError:
				Error(_log, "Framing error detected by the hardware while reading data. This value is obsolete. We strongly advise against using it in new code."); break;
			case QSerialPort::BreakConditionError:
				Error(_log, "Break condition detected by the hardware on the input line. This value is obsolete. We strongly advise against using it in new code."); break;
			case QSerialPort::WriteError:
				Error(_log, "An I/O error occurred while writing the data."); break;
			case QSerialPort::ReadError:
				Error(_log, "An I/O error occurred while reading the data."); break;
			case QSerialPort::ResourceError:
				Error(_log, "An I/O error occurred when a resource becomes unavailable, e.g. when the device is unexpectedly removed from the system."); break;
			case QSerialPort::UnsupportedOperationError:
				Error(_log, "The requested device operation is not supported or prohibited by the running operating system."); break;
			case QSerialPort::TimeoutError:
				Error(_log, "A timeout error occurred."); break;
			default: Error(_log,"An unidentified error occurred. (%d)", error);
		}
		_rs232Port.clearError();
	}
}

ProviderRs232::~ProviderRs232()
{
	disconnect(&_rs232Port, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(error(QSerialPort::SerialPortError)));
	closeDevice();
}

void ProviderRs232::closeDevice()
{
	if (_rs232Port.isOpen())
	{
		_rs232Port.close();
		Debug(_log,"Close UART: %s", _deviceName.c_str());
	}
}

int ProviderRs232::open()
{
	Info(_log, "Opening UART: %s", _deviceName.c_str());
	_rs232Port.setPortName(_deviceName.c_str());

	return tryOpen() ? 0 : -1;
}


bool ProviderRs232::tryOpen()
{
	if ( ! _rs232Port.isOpen() )
	{
		if ( ! _rs232Port.open(QIODevice::WriteOnly) )
		{
			if ( _stateChanged )
			{
				Error(_log, "Unable to open RS232 device (%s)", _deviceName.c_str());
				_stateChanged = false;
			}
			return false;
		}
		Debug(_log, "Setting baud rate to %d", _baudRate_Hz);
		_rs232Port.setBaudRate(_baudRate_Hz);
		_stateChanged = true;
	}

	if (_delayAfterConnect_ms > 0)
	{
		_blockedForDelay = true;
		QTimer::singleShot(_delayAfterConnect_ms, this, SLOT(unblockAfterDelay()));
		Debug(_log, "Device blocked for %d ms", _delayAfterConnect_ms);
	}

	return _rs232Port.isOpen();
}


int ProviderRs232::writeBytes(const unsigned size, const uint8_t * data)
{
	if (_blockedForDelay)
	{
		return 0;
	}

	if (!_rs232Port.isOpen())
	{
		_delayAfterConnect_ms = 3000;
		return tryOpen() ? 0 : -1;
	}

	_rs232Port.flush();
	qint64 result = _rs232Port.write(reinterpret_cast<const char*>(data), size);
	if ( result < 0  || result != size)
	{
		return -1;
	}
	
	Debug(_log, "write %d ", result);
	_rs232Port.flush();

	return 0;
}


void ProviderRs232::unblockAfterDelay()
{
	Debug(_log, "Device unblocked");
	_blockedForDelay = false;
}
