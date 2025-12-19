#include <leddevice/LedDeviceFactory.h>

#include <exception>
#include <map>

#include <HyperionConfig.h>

#include <leddevice/LedDeviceWrapper.h>
#include <leddevice/LedDevice.h>

#include <utils/Logger.h>

// autogen
#include "LedDevice_headers.h"

LedDevice * LedDeviceFactory::construct(const QJsonObject & deviceConfig)
{
	QSharedPointer<Logger> log = Logger::getInstance("LEDDEVICE");
	QJsonDocument config(deviceConfig);

	QString type = deviceConfig["type"].toString("UNSPECIFIED").toLower();

	const LedDeviceRegistry& deviceList = LedDeviceWrapper::getDeviceMap();
	LedDevice* device = nullptr;

	if (!deviceList.contains(type))
	{
		QString dummyDeviceType = "file";
		Error(log, "Configured device '%s' is not supported. Dummy device type (%s) will be used instead", QSTRING_CSTR(type), QSTRING_CSTR(dummyDeviceType));

		QJsonObject dummyDeviceConfig;
		dummyDeviceConfig.insert("type",dummyDeviceType);
		device = LedDeviceFile::construct(dummyDeviceConfig);
	}
	else
	{
		device = deviceList.value(type)(deviceConfig);
	}
	return device;
}
