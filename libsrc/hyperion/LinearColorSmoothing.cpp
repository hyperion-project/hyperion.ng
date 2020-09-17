// Qt includes
#include <QDateTime>
#include <QTimer>

#include "LinearColorSmoothing.h"
#include <hyperion/Hyperion.h>

#include <cmath>

using namespace hyperion;

const int64_t  DEFAUL_SETTLINGTIME    = 200;	// settlingtime in ms
const double   DEFAUL_UPDATEFREQUENCY = 25;	// updatefrequncy in hz
const int64_t  DEFAUL_UPDATEINTERVALL = static_cast<int64_t>(1000 / DEFAUL_UPDATEFREQUENCY); // updateintervall in ms
const unsigned DEFAUL_OUTPUTDEPLAY    = 0;	// outputdelay in ms

LinearColorSmoothing::LinearColorSmoothing(const QJsonDocument& config, Hyperion* hyperion)
	: QObject(hyperion)
	, _log(Logger::getInstance("SMOOTHING"))
	, _hyperion(hyperion)
	, _updateInterval(DEFAUL_UPDATEINTERVALL)
	, _settlingTime(DEFAUL_SETTLINGTIME)
	, _timer(new QTimer(this))
	, _outputDelay(DEFAUL_OUTPUTDEPLAY)
	, _writeToLedsEnable(false)
	, _continuousOutput(false)
	, _pause(false)
	, _currentConfigId(0)
	, _enabled(false)
{
	// init cfg 0 (default)
	addConfig(DEFAUL_SETTLINGTIME, DEFAUL_UPDATEFREQUENCY, DEFAUL_OUTPUTDEPLAY);
	handleSettingsUpdate(settings::SMOOTHING, config);
	selectConfig(0, true);

	// add pause on cfg 1
	SMOOTHING_CFG cfg = {true, 0, 0, 0};
	_cfgList.append(cfg);

	// listen for comp changes
	connect(_hyperion, &Hyperion::compStateChangeRequest, this, &LinearColorSmoothing::componentStateChange);
	// timer
	connect(_timer, &QTimer::timeout, this, &LinearColorSmoothing::updateLeds);
}

void LinearColorSmoothing::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type == settings::SMOOTHING)
	{
		//	std::cout << "LinearColorSmoothing::handleSettingsUpdate" << std::endl;
		//	std::cout << config.toJson().toStdString() << std::endl;

		QJsonObject obj = config.object();
		if(enabled() != obj["enable"].toBool(true))
			setEnable(obj["enable"].toBool(true));

		_continuousOutput = obj["continuousOutput"].toBool(true);

		SMOOTHING_CFG cfg = {false,
							 static_cast<int64_t>(obj["time_ms"].toInt(DEFAUL_SETTLINGTIME)),
							 static_cast<int64_t>(1000.0/obj["updateFrequency"].toDouble(DEFAUL_UPDATEFREQUENCY)),
							 static_cast<unsigned>(obj["updateDelay"].toInt(DEFAUL_OUTPUTDEPLAY))
							};
		//Debug( _log, "smoothing cfg_id %d: pause: %d bool, settlingTime: %d ms, interval: %d ms (%u Hz), updateDelay: %u frames",  _currentConfigId, cfg.pause, cfg.settlingTime, cfg.updateInterval, unsigned(1000.0/cfg.updateInterval), cfg.outputDelay );
		_cfgList[0] = cfg;

		// if current id is 0, we need to apply the settings (forced)
		if( _currentConfigId == 0)
		{
			//Debug( _log, "_currentConfigId == 0");
			selectConfig(0, true);
		}
		else
		{
			//Debug( _log, "_currentConfigId != 0");
		}
	}
}

int LinearColorSmoothing::write(const std::vector<ColorRgb> &ledValues)
{
	_targetTime = QDateTime::currentMSecsSinceEpoch() + _settlingTime;
	_targetValues = ledValues;

	// received a new target color
	if (_previousValues.empty())
	{
		// not initialized yet
		_previousTime = QDateTime::currentMSecsSinceEpoch();
		_previousValues = ledValues;

		//Debug( _log, "Start Smoothing timer: settlingTime: %d ms, interval: %d ms (%u Hz), updateDelay: %u frames", _settlingTime, _updateInterval, unsigned(1000.0/_updateInterval), _outputDelay );
		QMetaObject::invokeMethod(_timer, "start", Qt::QueuedConnection, Q_ARG(int, _updateInterval));
	}

	return 0;
}

