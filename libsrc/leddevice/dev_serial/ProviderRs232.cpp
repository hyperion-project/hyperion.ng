
// STL includes
#include <cstring>
#include <iostream>

// Qt includes
#include <QTimer>
#include <QDateTime>
#include <QFile>
#include <QSerialPortInfo>

// Local Hyperion includes
#include "ProviderRs232.h"

ProviderRs232::ProviderRs232()
	: _rs232Port(this)
	, _writeTimeout(this)
	, _blockedForDelay(false)
	, _stateChanged(true)
	, _bytesToWrite(0)
	, _frameDropCounter(0)
	, _lastError(QSerialPort::NoError)
	, _preOpenDelayTimeOut(0)
	, _preOpenDelay(2000)
	, _enableAutoDeviceName(false)
{
	connect(&_rs232Port, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(error(QSerialPort::SerialPortError)));
	connect(&_rs232Port, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWritten(qint64)));
	connect(&_rs232Port, SIGNAL(readyRead()), this, SLOT(readyRead()));
	_writeTimeout.setInterval(5000);
	_writeTimeout.setSingleShot(true);
	connect(&_writeTimeout, SIGNAL(timeout()), this, SLOT(writeTimeout()));
}

bool ProviderRs232::init(const QJsonObject &deviceConfig)
{
	closeDevice();

	LedDevice::init(deviceConfig);

	_deviceName           = deviceConfig["output"].toString("auto");
	_enableAutoDeviceName = _deviceName == "auto";
	_baudRate_Hz          = deviceConfig["rate"].toInt();
	_delayAfterConnect_ms = deviceConfig["delayAfterConnect"].toInt(1500);
	_preOpenDelay         = deviceConfig["delayBeforeConnect"].toInt(1500);

	return true;
}

QString ProviderRs232::findSerialDevice()
{
	// take first available usb serial port - currently no probing!
	for( auto port : QSerialPortInfo::availablePorts())
	{
		if (port.hasProductIdentifier() && port.hasVendorIdentifier() && !port.isBusy())
		{
			Info(_log, "found serial device: %s", port.systemLocation().toLocal8Bit().constData());
			return port.systemLocation();
			break;
		}
	}
	return "";
}

void ProviderRs232::bytesWritten(qint64 bytes)
{
	_bytesToWrite -= bytes;
	if (_bytesToWrite <= 0)
	{
		_blockedForDelay = false;
		_writeTimeout.stop();
	}
}


void ProviderRs232::readyRead()
{
	emit receivedData(_rs232Port.readAll());
	//Debug(_log, "received data");
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
				Error(_log, "An error occurred while attempting to open an already opened device by another process or a user not having enough permission and credentials to open. Device disabled.");
				_deviceReady = false;
				break;
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
				Error(_log, "The requested device operation is not supported or prohibited by the running operating system. Device disabled.");
				_deviceReady = false;
				break;
			case QSerialPort::TimeoutError:
				Error(_log, "A timeout error occurred."); break;
			default: 
				Error(_log,"An unidentified error occurred. Device disabled. (%d)", error);
				_deviceReady = false;
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
	_writeTimeout.stop();

	if (_rs232Port.isOpen())
	{
		_rs232Port.close();
		Debug(_log,"Close UART: %s", _deviceName.toLocal8Bit().constData());
	}

	_stateChanged = true;
	_bytesToWrite = 0;
	_blockedForDelay = false;
	_deviceReady = false;
}

int ProviderRs232::open()
{
	return tryOpen(_delayAfterConnect_ms) ? 0 : -1;
}


bool ProviderRs232::tryOpen(const int delayAfterConnect_ms)
{
	if (_deviceName.isEmpty() || _rs232Port.portName().isEmpty())
	{
		if ( _enableAutoDeviceName )
		{
			_deviceName = findSerialDevice();
			if ( _deviceName.isEmpty() )
			{
				return false;
			}
		}
		Info(_log, "Opening UART: %s", _deviceName.toLocal8Bit().constData());
		_rs232Port.setPortName(_deviceName);
	}

	if ( ! _rs232Port.isOpen() )
	{
		_frameDropCounter = 0;
		if (QFile::exists(_deviceName))
		{
			if ( _preOpenDelayTimeOut > QDateTime::currentMSecsSinceEpoch() )
			{
				return false;
			}
			if ( ! _rs232Port.open(QIODevice::ReadWrite) )
			{
				if ( _stateChanged )
				{
					Error(_log, "Unable to open RS232 device (%s)", _deviceName.toLocal8Bit().constData());
					_stateChanged = false;
				}
				return false;
			}
			Debug(_log, "Setting baud rate to %d", _baudRate_Hz);
			_rs232Port.setBaudRate(_baudRate_Hz);
			_stateChanged = true;
			_preOpenDelayTimeOut = 0;
		}
		else
		{
			_preOpenDelayTimeOut = QDateTime::currentMSecsSinceEpoch() + _preOpenDelay;
			return false;
		}
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
		_writeTimeout.start();
	}
	else
	{
		_frameDropCounter++;
	}

	return 0;
}

void ProviderRs232::writeTimeout()
{
	Error(_log, "Timeout on write data to %s", _deviceName.toLocal8Bit().constData());
	closeDevice();
}

void ProviderRs232::unblockAfterDelay()
{
	_blockedForDelay = false;
}

int ProviderRs232::rewriteLeds()
{
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
