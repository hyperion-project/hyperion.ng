
// LedDevice includes
#include <leddevice/LedDevice.h>
#include "ProviderRs232.h"

// qt includes
#include <QSerialPortInfo>
#include <QEventLoop>

#include <chrono>

// Constants
constexpr std::chrono::milliseconds WRITE_TIMEOUT{1000};	// device write timeout in ms
constexpr std::chrono::milliseconds OPEN_TIMEOUT{5000};		// device open timeout in ms
const int MAX_WRITE_TIMEOUTS = 5;	// Maximum number of allowed timeouts
const int NUM_POWEROFF_WRITE_BLACK = 2;	// Number of write "BLACK" during powering off

ProviderRs232::ProviderRs232(const QJsonObject &deviceConfig)
	: LedDevice(deviceConfig)
	  , _rs232Port(this)
	  ,_baudRate_Hz(1000000)
	  ,_isAutoDeviceName(false)
	  ,_delayAfterConnect_ms(0)
	  ,_frameDropCounter(0)
{
}

bool ProviderRs232::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = false;

	// Initialise sub-class
	if ( LedDevice::init(deviceConfig) )
	{

		Debug(_log, "DeviceType   : %s", QSTRING_CSTR( this->getActiveDeviceType() ));
		Debug(_log, "LedCount     : %u", this->getLedCount());
		Debug(_log, "ColorOrder   : %s", QSTRING_CSTR( this->getColorOrder() ));
		Debug(_log, "RefreshTime  : %d", _refreshTimerInterval_ms);
		Debug(_log, "LatchTime    : %d", this->getLatchTime());

		_deviceName           = deviceConfig["output"].toString("auto");

		// If device name was given as unix /dev/ system-location, get port name
		if ( _deviceName.startsWith(QLatin1String("/dev/")) )
			_deviceName = _deviceName.mid(5);

		_isAutoDeviceName     = _deviceName.toLower() == "auto";
		_baudRate_Hz          = deviceConfig["rate"].toInt();
		_delayAfterConnect_ms = deviceConfig["delayAfterConnect"].toInt(1500);

		Debug(_log, "deviceName   : %s", QSTRING_CSTR(_deviceName));
		Debug(_log, "AutoDevice   : %d", _isAutoDeviceName);
		Debug(_log, "baudRate_Hz  : %d", _baudRate_Hz);
		Debug(_log, "delayAfCon ms: %d", _delayAfterConnect_ms);

		isInitOK = true;
	}
	return isInitOK;
}

ProviderRs232::~ProviderRs232()
{
}

int ProviderRs232::open()
{
	int retval = -1;
	_isDeviceReady = false;
	_isInSwitchOff = false;

	// open device physically
	if ( tryOpen(_delayAfterConnect_ms) )
	{
		// Everything is OK, device is ready
		_isDeviceReady = true;
		retval = 0;
	}
	return retval;
}

int ProviderRs232::close()
{
	int retval = 0;

	_isDeviceReady = false;

	// Test, if device requires closing
	if (_rs232Port.isOpen())
	{
		if ( _rs232Port.flush() )
		{
			Debug(_log,"Flush was successful");
		}
		Debug(_log,"Close UART: %s", QSTRING_CSTR(_deviceName) );
		_rs232Port.close();
		// Everything is OK -> device is closed
	}
	return retval;
}

bool ProviderRs232::powerOff()
{
	// Simulate power-off by writing a final "Black" to have a defined outcome
	bool rc = false;
	if ( writeBlack( NUM_POWEROFF_WRITE_BLACK ) >= 0 )
	{
		rc = true;
	}
	return rc;
}

bool ProviderRs232::tryOpen(int delayAfterConnect_ms)
{
	if (_deviceName.isEmpty() || _rs232Port.portName().isEmpty())
	{
		if (!_rs232Port.isOpen())
		{
			if ( _isAutoDeviceName )
			{
				_deviceName = discoverFirst();
				if (_deviceName.isEmpty())
				{
					this->setInError( QString("No serial device found automatically!") );
					return false;
				}
			}
		}

		_rs232Port.setPortName(_deviceName);
	}

	if (!_rs232Port.isOpen())
	{
		Info(_log, "Opening UART: %s", QSTRING_CSTR(_deviceName));

		_frameDropCounter = 0;

		_rs232Port.setBaudRate( _baudRate_Hz );

		Debug(_log, "_rs232Port.open(QIODevice::WriteOnly): %s, Baud rate [%d]bps", QSTRING_CSTR(_deviceName), _baudRate_Hz);

		QSerialPortInfo serialPortInfo(_deviceName);

		QJsonObject portInfo;
		Debug(_log, "portName:          %s", QSTRING_CSTR(serialPortInfo.portName()));
		Debug(_log, "systemLocation:    %s", QSTRING_CSTR(serialPortInfo.systemLocation()));
		Debug(_log, "description:       %s", QSTRING_CSTR(serialPortInfo.description()));
		Debug(_log, "manufacturer:      %s", QSTRING_CSTR(serialPortInfo.manufacturer()));
		Debug(_log, "productIdentifier: %s", QSTRING_CSTR(QString("0x%1").arg(serialPortInfo.productIdentifier(), 0, 16)));
		Debug(_log, "vendorIdentifier:  %s", QSTRING_CSTR(QString("0x%1").arg(serialPortInfo.vendorIdentifier(), 0, 16)));
		Debug(_log, "serialNumber:      %s", QSTRING_CSTR(serialPortInfo.serialNumber()));

		if (!serialPortInfo.isNull() )
		{
			if ( !_rs232Port.open(QIODevice::WriteOnly) )
			{
				this->setInError(_rs232Port.errorString());
				return false;
			}
		}
		else
		{
			QString errortext = QString("Invalid serial device name: [%1]!").arg(_deviceName);
			this->setInError( errortext );
			return false;
		}
	}

	if (delayAfterConnect_ms > 0)
	{

		Debug(_log, "delayAfterConnect for %d ms - start", delayAfterConnect_ms);

		// Wait delayAfterConnect_ms before allowing write
		QEventLoop loop;
		QTimer::singleShot(delayAfterConnect_ms, &loop, &QEventLoop::quit);
		loop.exec();

		Debug(_log, "delayAfterConnect for %d ms - finished", delayAfterConnect_ms);
	}

	return _rs232Port.isOpen();
}