int LinearColorSmoothing::updateLedValues(const std::vector<ColorRgb>& ledValues)
{
	int retval = 0;
	if (!_enabled)
	{
		return -1;
	}
	else
	{
		retval = write(ledValues);
	}
	return retval;
}

void LinearColorSmoothing::updateLeds()
{
	int64_t now = QDateTime::currentMSecsSinceEpoch();
	int64_t deltaTime = _targetTime - now;

	//Debug(_log, "elapsed Time [%d], _targetTime [%d] - now [%d], deltaTime [%d]", now -_previousTime, _targetTime, now, deltaTime);
	if (deltaTime < 0)
	{
		_previousValues = _targetValues;
		_previousTime = now;

		queueColors(_previousValues);
		_writeToLedsEnable = _continuousOutput;
	}
	else
	{
		_writeToLedsEnable = true;

		//std::cout << "LinearColorSmoothing::updateLeds> _previousValues: "; LedDevice::printLedValues ( _previousValues );

		float k = 1.0f - 1.0f * deltaTime / (_targetTime - _previousTime);

		int reddif = 0, greendif = 0, bluedif = 0;

		for (size_t i = 0; i < _previousValues.size(); ++i)
		{
			ColorRgb & prev   = _previousValues[i];
			ColorRgb & target = _targetValues[i];

			reddif   = target.red   - prev.red;
			greendif = target.green - prev.green;
			bluedif  = target.blue  - prev.blue;

			prev.red   += (reddif   < 0 ? -1:1) * std::ceil(k * std::abs(reddif));
			prev.green += (greendif < 0 ? -1:1) * std::ceil(k * std::abs(greendif));
			prev.blue  += (bluedif  < 0 ? -1:1) * std::ceil(k * std::abs(bluedif));
		}
		_previousTime = now;

		//std::cout << "LinearColorSmoothing::updateLeds> _targetValues: "; LedDevice::printLedValues ( _targetValues );

		queueColors(_previousValues);
	}
}

void LinearColorSmoothing::queueColors(const std::vector<ColorRgb> & ledColors)
{
	//Debug(_log, "queueColors -  _outputDelay[%d] _outputQueue.size() [%d], _writeToLedsEnable[%d]", _outputDelay, _outputQueue.size(), _writeToLedsEnable);
	if (_outputDelay == 0)
	{
		// No output delay => immediate write
		if ( _writeToLedsEnable && !_pause)
		{
//			if ( ledColors.size() == 0 )
//				qFatal ("No LedValues! - in LinearColorSmoothing::queueColors() - _outputDelay == 0");
//			else
			emit _hyperion->ledDeviceData(ledColors);
		}
	}
	else
	{
		// Push new colors in the delay-buffer
		if ( _writeToLedsEnable )
			_outputQueue.push_back(ledColors);

		// If the delay-buffer is filled pop the front and write to device
		if (_outputQueue.size() > 0 )
		{
			if ( _outputQueue.size() > _outputDelay || !_writeToLedsEnable )
			{
				if (!_pause)
				{
					emit _hyperion->ledDeviceData(_outputQueue.front());
				}
				_outputQueue.pop_front();
			}
		}
	}
}

void LinearColorSmoothing::clearQueuedColors()
{
	QMetaObject::invokeMethod(_timer, "stop", Qt::QueuedConnection);
	_previousValues.clear();

	_targetValues.clear();
}

void LinearColorSmoothing::componentStateChange(hyperion::Components component, bool state)
{
	_writeToLedsEnable = state;
	if(component == hyperion::COMP_LEDDEVICE)
	{
		clearQueuedColors();
	}

	if(component == hyperion::COMP_SMOOTHING)
	{
		setEnable(state);
	}
}

void LinearColorSmoothing::setEnable(bool enable)
{
	_enabled = enable;
	if (!_enabled)
	{
		clearQueuedColors();
	}
	// update comp register
	_hyperion->setNewComponentState(hyperion::COMP_SMOOTHING, enable);
}

