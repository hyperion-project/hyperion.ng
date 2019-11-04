// Qt includes
#include <QDateTime>
#include <QTimer>

#include "LinearColorSmoothing.h"
#include <hyperion/Hyperion.h>

#include <cmath>

using namespace hyperion;

LinearColorSmoothing::LinearColorSmoothing(const QJsonDocument& config, Hyperion* hyperion)
	: LedDevice(QJsonObject(), hyperion)
	, _log(Logger::getInstance("SMOOTHING"))
	, _hyperion(hyperion)
	, _updateInterval(1000)
	, _settlingTime(200)
	, _timer(new QTimer(this))
	, _outputDelay(0)
	, _writeToLedsEnable(true)
	, _continuousOutput(false)
	, _pause(false)
	, _currentConfigId(0)
{
	// set initial state to true, as LedDevice::enabled() is true by default
	_hyperion->getComponentRegister().componentStateChanged(hyperion::COMP_SMOOTHING, true);

	// init cfg 0 (default)
	_cfgList.append({false, 200, 25, 0});
	handleSettingsUpdate(settings::SMOOTHING, config);

	// add pause on cfg 1
	SMOOTHING_CFG cfg = {true};
	_cfgList.append(cfg);

	// listen for comp changes
	connect(_hyperion, &Hyperion::componentStateChanged, this, &LinearColorSmoothing::componentStateChange);
	// timer
	connect(_timer, SIGNAL(timeout()), this, SLOT(updateLeds()));
}

LinearColorSmoothing::~LinearColorSmoothing()
{

}

void LinearColorSmoothing::handleSettingsUpdate(const settings::type& type, const QJsonDocument& config)
{
	if(type == settings::SMOOTHING)
	{
		QJsonObject obj = config.object();
		_continuousOutput = obj["continuousOutput"].toBool(true);
		SMOOTHING_CFG cfg = {false, obj["time_ms"].toInt(200), unsigned(1000.0/obj["updateFrequency"].toDouble(25.0)), unsigned(obj["updateDelay"].toInt(0))};
		_cfgList[0] = cfg;
		// if current id is 0, we need to apply the settings (forced)
		if(!_currentConfigId)
			selectConfig(0, true);

		if(enabled() != obj["enable"].toBool(true))
			setEnable(obj["enable"].toBool(true));
	}
}

int LinearColorSmoothing::write(const std::vector<ColorRgb> &ledValues)
{
	// received a new target color
	if (_previousValues.empty())
	{
		// not initialized yet
		_targetTime = QDateTime::currentMSecsSinceEpoch() + _settlingTime;
		_targetValues = ledValues;

		_previousTime = QDateTime::currentMSecsSinceEpoch();
		_previousValues = ledValues;
		QMetaObject::invokeMethod(_timer, "start", Qt::QueuedConnection, Q_ARG(int, _updateInterval));
	}
	else
	{
		_targetTime = QDateTime::currentMSecsSinceEpoch() + _settlingTime;
		memcpy(_targetValues.data(), ledValues.data(), ledValues.size() * sizeof(ColorRgb));
	}

	return 0;
}

int LinearColorSmoothing::switchOff()
{
	// We will keep updating the leds (but with pure-black)

	// Clear the smoothing parameters
	std::fill(_targetValues.begin(), _targetValues.end(), ColorRgb::BLACK);
	_targetTime = 0;

	// Erase the output-queue
	for (unsigned i=0; i<_outputQueue.size(); ++i)
	{
		_outputQueue.push_back(_targetValues);
		_outputQueue.pop_front();
	}

	emit _hyperion->ledDeviceData(std::vector<ColorRgb>(_ledCount, ColorRgb::BLACK));

	return 0;
}

void LinearColorSmoothing::updateLeds()
{
	int64_t now = QDateTime::currentMSecsSinceEpoch();
	int deltaTime = _targetTime - now;

	if (deltaTime < 0)
	{
		memcpy(_previousValues.data(), _targetValues.data(), _targetValues.size() * sizeof(ColorRgb));
		_previousTime = now;

		queueColors(_previousValues);
		_writeToLedsEnable = _continuousOutput;
	}
	else
	{
		_writeToLedsEnable = true;
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

		queueColors(_previousValues);
	}
}

void LinearColorSmoothing::queueColors(const std::vector<ColorRgb> & ledColors)
{
	if (_outputDelay == 0)
	{
		// No output delay => immediate write
		if ( _writeToLedsEnable && !_pause)
			emit _hyperion->ledDeviceData(ledColors);

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

void LinearColorSmoothing::componentStateChange(const hyperion::Components component, const bool state)
{
	if(component == hyperion::COMP_LEDDEVICE)
	{
		setEnable(state);
	}

	if(component == hyperion::COMP_SMOOTHING)
	{
		setEnable(state);
		// update comp register
		_hyperion->getComponentRegister().componentStateChanged(hyperion::COMP_SMOOTHING, state);
	}

}

void LinearColorSmoothing::setEnable(bool enable)
{
	if (!enable)
	{
		QMetaObject::invokeMethod(_timer, "stop", Qt::QueuedConnection);
		_previousValues.clear();
	}
}

void LinearColorSmoothing::setPause(bool pause)
{
	_pause = pause;
}

unsigned LinearColorSmoothing::addConfig(int settlingTime_ms, double ledUpdateFrequency_hz, unsigned updateDelay)
{
	SMOOTHING_CFG cfg = {false, settlingTime_ms, int64_t(1000.0/ledUpdateFrequency_hz), updateDelay};
	_cfgList.append(cfg);

	//Debug( _log, "smoothing cfg %d: interval: %d ms, settlingTime: %d ms, updateDelay: %d frames",  _cfgList.count()-1, cfg.updateInterval, cfg.settlingTime,  cfg.outputDelay );
	return _cfgList.count() - 1;
}

bool LinearColorSmoothing::selectConfig(unsigned cfg, const bool& force)
{
	if (_currentConfigId == cfg && !force)
	{
		return true;
	}

	if ( cfg < (unsigned)_cfgList.count())
	{
		_settlingTime     = _cfgList[cfg].settlingTime;
		_outputDelay      = _cfgList[cfg].outputDelay;
		_pause            = _cfgList[cfg].pause;

		if (_cfgList[cfg].updateInterval != _updateInterval)
		{
			QMetaObject::invokeMethod(_timer, "stop", Qt::QueuedConnection);
			_updateInterval = _cfgList[cfg].updateInterval;
			QMetaObject::invokeMethod(_timer, "start", Qt::QueuedConnection, Q_ARG(int, _updateInterval));
		}
		_currentConfigId = cfg;
		//DebugIf( enabled() && !_pause, _log, "set smoothing cfg: %d, interval: %d ms, settlingTime: %d ms, updateDelay: %d frames",  _currentConfigId, _updateInterval, _settlingTime,  _outputDelay );
		//DebugIf( _pause, _log, "set smoothing cfg: %d, pause",  _currentConfigId );

		return true;
	}

	// reset to default
	_currentConfigId = 0;
	return false;
}
