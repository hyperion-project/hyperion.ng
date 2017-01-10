
// STL includes
#include <cstring>
#include <iostream>

// Qt includes
#include <QTimer>

// Local Hyperion includes
#include "ProviderRs232.h"

ProviderRs232::ProviderRs232()
	: _rs232Port(this)
	, _blockedForDelay(false)
	, _stateChanged(true)
	, _bytesToWrite(0)
	, _bytesWritten(0)
	, _frameDropCounter(0)
	, _lastError(QSerialPort::NoError)
{
	connect(&_rs232Port, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(error(QSerialPort::SerialPortError)));
	connect(&_rs232Port, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWritten(qint64)));
	connect(&_rs232Port, SIGNAL(readyRead()), this, SLOT(readyRead()));
}

bool ProviderRs232::init(const QJsonObject &deviceConfig)
{
	closeDevice();

	LedDevice::init(deviceConfig);

	_deviceName           = deviceConfig["output"].toString().toStdString();
	_baudRate_Hz          = deviceConfig["rate"].toInt();
	_delayAfterConnect_ms = deviceConfig["delayAfterConnect"].toInt(250);

	return true;
}

void ProviderRs232::bytesWritten(qint64 bytes)
{
	_bytesWritten += bytes;
	if (_bytesWritten >= _bytesToWrite)
	{
		_bytesToWrite = 0;
		_blockedForDelay = false;
	}
}


void ProviderRs232::readyRead()
{
	QByteArray data = _rs232Port.readAll();
	Debug(_log, "received %d bytes data", data.size());
}


void ProviderRs232::error(QSerialPort::SerialPortError error)
{
	if ( error != QSerialPort::NoError )
	{
		if (_lastError != error)
		{
			_lastError = error;
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
				Error(_log, "Parity error detected by the hardware while reading data."); break;
			case QSerialPort::FramingError:
				Error(_log, "Framing error detected by the hardware while reading data."); break;
			case QSerialPort::BreakConditionError:
				Error(_log, "Break condition detected by the hardware on the input line."); break;
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
			closeDevice();
		}
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

	return tryOpen(_delayAfterConnect_ms) ? 0 : -1;
}


bool ProviderRs232::tryOpen(const int delayAfterConnect_ms)
{
	if ( ! _rs232Port.isOpen() )
	{
		if ( ! _rs232Port.open(QIODevice::ReadWrite) )
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

	if (delayAfterConnect_ms > 0)
	{
		_blockedForDelay = true;
		QTimer::singleShot(delayAfterConnect_ms, this, SLOT(unblockAfterDelay()));
		Debug(_log, "Device blocked for %d ms", delayAfterConnect_ms);
	}

	return _rs232Port.isOpen();
}


int ProviderRs232::writeBytes(const qint64 size, const uint8_t * data)
{
	if (! _blockedForDelay)
	{
		if (!_rs232Port.isOpen())
		{
			return tryOpen(5000) ? 0 : -1;
		}

		if (_frameDropCounter > 5)
		{
			Debug(_log, "%d frames dropped", _frameDropCounter);
		}
		_frameDropCounter = 0;
		_blockedForDelay = true;
		_bytesToWrite = size;
		qint64 bytesWritten = _rs232Port.write(reinterpret_cast<const char*>(data), size);
		if (bytesWritten == -1 || bytesWritten != size)
		{
			Warning(_log,"failed writing data");
			QTimer::singleShot(500, this, SLOT(unblockAfterDelay()));
			return -1;
		}
		QTimer::singleShot(5000, this, SLOT(unblockAfterDelay()));
	}
	else
	{
		_frameDropCounter++;
	}

	return 0;
}


void ProviderRs232::unblockAfterDelay()
{
	_blockedForDelay = false;
}

int ProviderRs232::rewriteLeds()
{
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
