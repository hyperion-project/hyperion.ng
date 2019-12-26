#include <leddevice/LedDevice.h>
#include <sstream>

//QT include
#include <QResource>
#include <QStringList>
#include <QDir>
#include <QDateTime>
#include <QEventLoop>
#include <QTimer>

#include "hyperion/Hyperion.h"
#include <utils/JsonUtils.h>

LedDevice::LedDevice(const QJsonObject& config, QObject* parent)
	: QObject(parent)
	, _devConfig(config)
	, _log(Logger::getInstance("LEDDEVICE"))
	, _ledBuffer(0)
	, _deviceReady(false)
	, _deviceInError(false)
	, _refresh_timer(new QTimer(this))
	, _refresh_timer_interval(0)
	, _last_write_time(QDateTime::currentMSecsSinceEpoch())
	, _latchTime_ms(0)
	, _componentRegistered(false)
	, _enabled(false)
	, _refresh_enabled (false)
{
	// setup refreshTimer
	_refresh_timer->setTimerType(Qt::PreciseTimer);
	_refresh_timer->setInterval( _refresh_timer_interval );
	connect(_refresh_timer, SIGNAL(timeout()), this, SLOT(rewriteLeds()));
}

LedDevice::~LedDevice()
{
	_refresh_timer->deleteLater();
}

int LedDevice::open()
{
	setEnable (true);
	return 0;
}

void LedDevice::error(const QString errorMsg)
{
	_deviceInError = true;
	_deviceReady = false;
	_enabled = false;
	this->stopRefreshTimer();

	Error(_log, "Device disabled, device '%s' signals error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(errorMsg));
	emit enableStateChanged(_enabled);
}

void LedDevice::close()
{
	switchOff();
	this->stopRefreshTimer();
}

void LedDevice::setEnable(bool enable)
{
	if (!_deviceReady && enable)
	{
		Error(_log, "Device '%s' cannot be enabled, as it is not ready!", QSTRING_CSTR(_activeDeviceType));
	}
	else
	{
		// emit signal when state changed
		if (_enabled != enable)
		{
			emit enableStateChanged(enable);
		}
		// switch off device when disabled, default: set black to leds when they should go off
		if ( _enabled && !enable)
		{
			switchOff();
		}
		else
		{
			// switch on device when enabled
			if ( !_enabled && enable)
			{
				switchOn();
			}
		}
		_enabled = enable;
	}
}

void LedDevice::setActiveDeviceType(QString deviceType)
{
	_activeDeviceType = deviceType;
}

bool LedDevice::init(const QJsonObject &deviceConfig)
{
	_colorOrder = deviceConfig["colorOrder"].toString("RGB");
	_activeDeviceType = deviceConfig["type"].toString("file").toLower();
	setLedCount(deviceConfig["currentLedCount"].toInt(1)); // property injected to reflect real led count

	_latchTime_ms =deviceConfig["latchTime"].toInt( _latchTime_ms );
	_refresh_timer_interval =  deviceConfig["rewriteTime"].toInt( _refresh_timer_interval);

	if ( _refresh_timer_interval > 0 )
	{
		_refresh_enabled = true;

		if (_refresh_timer_interval <= _latchTime_ms )
		{
			int new_refresh_timer_interval = _latchTime_ms + 10;
			Warning(_log, "latchTime(%d) is bigger/equal rewriteTime(%d), set rewriteTime to %dms", _latchTime_ms, _refresh_timer_interval, new_refresh_timer_interval);
			_refresh_timer_interval = new_refresh_timer_interval;
			_refresh_timer->setInterval( _refresh_timer_interval );
		}

		//Debug(_log, "Refresh interval = %dms",_refresh_timer_interval );
		_refresh_timer->setInterval( _refresh_timer_interval );

		_last_write_time = QDateTime::currentMSecsSinceEpoch();

		this->startRefreshTimer();
	}
	return true;
}

void LedDevice::startRefreshTimer()
{
	if ( _deviceReady)
	{
		_refresh_timer->start();
	}
}

void LedDevice::stopRefreshTimer()
{
	_refresh_timer->stop();
}

int LedDevice::updateLeds(const std::vector<ColorRgb>& ledValues)
{
	int retval = 0;
	if ( !_deviceReady )
	{
		//std::cout << "LedDevice::updateLeds(), LedDevice NOT ready!" <<  std::endl;
		return -1;
	}
	else
	{
		qint64 elapsedTime = QDateTime::currentMSecsSinceEpoch() - _last_write_time;
		if (_latchTime_ms == 0 || elapsedTime >= _latchTime_ms)
		{
			//std::cout << "LedDevice::updateLeds(), Elapsed time since last write (" << elapsedTime << ") ms > _latchTime_ms (" << _latchTime_ms << ") ms" << std::endl;
			retval = write(ledValues);
			_last_write_time = QDateTime::currentMSecsSinceEpoch();

			// if device requires refreshing, save Led-Values and restart the timer
			if ( _refresh_enabled )
			{
				this->startRefreshTimer();
				_last_ledValues = ledValues;
			}
		}
		else
		{
			//std::cout << "LedDevice::updateLeds(), Skip write. _latchTime_ms (" << _latchTime_ms << ") ms > elapsedTime (" << elapsedTime << ") ms" << std::endl;
			if ( _refresh_enabled )
			{
				//Stop timer to allow for next non-refresh update
				this->stopRefreshTimer();
			}
		}
	}
	return retval;
}

int LedDevice::writeBlack()
{
	return _deviceReady ? updateLeds(std::vector<ColorRgb>(static_cast<unsigned long>(_ledCount), ColorRgb::BLACK )) : -1;
}

int LedDevice::switchOff()
{
	// Stop refresh timer to ensure that "write Black" is executed
	this->stopRefreshTimer();

	if ( _latchTime_ms > 0 )
	{
		// Wait latchtime before writing black
		QEventLoop loop;
		QTimer::singleShot( _latchTime_ms, &loop, SLOT( quit() ) );
		loop.exec();
	}
	int rc = writeBlack();
	return rc;
}

int LedDevice::switchOn()
{
	return 0;
}

void LedDevice::setLedCount(int ledCount)
{
	_ledCount     = ledCount;
	_ledRGBCount  = _ledCount * static_cast<int>(sizeof(ColorRgb));
	_ledRGBWCount = _ledCount * static_cast<int>(sizeof(ColorRgbw));
}

int LedDevice::rewriteLeds()
{
	int retval = -1;

	if ( _deviceReady )
	{
		//qint64 elapsedTime = QDateTime::currentMSecsSinceEpoch() - _last_write_time;
		//std::cout << "LedDevice::rewriteLeds(): Rewrite Leds now, elapsedTime [" << elapsedTime << "] ms" << std::endl;
//		//:TESTING: Inject "white" output records to differentiate from normal writes
//		_last_ledValues.clear();
//		_last_ledValues.resize(static_cast<unsigned long>(_ledCount), ColorRgb::WHITE);
//		printLedValues(_last_ledValues);
//		//:TESTING:

		retval = write(_last_ledValues);
		_last_write_time = QDateTime::currentMSecsSinceEpoch();
	}
	else
	{
		// If Device is not ready stop timer
		this->stopRefreshTimer();
	}
	return retval;
}

void LedDevice::printLedValues(const std::vector<ColorRgb>& ledValues )
{
	std::cout << "LedValues [" << ledValues.size() <<"] [";
	for (const ColorRgb& color : ledValues)
	{
		std::cout << color;
	}
	std::cout << "]" << std::endl;
}
