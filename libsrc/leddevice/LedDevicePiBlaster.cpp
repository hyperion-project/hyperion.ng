
// STL includes
#include <cerrno>
#include <cstring>
#include <csignal>


// jsoncpp includes
#include <json/json.h>

// QT includes
#include <QFile>

// Local LedDevice includes
#include "LedDevicePiBlaster.h"

LedDevicePiBlaster::LedDevicePiBlaster(const std::string & deviceName, const Json::Value & gpioMapping) :
	_deviceName(deviceName),
	_fid(nullptr)
{

	signal(SIGPIPE,  SIG_IGN);

// initialise the mapping tables
// -1 is invalid
// z is also meaningless
// { "gpio" : 4, "ledindex" : 0, "ledcolor" : "r" },
	#define TABLE_SZ sizeof(_gpio_to_led)/sizeof(_gpio_to_led[0])

	for (int i=0; i <  TABLE_SZ; i++ )
	{
		_gpio_to_led[i] = -1;
		_gpio_to_color[i] = 'z';
	}

// walk through the json config and populate the mapping tables
	for (const Json::Value& gpioMap : gpioMapping)
	{
		const int gpio = gpioMap.get("gpio",-1).asInt();
		const int ledindex = gpioMap.get("ledindex",-1).asInt();
		const std::string ledcolor = gpioMap.get("ledcolor","z").asString();
//		printf ("got gpio %d ledindex %d color %c\n", gpio,ledindex, ledcolor[0]);
		// ignore missing/invalid settings
		if ( (gpio >= 0) && (gpio < TABLE_SZ) && (ledindex >= 0) ){
			_gpio_to_led[gpio] = ledindex;
			_gpio_to_color[gpio] = ledcolor[0]; // 1st char of string
		} else {
			printf ("IGNORING gpio %d ledindex %d color %c\n", gpio,ledindex, ledcolor[0]);
		}
	}
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

int LedDevicePiBlaster::open(bool report)
{
	if (_fid != nullptr)
	{
		// The file pointer is already open
		if (report)
		{
			std::cerr << "Attempt to open allready opened device (" << _deviceName << ")" << std::endl;
		}
		return -1;
	}

	if (!QFile::exists(_deviceName.c_str()))
	{
		if (report)
		{
			std::cerr << "The device(" << _deviceName << ") does not yet exist, can not connect (yet)." << std::endl;
		}
		return -1;
	}

	_fid = fopen(_deviceName.c_str(), "w");
	if (_fid == nullptr)
	{
		if (report)
		{
			std::cerr << "Failed to open device (" << _deviceName << "). Error message: " << strerror(errno) << std::endl;
		}
		return -1;
	}

	std::cout << "Connect to device(" << _deviceName << ")" << std::endl;

	return 0;
}

int LedDevicePiBlaster::write(const std::vector<ColorRgb> & ledValues)
{
	// Attempt to open if not yet opened
	if (_fid == nullptr && open(false) < 0)
	{
		return -1;
	}

	int valueIdx = -1;
	for (unsigned int i=0; i < TABLE_SZ; i++ )
	{
		valueIdx = _gpio_to_led[ i ];
		if ( (valueIdx >= 0) && (valueIdx < (signed) ledValues.size()) ) 
		{
			double pwmDutyCycle = 0.0;
//			printf ("iPin %d valueIdx %d color %c\n", iPin, valueIdx, _gpio_to_color[ iPins[iPin] ] ) ;
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

int LedDevicePiBlaster::switchOff()
{
	// Attempt to open if not yet opened
	if (_fid == nullptr && open(false) < 0)
	{
		return -1;
	}

	int valueIdx = -1;
	for (unsigned int i=0; i < TABLE_SZ; i++ )
	{
		valueIdx = _gpio_to_led[ i ];
		if (valueIdx >= 0)
		{
			fprintf(_fid, "%i=%f\n", i, 0.0);
			fflush(_fid);
		}
	}

	return 0;
}
