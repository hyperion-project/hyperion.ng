
// STL includes
#include <cstring>
#include <csignal>

// QT includes
#include <QFile>

// Local LedDevice includes
#include "LedDevicePiBlaster.h"

LedDevicePiBlaster::LedDevicePiBlaster(const QJsonObject &deviceConfig)
	: _fid(nullptr)
{
	signal(SIGPIPE,  SIG_IGN);

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

	_deviceReady = init(deviceConfig);
}

LedDevicePiBlaster::~LedDevicePiBlaster()
{
	// Close the device (if it is opened)
	if (_fid != nullptr)
	{
		fclose(_fid);
		_fid = nullptr;
	}
}


bool LedDevicePiBlaster::init(const QJsonObject &deviceConfig)
{
	LedDevice::init(deviceConfig);

	_deviceName    = deviceConfig["output"].toString("/dev/pi-blaster");
	QJsonArray gpioMapping = deviceConfig["gpiomap"].toArray();

	if (gpioMapping.isEmpty())
	{
		throw std::runtime_error("Piblaster: no gpiomap defined.");
	}

	// walk through the json config and populate the mapping tables
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

	return true;
}

LedDevice* LedDevicePiBlaster::construct(const QJsonObject &deviceConfig)
{
	return new LedDevicePiBlaster(deviceConfig);
}

int LedDevicePiBlaster::open()
{
	if (_fid != nullptr)
	{
		// The file pointer is already open
		Error( _log, "Device (%s) is already open.", QSTRING_CSTR(_deviceName) );
		return -1;
	}

	if (!QFile::exists(_deviceName))
	{
		Error( _log, "The device (%s) does not yet exist.",QSTRING_CSTR(_deviceName) );
		return -1;
	}

	_fid = fopen(QSTRING_CSTR(_deviceName), "w");
	if (_fid == nullptr)
	{
		Error( _log, "Failed to open device (%s). Error message: %s", QSTRING_CSTR(_deviceName),  strerror(errno) );
		return -1;
	}

	Info( _log, "Connected to device(%s)", QSTRING_CSTR(_deviceName));

	return 0;
}

int LedDevicePiBlaster::write(const std::vector<ColorRgb> & ledValues)
{
	// Attempt to open if not yet opened
	if (_fid == nullptr && open() < 0)
	{
		return -1;
	}

	int valueIdx = -1;
	for (unsigned int i=0; i < TABLE_SZ; i++ )
	{
		valueIdx = _gpio_to_led[ i ];
		if ( (valueIdx >= 0) && (valueIdx < _ledCount) ) 
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

//			fprintf(_fid, "%i=%f\n", iPins[iPin], pwmDutyCycle);

			if ( (fprintf(_fid, "%i=%f\n", i, pwmDutyCycle) < 0) 
			  || (fflush(_fid) < 0)) {
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
