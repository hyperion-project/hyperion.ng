
// STL includes
#include <cerrno>
#include <cstring>

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

int LedDevicePiBlaster::open()
{
	if (_fid != nullptr)
	{
		// The file pointer is already open
		std::cerr << "Attempt to open allready opened device (" << _deviceName << ")" << std::endl;
		return -1;
	}

	_fid = fopen(_deviceName.c_str(), "w");
	if (_fid == nullptr)
	{
		std::cerr << "Failed to open device (" << _deviceName << "). Error message: " << strerror(errno) << std::endl;
		return -1;
	}

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
	unsigned colorIdx = 0;
	for (unsigned iChannel=0; iChannel<8; ++iChannel)
	{
		double pwmDutyCycle = 0.0;
		switch (_channelAssignment[iChannel])
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

		fprintf(_fid, "%i=%f\n", iChannel, pwmDutyCycle);
		fflush(_fid);
	}

	return 0;
}

int LedDevicePiBlaster::switchOff()
{
	for (unsigned iChannel=0; iChannel<8; ++iChannel)
	{
		if (_channelAssignment[iChannel] != ' ')
		{
			fprintf(_fid, "%i=%f\n", iChannel, 0.0);
			fflush(_fid);
		}
	}

	return 0;
}
