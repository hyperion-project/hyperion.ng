#include "LedDeviceFile.h"

#include <chrono>
#include <iomanip>
#include <iostream>

LedDeviceFile::LedDeviceFile(const QJsonObject &deviceConfig)
	: LedDevice()
{
	_deviceReady = init(deviceConfig);
}

LedDeviceFile::~LedDeviceFile()
{
}

LedDevice* LedDeviceFile::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceFile(deviceConfig);
}

bool LedDeviceFile::init(const QJsonObject &deviceConfig)
{
	LedDevice::init(deviceConfig);
	_refresh_timer_interval = 0;
	_fileName = deviceConfig["output"].toString("/dev/null");
	_printTimeStamp = deviceConfig["printTimeStamp"].toBool(false);

	return true;
}

int LedDeviceFile::open()
{
	if ( _ofs.is_open() )
	{
		_ofs.close();
	}
	_ofs.open( QSTRING_CSTR(_fileName) );
	return 0;
}

int LedDeviceFile::write(const std::vector<ColorRgb> & ledValues)
{
	if ( _printTimeStamp )
	{
		// get a precise timestamp as a string
		const auto now = std::chrono::system_clock::now();
		const auto nowAsTimeT = std::chrono::system_clock::to_time_t(now);
		const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		now.time_since_epoch()) % 1000;

		_ofs
			<< std::put_time(std::localtime(&nowAsTimeT), "%Y-%m-%d %T")
			<< '.' << std::setfill('0') << std::setw(3) << nowMs.count();

	}
	_ofs << " [";
	for (const ColorRgb& color : ledValues)
	{
		_ofs << color;
	}
	_ofs << "]" << std::endl;

	return 0;
}