void LinearColorSmoothing::setPause(bool pause)
{
	_pause = pause;
}

unsigned LinearColorSmoothing::addConfig(int settlingTime_ms, double ledUpdateFrequency_hz, unsigned updateDelay)
{
	SMOOTHING_CFG cfg = {false, settlingTime_ms, int64_t(1000.0/ledUpdateFrequency_hz), updateDelay};
	_cfgList.append(cfg);

	//Debug( _log, "smoothing cfg %d: pause: %d bool, settlingTime: %d ms, interval: %d ms (%u Hz), updateDelay: %u frames",  _cfgList.count()-1, cfg.pause, cfg.settlingTime, cfg.updateInterval, unsigned(1000.0/cfg.updateInterval), cfg.outputDelay );
	return _cfgList.count() - 1;
}

unsigned LinearColorSmoothing::updateConfig(unsigned cfgID, int settlingTime_ms, double ledUpdateFrequency_hz, unsigned updateDelay)
{
	unsigned updatedCfgID = cfgID;
	if ( cfgID < static_cast<unsigned>(_cfgList.count()) )
	{
		SMOOTHING_CFG cfg = {false, settlingTime_ms, int64_t(1000.0/ledUpdateFrequency_hz), updateDelay};
		_cfgList[updatedCfgID] = cfg;
	}
	else
	{
		updatedCfgID = addConfig ( settlingTime_ms, ledUpdateFrequency_hz, updateDelay);
	}
//	Debug( _log, "smoothing updatedCfgID %u: settlingTime: %d ms, "
//				 "interval: %d ms (%u Hz), updateDelay: %u frames",  cfgID, _settlingTime, int64_t(1000.0/ledUpdateFrequency_hz), unsigned(ledUpdateFrequency_hz), updateDelay );
	return updatedCfgID;
}

bool LinearColorSmoothing::selectConfig(unsigned cfg, bool force)
{
	if (_currentConfigId == cfg && !force)
	{
		//Debug( _log, "selectConfig SAME as before, not FORCED - _currentConfigId [%u], force [%d]", cfg, force);
		//Debug( _log, "current smoothing cfg: %d, settlingTime: %d ms, interval: %d ms (%u Hz), updateDelay: %u frames",  _currentConfigId, _settlingTime, _updateInterval, unsigned(1000.0/_updateInterval), _outputDelay );
		return true;
	}

	//Debug( _log, "selectConfig FORCED - _currentConfigId [%u], force [%d]", cfg, force);
	if ( cfg < (unsigned)_cfgList.count())
	{
		_settlingTime     = _cfgList[cfg].settlingTime;
		_outputDelay      = _cfgList[cfg].outputDelay;
		_pause            = _cfgList[cfg].pause;

		if (_cfgList[cfg].updateInterval != _updateInterval)
		{

			QMetaObject::invokeMethod(_timer, "stop", Qt::QueuedConnection);
			_updateInterval = _cfgList[cfg].updateInterval;
			if ( this->enabled() && this->_writeToLedsEnable )
			{
				//Debug( _log, "_cfgList[cfg].updateInterval != _updateInterval - Restart timer - _updateInterval [%d]", _updateInterval);
				QMetaObject::invokeMethod(_timer, "start", Qt::QueuedConnection, Q_ARG(int, _updateInterval));
			}
			else
			{
				//Debug( _log, "Smoothing disabled, do NOT restart timer");
			}
		}
		_currentConfigId = cfg;
		// Debug( _log, "current smoothing cfg: %d, settlingTime: %d ms, interval: %d ms (%u Hz), updateDelay: %u frames",  _currentConfigId, _settlingTime, _updateInterval, unsigned(1000.0/_updateInterval), _outputDelay );
		//	DebugIf( enabled() && !_pause, _log, "set smoothing cfg: %u settlingTime: %d ms, interval: %d ms,  updateDelay: %u frames",  _currentConfigId, _settlingTime, _updateInterval,  _outputDelay );
		// DebugIf( _pause, _log, "set smoothing cfg: %d, pause",  _currentConfigId );

		return true;
	}

	// reset to default
	_currentConfigId = 0;
	return false;
}