void ProviderRs232::setInError(const QString& errorMsg)
{
	_rs232Port.clearError();
	this->close();

	LedDevice::setInError( errorMsg );
}

int ProviderRs232::writeBytes(const qint64 size, const uint8_t *data)
{
	DebugIf(_isInSwitchOff, _log, "_inClosing [%d], enabled [%d], _deviceReady [%d], _frameDropCounter [%d]", _isInSwitchOff, this->isEnabled(), _isDeviceReady, _frameDropCounter);

	int rc = 0;
	if (!_rs232Port.isOpen())
	{
		Debug(_log, "!_rs232Port.isOpen()");

		if ( !tryOpen(OPEN_TIMEOUT.count()) )
		{
			return -1;
		}
	}

	DebugIf( _isInSwitchOff, _log, "[%s]", QSTRING_CSTR(uint8_t_to_hex_string(data, size, 32)) );

	qint64 bytesWritten = _rs232Port.write(reinterpret_cast<const char*>(data), size);
	if (bytesWritten == -1 || bytesWritten != size)
	{
		this->setInError( QString ("Rs232 SerialPortError: %1").arg(_rs232Port.errorString()) );
		rc = -1;
	}
	else
	{
		if (!_rs232Port.waitForBytesWritten(WRITE_TIMEOUT.count()))
		{
			if ( _rs232Port.error() == QSerialPort::TimeoutError )
			{
				Debug(_log, "Timeout after %dms: %d frames already dropped", WRITE_TIMEOUT, _frameDropCounter);

				++_frameDropCounter;

				// Check,if number of timeouts in a given time frame is greater than defined
				// TODO: ProviderRs232::writeBytes - Add time frame to check for timeouts that devices does not close after absolute number of timeouts
				if ( _frameDropCounter > MAX_WRITE_TIMEOUTS )
				{
					this->setInError( QString ("Timeout writing data to %1").arg(_deviceName) );
					rc = -1;
				}
				else
				{
					//give it another try
					_rs232Port.clearError();
				}
			}
			else
			{
				this->setInError( QString ("Rs232 SerialPortError: %1").arg(_rs232Port.errorString()) );
				rc = -1;
			}
		}
		else
		{
			DebugIf(_isInSwitchOff,_log, "In Closing: bytesWritten [%d], _rs232Port.error() [%d], %s", bytesWritten, _rs232Port.error(), _rs232Port.error() == QSerialPort::NoError ? "No Error" : QSTRING_CSTR(_rs232Port.errorString()) );
		}
	}

	DebugIf(_isInSwitchOff, _log, "[%d], _inClosing[%d], enabled [%d], _deviceReady [%d]", rc, _isInSwitchOff, this->isEnabled(), _isDeviceReady);
	return rc;
}

QString ProviderRs232::discoverFirst()
{
	// take first available USB serial port - currently no probing!
	for (auto const & port : QSerialPortInfo::availablePorts())
	{
		if (!port.isNull() && !port.isBusy())
		{
			Info(_log, "found serial device: %s", QSTRING_CSTR(port.portName()));
			return port.portName();
		}
	}
	return "";
}

QJsonObject ProviderRs232::discover()
{
	QJsonObject devicesDiscovered;
	devicesDiscovered.insert("ledDeviceType", _activeDeviceType );

	QJsonArray deviceList;

	// Discover serial Devices
	for (auto &port : QSerialPortInfo::availablePorts() )
	{
		if ( !port.isNull() )
		{
			QJsonObject portInfo;
			portInfo.insert("description", port.description());
			portInfo.insert("manufacturer", port.manufacturer());
			portInfo.insert("portName", port.portName());
			portInfo.insert("productIdentifier", QString("0x%1").arg(port.productIdentifier(), 0, 16));
			portInfo.insert("serialNumber", port.serialNumber());
			portInfo.insert("systemLocation", port.systemLocation());
			portInfo.insert("vendorIdentifier", QString("0x%1").arg(port.vendorIdentifier(), 0, 16));

			deviceList.append(portInfo);
		}
	}

	devicesDiscovered.insert("devices", deviceList);
	return devicesDiscovered;
}
