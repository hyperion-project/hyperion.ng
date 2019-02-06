#include <leddevice/LedDevice.h>
#include <sstream>

//QT include
#include <QResource>
#include <QStringList>
#include <QDir>
#include <QDateTime>

#include "hyperion/Hyperion.h"
#include <utils/JsonUtils.h>

LedDevice::LedDevice(const QJsonObject& config, QObject* parent)
	: QObject(parent)
	, _devConfig(config)
	, _log(Logger::getInstance("LEDDEVICE"))
	, _ledBuffer(0)
	, _deviceReady(true)
	, _refresh_timer()
	, _refresh_timer_interval(0)
	, _last_write_time(QDateTime::currentMSecsSinceEpoch())
	, _latchTime_ms(0)
	, _componentRegistered(false)
	, _enabled(true)
{
	// setup timer
	_refresh_timer.setInterval(0);
	connect(&_refresh_timer, SIGNAL(timeout()), this, SLOT(rewriteLeds()));
}

LedDevice::~LedDevice()
{

}

// dummy implemention
int LedDevice::open()
{
	return 0;
}

void LedDevice::setEnable(bool enable)
{
	// emit signal when state changed
	if (_enabled != enable)
	{
		emit enableStateChanged(enable);
	}

	// set black to leds when they should go off
	if ( _enabled && !enable)
	{
		switchOff();
	}
	_enabled = enable;
}

void LedDevice::setActiveDevice(QString dev)
{
	_activeDevice = dev;
}

bool LedDevice::init(const QJsonObject &deviceConfig)
{
	_colorOrder = deviceConfig["colorOrder"].toString("RGB");
	_activeDevice = deviceConfig["type"].toString("file").toLower();
	setLedCount(deviceConfig["currentLedCount"].toInt(1)); // property injected to reflect real led count

	_latchTime_ms = deviceConfig["latchTime"].toInt(_latchTime_ms);
	_refresh_timer.setInterval( deviceConfig["rewriteTime"].toInt( _refresh_timer_interval) );
	if (_refresh_timer.interval() <= (signed)_latchTime_ms )
	{
		Warning(_log, "latchTime(%d) is bigger/equal rewriteTime(%d)", _refresh_timer.interval(), _latchTime_ms);
		_refresh_timer.setInterval(_latchTime_ms+10);
	}

	return true;
}

int LedDevice::setLedValues(const std::vector<ColorRgb>& ledValues)
{
	int retval = 0;
	if (!_deviceReady || !_enabled)
		return -1;


	// restart the timer
	if (_refresh_timer.interval() > 0)
	{
		_refresh_timer.start();
	}

	if (_latchTime_ms == 0 || QDateTime::currentMSecsSinceEpoch()-_last_write_time >= _latchTime_ms)
	{
		_ledValues = ledValues;
		retval = write(ledValues);
		_last_write_time = QDateTime::currentMSecsSinceEpoch();
	}
	//else Debug(_log, "latch %d", QDateTime::currentMSecsSinceEpoch()-_last_write_time);

	return retval;
}

int LedDevice::switchOff()
{
	return _deviceReady ? write(std::vector<ColorRgb>(_ledCount, ColorRgb::BLACK )) : -1;
}

int LedDevice::switchOn()
{
	return 0;
}

void LedDevice::setLedCount(int ledCount)
{
	_ledCount     = ledCount;
	_ledRGBCount  = _ledCount * sizeof(ColorRgb);
	_ledRGBWCount = _ledCount * sizeof(ColorRgbw);
}

int LedDevice::rewriteLeds()
{
	return _enabled ? write(_ledValues) : -1;
}
