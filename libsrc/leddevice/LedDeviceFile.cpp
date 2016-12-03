#include "LedDeviceFile.h"

LedDeviceFile::LedDeviceFile(const QJsonObject &deviceConfig)
	: LedDevice()
{
	init(deviceConfig);
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
	if ( _ofs.is_open() )
	{
		_ofs.close();
	}

	_refresh_timer_interval = 0;
	LedDevice::init(deviceConfig);

	std::string fileName = deviceConfig["output"].toString("/dev/null").toStdString();
	_ofs.open( fileName.c_str() );

	return true;
}

int LedDeviceFile::write(const std::vector<ColorRgb> & ledValues)
{
	_ofs << "[";
	for (const ColorRgb& color : ledValues)
	{
		_ofs << color;
	}
	_ofs << "]" << std::endl;

	return 0;
}
