// Stl includes
#include <exception>
#include <map>

// Build configuration
#include <HyperionConfig.h>

// Leddevice includes
#include <leddevice/LedDeviceFactory.h>
#include <leddevice/LedDeviceWrapper.h>
#include <utils/Logger.h>
#include <leddevice/LedDevice.h>

// autogen
#include "LedDevice_headers.h"

LedDevice * LedDeviceFactory::construct(const QJsonObject & deviceConfig)
{
	Logger * log = Logger::getInstance("LEDDEVICE");
	QJsonDocument config(deviceConfig);

	QString type = deviceConfig["type"].toString("UNSPECIFIED").toLower();

	const LedDeviceRegistry& devList = LedDeviceWrapper::getDeviceMap();
	LedDevice* device = nullptr;
	try
	{
		for ( auto dev: devList)
		{
			if (dev.first == type)
			{
				device = dev.second(deviceConfig);
				break;
			}
		}

		if (device == nullptr)
		{
			Error(log, "Dummy device used, because configured device '%s' is unknown", QSTRING_CSTR(type) );
			throw std::runtime_error("unknown device");
		}
	}
	catch(std::exception& e)
	{

		Error(log, "Dummy device used, because configured device '%s' throws error '%s'", QSTRING_CSTR(type), e.what());
		const QJsonObject dummyDeviceConfig;
		device = LedDeviceFile::construct(QJsonObject());
	}

	return device;
}
