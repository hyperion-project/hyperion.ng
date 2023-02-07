
// LedDevice includes
#include <leddevice/LedDevice.h>
#include "ProviderRs232.h"
#include <utils/WaitTime.h>

// qt includes
#include <QSerialPortInfo>
#include <QEventLoop>
#include <QDir>

#include <chrono>

// Constants
namespace {
	const bool verbose = false;

	constexpr std::chrono::milliseconds WRITE_TIMEOUT{ 1000 };	// device write timeout in ms
	constexpr std::chrono::milliseconds OPEN_TIMEOUT{ 5000 };		// device open timeout in ms
	const int MAX_WRITE_TIMEOUTS = 5;	// Maximum number of allowed timeouts
	const int NUM_POWEROFF_WRITE_BLACK = 5;	// Number of write "BLACK" during powering off

	constexpr std::chrono::milliseconds DEFAULT_IDENTIFY_TIME{ 500 };

	// tty discovery service
	const char DISCOVERY_DIRECTORY[] = "/dev/";
	const char DISCOVERY_FILEPATTERN[] = "tty*";
} //End of constants

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
		_deviceName           = deviceConfig["output"].toString("auto");
		_isAutoDeviceName     = _deviceName.toLower() == "auto";

		// If device name was given as unix /dev/ system-location, get port name
		if ( _deviceName.startsWith(QLatin1String("/dev/")) )
		{
			_location = _deviceName;
			//Handle udev devices
			QFileInfo file_info(_deviceName);
			if (file_info.isSymLink())
			{
				_deviceName = file_info.symLinkTarget();
			}
			_deviceName = _deviceName.mid(5);
		}

		_baudRate_Hz = deviceConfig["rate"].toInt();
		_delayAfterConnect_ms = deviceConfig["delayAfterConnect"].toInt(1500);

		Debug(_log, "DeviceName   : %s", QSTRING_CSTR(_deviceName));
		DebugIf(!_location.isEmpty(), _log, "Location     : %s", QSTRING_CSTR(_location));
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

	// open device physically
	if ( tryOpen(_delayAfterConnect_ms) )
	{
		connect(&_rs232Port, &QSerialPort::readyRead, this, &ProviderRs232::readFeedback);

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

		disconnect(&_rs232Port, &QSerialPort::readyRead, this, &ProviderRs232::readFeedback);

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
		if (!_location.isEmpty())
		{
			Info(_log, "Opening UART: %s (%s)", QSTRING_CSTR(_deviceName), QSTRING_CSTR(_location));
		}
		else
		{
			Info(_log, "Opening UART: %s", QSTRING_CSTR(_deviceName));
		}

		_frameDropCounter = 0;

		_rs232Port.setBaudRate( _baudRate_Hz );

		Debug(_log, "_rs232Port.open(QIODevice::ReadWrite): %s, Baud rate [%d]bps", QSTRING_CSTR(_deviceName), _baudRate_Hz);

		QSerialPortInfo serialPortInfo(_deviceName);
		if (!serialPortInfo.isNull() )
		{
			Debug(_log, "portName:          %s", QSTRING_CSTR(serialPortInfo.portName()));
			Debug(_log, "systemLocation:    %s", QSTRING_CSTR(serialPortInfo.systemLocation()));
			Debug(_log, "description:       %s", QSTRING_CSTR(serialPortInfo.description()));
			Debug(_log, "manufacturer:      %s", QSTRING_CSTR(serialPortInfo.manufacturer()));
			Debug(_log, "vendorIdentifier:  %s", QSTRING_CSTR(QString("0x%1").arg(serialPortInfo.vendorIdentifier(), 0, 16)));
			Debug(_log, "productIdentifier: %s", QSTRING_CSTR(QString("0x%1").arg(serialPortInfo.productIdentifier(), 0, 16)));
			Debug(_log, "serialNumber:      %s", QSTRING_CSTR(serialPortInfo.serialNumber()));

			if ( !_rs232Port.open(QIODevice::ReadWrite) )
			{
				this->setInError(_rs232Port.errorString());
				return false;
			}
		}
		else
		{
			QString errortext = QString("Invalid serial device name: %1 %2!").arg(_deviceName, _location);
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

void ProviderRs232::setInError(const QString& errorMsg, bool isRecoverable)
{
	_rs232Port.clearError();
	this->close();

	LedDevice::setInError( errorMsg, isRecoverable );
}

int ProviderRs232::writeBytes(const qint64 size, const uint8_t *data)
{
	int rc = 0;
	if (!_rs232Port.isOpen())
	{
		Debug(_log, "!_rs232Port.isOpen()");

		if ( !tryOpen(OPEN_TIMEOUT.count()) )
		{
			return -1;
		}
	}
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
				Debug(_log, "Timeout after %dms: %d frames already dropped", WRITE_TIMEOUT.count(), _frameDropCounter);

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
	}
	return rc;
}

void ProviderRs232::readFeedback()
{
	QByteArray readData = _rs232Port.readAll();
	if (!readData.isEmpty())
	{
		//Output as received
		std::cout << readData.toStdString();
	}
}

QString ProviderRs232::discoverFirst()
{
	// take first available USB serial port - currently no probing!
	for (auto & port : QSerialPortInfo::availablePorts())
	{
		if (!port.isNull())
		{
			Info(_log, "found serial device: %s", QSTRING_CSTR(port.portName()));
			return port.portName();
		}
	}
	return "";
}

QJsonObject ProviderRs232::discover(const QJsonObject& params)
{
	DebugIf(verbose,_log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());

	QJsonObject devicesDiscovered;
	devicesDiscovered.insert("ledDeviceType", _activeDeviceType );

	QJsonArray deviceList;

	bool showAll = params["discoverAll"].toBool(false);

	// Discover serial Devices
	for (auto &port : QSerialPortInfo::availablePorts() )
	{
		if ( !port.isNull() && (showAll || port.vendorIdentifier() != 0) )
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

#ifndef _WIN32
	//Check all /dev/tty* files, if they are udev-serial devices
	QDir deviceDirectory (DISCOVERY_DIRECTORY);
	QStringList deviceFilter(DISCOVERY_FILEPATTERN);
	deviceDirectory.setNameFilters(deviceFilter);
	deviceDirectory.setSorting(QDir::Name);
	QFileInfoList deviceFiles = deviceDirectory.entryInfoList(QDir::AllEntries);

	QFileInfoList::const_iterator deviceFileIterator;
	for (deviceFileIterator = deviceFiles.constBegin(); deviceFileIterator != deviceFiles.constEnd(); ++deviceFileIterator)
	{
		if ((*deviceFileIterator).isSymLink())
		{
			QSerialPortInfo port = QSerialPortInfo(QSerialPort((*deviceFileIterator).symLinkTarget()));

			QJsonObject portInfo;
			portInfo.insert("portName", (*deviceFileIterator).fileName());
			portInfo.insert("systemLocation", (*deviceFileIterator).absoluteFilePath());
			portInfo.insert("udev", true);

			portInfo.insert("description", port.description());
			portInfo.insert("manufacturer", port.manufacturer());
			portInfo.insert("productIdentifier", QString("0x%1").arg(port.productIdentifier(), 0, 16));
			portInfo.insert("serialNumber", port.serialNumber());
			portInfo.insert("vendorIdentifier", QString("0x%1").arg(port.vendorIdentifier(), 0, 16));

			deviceList.append(portInfo);
		}
	}
#endif

	devicesDiscovered.insert("devices", deviceList);
	DebugIf(verbose,_log, "devicesDiscovered: [%s]", QString(QJsonDocument(devicesDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

	return devicesDiscovered;
}

void ProviderRs232::identify(const QJsonObject& params)
{
	DebugIf(verbose,_log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());

	QString deviceName = params["output"].toString("");
	if (!deviceName.isEmpty())
	{
		Info(_log, "Identify %s, device: %s", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(deviceName) );

		_devConfig = params;
		init(_devConfig);
		{
			if ( open() == 0 )
			{
				for (int i = 0; i < 2; ++i)
				{
					if (writeColor(ColorRgb::RED) == 0)
					{
						wait(DEFAULT_IDENTIFY_TIME);

						writeColor(ColorRgb::BLACK);
						wait(DEFAULT_IDENTIFY_TIME);
					}
					else
					{
						break;
					}
				}
				close();
			}
		}
	}
}
