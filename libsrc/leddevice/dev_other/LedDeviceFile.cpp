#include "LedDeviceFile.h"

#include <chrono>
#include <iomanip>
#include <iostream>

LedDeviceFile::LedDeviceFile(const QJsonObject &deviceConfig)
	: LedDevice()
{
	_devConfig = deviceConfig;
	_deviceReady = false;
	_printTimeStamp = false;
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
	bool initOK = LedDevice::init(deviceConfig);

	_fileName = deviceConfig["output"].toString("/dev/null");
	_printTimeStamp = deviceConfig["printTimeStamp"].toBool(false);

	return initOK;
}

int LedDeviceFile::open()
{
	int retval = -1;
	QString errortext;
	_deviceReady = false;

	if ( init(_devConfig) )
	{
		if ( _ofs.is_open() )
		{
			_ofs.close();
		}

		_ofs.open( QSTRING_CSTR(_fileName));
		if ( _ofs.fail() )
		{
			errortext = QString ("Failed to open file (%1). Error message: %2").arg(_fileName, strerror(errno));
		}
		else
		{
			_deviceReady = true;
			setEnable(true);
			retval = 0;
		}

		if ( retval < 0 )
		{
			this->setInError( errortext );
		}
	}
	return retval;
}

void LedDeviceFile::close()
{
	LedDevice::close();

	// LedDevice specific closing activites
	if ( _ofs )
	{
		_ofs.close();
		if ( _ofs.fail() )
		{
			Error( _log, "Failed to close device (%s). Error message: %s", QSTRING_CSTR(_fileName), strerror(errno) );
		}
	}
}

int LedDeviceFile::write(const std::vector<ColorRgb> & ledValues)
{
	//printLedValues (ledValues);
	if ( _printTimeStamp )
	{
		// get a precise timestamp as a string
		const auto now = std::chrono::system_clock::now();
		const auto nowAsTimeT = std::chrono::system_clock::to_time_t(now);
		const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

		const auto elapsedTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastWriteTime);

		_ofs
			<< std::put_time(std::localtime(&nowAsTimeT), "%Y-%m-%d %T")
			<< '.' << std::setfill('0') << std::setw(3) << nowMs.count()
			<< " | +" << std::setfill('0') << std::setw(4) << elapsedTimeMs.count();

		lastWriteTime = now;
	}

	_ofs << " [";
	for (const ColorRgb& color : ledValues)
	{
		_ofs << color;
	}
	_ofs << "]" << std::endl;

	return 0;
}
