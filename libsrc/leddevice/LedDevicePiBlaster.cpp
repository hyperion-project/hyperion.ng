
// STL includes
#include <cerrno>
#include <cstring>

// QT includes
#include <QFile>

// Local LedDevice includes
#include "LedDevicePiBlaster.h"

LedDevicePiBlaster::LedDevicePiBlaster(const std::string & deviceName, const std::string & channelAssignment) :
	_deviceName(deviceName),
	_channelAssignment(channelAssignment),
	_fid(nullptr)
{
	// empty
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

//Channel number    GPIO number   Pin in P1 header
//      0               4             P1-7
//      1              17             P1-11
//      2              18             P1-12
//      3              21             P1-13
//      4              22             P1-15
//      5              23             P1-16
//      6              24             P1-18
//      7              25             P1-22
int LedDevicePiBlaster::write(const std::vector<ColorRgb> & ledValues)
{
	// Attempt to open if not yet opened
	if (_fid == nullptr && open(false) < 0)
	{
		return -1;
	}

	std::vector<int> iPins = {4, 17, 18, 27, 21, 22, 23, 24, 25};

	unsigned colorIdx = 0;
	for (unsigned iPin=0; iPin<iPins.size(); ++iPin)
	{
		double pwmDutyCycle = 0.0;
		switch (_channelAssignment[iPin])
		{
		case 'r':
			pwmDutyCycle = ledValues[colorIdx].red / 255.0;
			++colorIdx;
			break;
		case 'g':
			pwmDutyCycle = ledValues[colorIdx].green / 255.0;
			++colorIdx;
			break;
		case 'b':
			pwmDutyCycle = ledValues[colorIdx].blue / 255.0;
			++colorIdx;
			break;
		default:
			continue;
		}

		fprintf(_fid, "%i=%f\n", iPins[iPin], pwmDutyCycle);
		fflush(_fid);
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

	std::vector<int> iPins = {4, 17, 18, 21, 22, 23, 24, 25};

	for (unsigned iPin=0; iPin<iPins.size(); ++iPin)
	{
		if (_channelAssignment[iPin] != ' ')
		{
			fprintf(_fid, "%i=%f\n", iPins[iPin], 0.0);
			fflush(_fid);
		}
	}

	return 0;
}
