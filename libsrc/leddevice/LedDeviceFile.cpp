
// Local-Hyperion includes
#include "LedDeviceFile.h"

LedDeviceFile::LedDeviceFile(const std::string& output) :
	_ofs(output.empty()?"/dev/null":output.c_str())
{
	// empty
}

LedDeviceFile::~LedDeviceFile()
{
	// empty
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
