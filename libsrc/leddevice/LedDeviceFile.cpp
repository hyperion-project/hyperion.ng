
// Local-Hyperion includes
#include "LedDeviceFile.h"

LedDeviceFile::LedDeviceFile(const Json::Value &deviceConfig)
	: LedDevice()
{
	setConfig(deviceConfig);
}

LedDeviceFile::~LedDeviceFile()
{
}

LedDevice* LedDeviceFile::construct(const Json::Value &deviceConfig)
{
	return new LedDeviceFile(deviceConfig);
}

bool LedDeviceFile::setConfig(const Json::Value &deviceConfig)
{
	if ( _ofs.is_open() )
	{
		_ofs.close();
	}
	
	std::string fileName = deviceConfig.get("output","/dev/null").asString();
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

int LedDeviceFile::switchOff()
{
	return 0;
}
