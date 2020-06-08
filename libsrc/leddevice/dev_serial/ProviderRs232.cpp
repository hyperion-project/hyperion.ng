
// LedDevice includes
#include <leddevice/LedDevice.h>
#include "ProviderRs232.h"

// qt includes
#include <QSerialPortInfo>
#include <QEventLoop>

// Constants
const int WRITE_TIMEOUT	= 1000;		// device write timeout in ms
const int OPEN_TIMEOUT	= 5000;		// device open timeout in ms
const int MAX_WRITE_TIMEOUTS = 5;	// maximum number of allowed timeouts

const int NUM_POWEROFF_WRITE_BLACK = 2;	// Number of write "BLACK" during powering off

ProviderRs232::ProviderRs232()
	: _rs232Port(this)
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
		_isAutoDeviceName	  = _deviceName == "auto";
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
	Debug(_log, "");

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

	Debug(_log, "[%d]", retval);
	return retval;
}

int ProviderRs232::close()
{
	Debug(_log, "");

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

	Debug(_log, "[%d]", retval);
	return retval;
}

bool ProviderRs232::powerOff()
{
	Debug(_log, "");

	// Simulate power-off by writing a final "Black" to have a defined outcome
	bool rc = false;
	if ( writeBlack( NUM_POWEROFF_WRITE_BLACK ) >= 0 )
	{
		rc = true;
	}

	Debug(_log, "[%d]", rc);
	return rc;
}

QString ProviderRs232::discoverFirst()
{
	Debug(_log, "");

	// take first available USB serial port - currently no probing!
	for( auto port : QSerialPortInfo::availablePorts())
	{
		if (port.hasProductIdentifier() && port.hasVendorIdentifier() && !port.isBusy())
		{
			Info(_log, "found serial device: %s", QSTRING_CSTR(port.systemLocation()) );
			return port.systemLocation();
		}
	}
	return "";
}

bool ProviderRs232::tryOpen(const int delayAfterConnect_ms)
{
	Debug(_log, "");

	if (_deviceName.isEmpty() || _rs232Port.portName().isEmpty())
	{
		if ( _isAutoDeviceName )
		{
			_deviceName = discoverFirst();
			if ( _deviceName.isEmpty() )
			{
				this->setInError( QString ("No serial device found automatically!") );
				return false;
			}
		}
		Info(_log, "Opening UART: %s", QSTRING_CSTR(_deviceName));
		_rs232Port.setPortName(_deviceName);
	}

	if ( ! _rs232Port.isOpen() )
	{
		Debug(_log, "! _rs232Port.isOpen(): %s", QSTRING_CSTR(_deviceName) );
		_frameDropCounter = 0;

		_rs232Port.setBaudRate( _baudRate_Hz );

		Debug(_log, "_rs232Port.open(QIODevice::WriteOnly): %s", QSTRING_CSTR(_deviceName));
		if ( ! _rs232Port.open(QIODevice::WriteOnly) )
		{
			this->setInError( _rs232Port.errorString() );
			return false;
		}
	}

	if (delayAfterConnect_ms > 0)
	{

		Debug(_log, "delayAfterConnect for %d ms - start", delayAfterConnect_ms);

		// Wait delayAfterConnect_ms before allowing write
		QEventLoop loop;
		QTimer::singleShot( delayAfterConnect_ms, &loop, SLOT( quit() ) );
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

int ProviderRs232::writeBytes(const qint64 size, const uint8_t * data)
{
	DebugIf(_isInSwitchOff, _log, "_inClosing [%d], enabled [%d], _deviceReady [%d], _frameDropCounter [%d]", _isInSwitchOff, this->isEnabled(), _isDeviceReady, _frameDropCounter);

	int rc = 0;
	if (!_rs232Port.isOpen())
	{
		Debug(_log, "!_rs232Port.isOpen()");

		if ( !tryOpen(OPEN_TIMEOUT) )
		{
			return -1;
		}
	}

	DebugIf(_isInSwitchOff, _log, "[%s]", uint8_t_to_hex_string(size,data, 32 ).c_str() );

	qint64 bytesWritten = _rs232Port.write(reinterpret_cast<const char*>(data), size);
	if (bytesWritten == -1 || bytesWritten != size)
	{
		this->setInError( QString ("Rs232 SerialPortError: %1").arg(_rs232Port.errorString()) );
		rc = -1;
	}
	else
	{
		if (!_rs232Port.waitForBytesWritten(WRITE_TIMEOUT))
		{
			if ( _rs232Port.error() == QSerialPort::TimeoutError )
			{
				Debug(_log, "Timeout after %dms: %d frames already dropped", WRITE_TIMEOUT, _frameDropCounter);

				++_frameDropCounter;

				// Check,if number of timeouts in a given time frame is greater than defined
				// TODO: Add time frame
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

QJsonObject ProviderRs232::discover()
{
	QJsonObject devicesDiscovered;
	devicesDiscovered.insert("ledDeviceType", _activeDeviceType );

	QJsonArray deviceList;

	// Discover serial Devices
	for (auto & port : QSerialPortInfo::availablePorts() )
	{
		if (port.hasProductIdentifier() && port.hasVendorIdentifier() )
		{
			QJsonObject portInfo;
			portInfo.insert("description",port.description());
			portInfo.insert("manufacturer",port.manufacturer());
			portInfo.insert("portName",port.portName());
			portInfo.insert("productIdentifier", QString("0x%1").arg(port.productIdentifier(),0,16));
			portInfo.insert("serialNumber",port.serialNumber());
			portInfo.insert("systemLocation",port.systemLocation());
			portInfo.insert("vendorIdentifier", QString("0x%1").arg(port.vendorIdentifier(),0,16));

			deviceList.append(portInfo);
		}
	}

	devicesDiscovered.insert("devices", deviceList);
	return devicesDiscovered;
}
