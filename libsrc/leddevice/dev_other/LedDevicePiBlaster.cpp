
// STL includes
#include <cstring>
#include <csignal>

// QT includes
#include <QDir>
#include <QFile>

// Local LedDevice includes
#include "LedDevicePiBlaster.h"

// Constants
namespace {
	const bool verbose = false;

	// Pi-Blaster discovery service
	const char DISCOVERY_DIRECTORY[] = "/dev/";
	const char DISCOVERY_FILEPATTERN[] = "pi-blaster";

} //End of constants

LedDevicePiBlaster::LedDevicePiBlaster(const QJsonObject &deviceConfig)
	: LedDevice(deviceConfig)
	, _fid(nullptr)
{
	// initialise the mapping tables
	// -1 is invalid
	// z is also meaningless
	// { "gpio" : 4, "ledindex" : 0, "ledcolor" : "r" },
	#define TABLE_SZ sizeof(_gpio_to_led)/sizeof(_gpio_to_led[0])

	for (unsigned i=0; i <  TABLE_SZ; i++ )
	{
		_gpio_to_led[i] = -1;
		_gpio_to_color[i] = 'z';
	}
}

LedDevicePiBlaster::~LedDevicePiBlaster()
{
	if (_fid != nullptr)
	{
		fclose(_fid);
		_fid = nullptr;
	}
}

bool LedDevicePiBlaster::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = LedDevice::init(deviceConfig);

	_deviceName    = deviceConfig["output"].toString("/dev/pi-blaster");

	if ( isInitOK )
	{
		QJsonArray gpioMapping = deviceConfig["gpiomap"].toArray();

		if (gpioMapping.isEmpty())
		{
			this->setInError("PiBlaster: no gpiomap defined.");
			return false;
		}

		// walk through the JSON configuration and populate the mapping tables
		for(QJsonArray::const_iterator gpioArray = gpioMapping.begin(); gpioArray != gpioMapping.end(); ++gpioArray)
		{
			const QJsonObject value = (*gpioArray).toObject();
			const int gpio = value["gpio"].toInt(-1);
			const int ledindex = value["ledindex"].toInt(-1);
			const std::string ledcolor = value["ledcolor"].toString("z").toStdString();

			// ignore missing/invalid settings
			if ( (gpio >= 0) && (gpio < signed(TABLE_SZ)) && (ledindex >= 0) ){
				_gpio_to_led[gpio] = ledindex;
				_gpio_to_color[gpio] = ledcolor[0]; // 1st char of string
			} else {
				Warning( _log, "IGNORING gpio %d ledindex %d color %c", gpio,ledindex, ledcolor[0]);
			}
		}
	}
	return isInitOK;
}

LedDevice* LedDevicePiBlaster::construct(const QJsonObject &deviceConfig)
{
	return new LedDevicePiBlaster(deviceConfig);
}

int LedDevicePiBlaster::open()
{
	int retval = -1;
	QString errortext;
	_isDeviceReady = false;

	if (_fid != nullptr)
	{
		// The file pointer is already open
		errortext = QString ("Device (%1) is already open.").arg(_deviceName);
	}
	else
	{
		if (!QFile::exists(_deviceName))
		{
			errortext = QString ("The device (%1) does not yet exist.").arg(_deviceName);
		}
		else
		{
			_fid = fopen(QSTRING_CSTR(_deviceName), "w");
			if (_fid == nullptr)
			{
				errortext = QString ("Failed to open device (%1). Error message: %2").arg(_deviceName, strerror(errno));
			}
			else
			{
				Info( _log, "Connected to device(%s)", QSTRING_CSTR(_deviceName));

				// Everything is OK, device is ready
				_isDeviceReady = true;
				retval = 0;
			}
		}
	}

	// On error/exceptions, set LedDevice in error
	if ( retval < 0 )
	{
		this->setInError( errortext );
	}
	return retval;
}

int LedDevicePiBlaster::close()
{
	int retval = 0;
	_isDeviceReady = false;

	// Test, if device requires closing
	if (_fid != nullptr)
	{
		fclose(_fid);
		_fid = nullptr;
	}
	return retval;
}

int LedDevicePiBlaster::write(const std::vector<ColorRgb> & ledValues)
{
	// Attempt to open if not yet opened
	if (_fid == nullptr && open() < 0)
	{
		return -1;
	}

	for (unsigned int i=0; i < TABLE_SZ; i++ )
	{
		int valueIdx = _gpio_to_led[ i ];
		if ( (valueIdx >= 0) && (valueIdx < static_cast<int>( _ledCount)) )
		{
			double pwmDutyCycle = 0.0;
			switch (_gpio_to_color[ i ])
			{
				case 'r':
					pwmDutyCycle = ledValues[valueIdx].red / 255.0;
					break;
				case 'g':
					pwmDutyCycle = ledValues[valueIdx].green / 255.0;
					break;
				case 'b':
					pwmDutyCycle = ledValues[valueIdx].blue / 255.0;
					break;
				case 'w':
					pwmDutyCycle = ledValues[valueIdx].red;
					pwmDutyCycle += ledValues[valueIdx].green;
					pwmDutyCycle += ledValues[valueIdx].blue;
					pwmDutyCycle /= (3.0*255.0);
					break;
				default:
					continue;
			}

			if ( (fprintf(_fid, "%i=%f\n", i, pwmDutyCycle) < 0) || (fflush(_fid) < 0))
			{
				if (_fid != nullptr)
				{
					fclose(_fid);
					_fid = nullptr;
					return -1;
				}
			}
		}
	}

	return 0;
}

QJsonObject LedDevicePiBlaster::discover(const QJsonObject& /*params*/)
{
	QJsonObject devicesDiscovered;
	devicesDiscovered.insert("ledDeviceType", _activeDeviceType );

	QJsonArray deviceList;

	QDir deviceDirectory (DISCOVERY_DIRECTORY);
	QStringList deviceFilter(DISCOVERY_FILEPATTERN);
	deviceDirectory.setNameFilters(deviceFilter);
	deviceDirectory.setSorting(QDir::Name);
	QFileInfoList deviceFiles = deviceDirectory.entryInfoList(QDir::System);

	QFileInfoList::const_iterator deviceFileIterator;
	for (deviceFileIterator = deviceFiles.constBegin(); deviceFileIterator != deviceFiles.constEnd(); ++deviceFileIterator)
	{
		QJsonObject deviceInfo;
		deviceInfo.insert("deviceName", (*deviceFileIterator).fileName());
		deviceInfo.insert("systemLocation", (*deviceFileIterator).absoluteFilePath());
		deviceList.append(deviceInfo);
	}
	devicesDiscovered.insert("devices", deviceList);

	DebugIf(verbose,_log, "devicesDiscovered: [%s]", QString(QJsonDocument(devicesDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

	return devicesDiscovered;
}
