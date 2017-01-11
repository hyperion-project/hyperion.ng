#include <leddevice/LedDevice.h>
#include <sstream>

//QT include
#include <QResource>
#include <QStringList>
#include <QDir>

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
	, _refresh_timer()
	, _refresh_timer_interval(0)
{
	LedDevice::getLedDeviceSchemas();

	// setup timer
	_refresh_timer.setSingleShot(false);
	_refresh_timer.setInterval(0);
	connect(&_refresh_timer, SIGNAL(timeout()), this, SLOT(rewriteLeds()));
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

bool LedDevice::init(const QJsonObject &deviceConfig)
{
	_refresh_timer.setInterval( deviceConfig["rewriteTime"].toInt(_refresh_timer_interval) );
	return true;
}

QJsonObject LedDevice::getLedDeviceSchemas()
{
	// make sure the resources are loaded (they may be left out after static linking)
	Q_INIT_RESOURCE(LedDeviceSchemas);
	QJsonParseError error;

	// read the json schema from the resource
	QDir d(":/leddevices/");
	QStringList l = d.entryList();
	QJsonObject result, schemaJson;
	
	for(QString &item : l)
	{
		QFile schemaData(QString(":/leddevices/")+item);
		QString devName = item.remove("schema-");
		
		if (!schemaData.open(QIODevice::ReadOnly))
		{
			Error(Logger::getInstance("LedDevice"), "Schema not found: %s", item.toUtf8().constData());
			throw std::runtime_error("ERROR: Schema not found: " + item.toStdString());
		}
		
		QByteArray schema = schemaData.readAll();
		QJsonDocument doc = QJsonDocument::fromJson(schema, &error);
		schemaData.close();
		
		if (error.error != QJsonParseError::NoError)
		{
			// report to the user the failure and their locations in the document.
			int errorLine(0), errorColumn(0);
			
			for( int i=0, count=qMin( error.offset,schema.size()); i<count; ++i )
			{
				++errorColumn;
				if(schema.at(i) == '\n' )
				{
					errorColumn = 0;
					++errorLine;
				}
			}
			
			std::stringstream sstream;
			sstream << error.errorString().toStdString() << " at Line: " << errorLine << ", Column: " << errorColumn;
			Error(Logger::getInstance("LedDevice"), "LedDevice JSON schema error in %s (%s)", item.toUtf8().constData(), sstream.str().c_str());
			throw std::runtime_error("ERROR: Json schema wrong: " + sstream.str());
		}
		
		schemaJson = doc.object();
		schemaJson["title"] = QString("edt_dev_spec_header_title");
		
		result[devName] = schemaJson;
	}
	
	return result;
}


int LedDevice::setLedValues(const std::vector<ColorRgb>& ledValues)
{
	if (!_deviceReady)
		return -1;
	
	_ledValues = ledValues;

	// restart the timer
	if (_refresh_timer.interval() > 0)
	{
		_refresh_timer.start();
	}
	return write(ledValues);
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

int LedDevice::rewriteLeds()
{
	return write(_ledValues);
}
