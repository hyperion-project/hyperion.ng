#include <leddevice/LedDevice.h>

#include <QResource>
#include <QStringList>
#include <QDir>
#include <json/json.h>

LedDeviceRegistry LedDevice::_ledDeviceMap = LedDeviceRegistry();
std::string LedDevice::_activeDevice = "";
int LedDevice::_ledCount    = 0;
int LedDevice::_ledRGBCount = 0;
int LedDevice::_ledRGBWCount= 0;

LedDevice::LedDevice()
	: QObject()
	, _log(Logger::getInstance("LedDevice"))
	, _ledBuffer(0)
	, _deviceReady(true)

{
	LedDevice::getLedDeviceSchemas();
}

// dummy implemention
int LedDevice::open()
{
	return 0;
}

int LedDevice::addToDeviceMap(std::string name, LedDeviceCreateFuncType funcPtr)
{
	_ledDeviceMap.emplace(name,funcPtr);
	return 0;
}

const LedDeviceRegistry& LedDevice::getDeviceMap()
{
	return _ledDeviceMap;
}

void LedDevice::setActiveDevice(std::string dev)
{
	_activeDevice = dev;
}

Json::Value LedDevice::getLedDeviceSchemas()
{
	// make sure the resources are loaded (they may be left out after static linking)
	Q_INIT_RESOURCE(LedDeviceSchemas);

	// read the json schema from the resource
	QDir d(":/leddevices/");
	QStringList l = d.entryList();
	Json::Value result;
	for(QString &item : l)
	{
		QResource schemaData(QString(":/leddevices/")+item);
		std::string devName = item.remove("schema-").toStdString();
		Json::Value & schemaJson = result[devName];
		
		Json::Reader jsonReader;
		if (!jsonReader.parse(reinterpret_cast<const char *>(schemaData.data()), reinterpret_cast<const char *>(schemaData.data()) + schemaData.size(), schemaJson, false))
		{
			Error(Logger::getInstance("LedDevice"), "LedDevice JSON schema error in %s (%s)", item.toUtf8().constData(), jsonReader.getFormattedErrorMessages().c_str() );
			throw std::runtime_error("ERROR: Json schema wrong: " + jsonReader.getFormattedErrorMessages())	;
		}
		schemaJson["title"] = "LED Device Specific";

	}
	
	return result;
}


int LedDevice::setLedValues(const std::vector<ColorRgb>& ledValues)
{
	return _deviceReady ? write(ledValues) : -1;
}

int LedDevice::switchOff()
{
	return _deviceReady ? write(std::vector<ColorRgb>(_ledCount, ColorRgb::BLACK )) : -1;
}


void LedDevice::setLedCount(int ledCount)
{
	_ledCount     = ledCount;
	_ledRGBCount  = _ledCount * sizeof(ColorRgb);
	_ledRGBWCount = _ledCount * sizeof(ColorRgbw);
}
