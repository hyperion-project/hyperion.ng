// Qt includes
#include <QDateTime>

#include "LinearColorSmoothing.h"
#include <hyperion/Hyperion.h>

#include <cmath>

using namespace hyperion;

// ledUpdateFrequency_hz = 0 > cause divide by zero!
LinearColorSmoothing::LinearColorSmoothing( LedDevice * ledDevice, double ledUpdateFrequency_hz, int settlingTime_ms, unsigned updateDelay, bool continuousOutput)
	: LedDevice()
	, _ledDevice(ledDevice)
	, _updateInterval(1000 / ledUpdateFrequency_hz)
	, _settlingTime(settlingTime_ms)
	, _timer()
	, _outputDelay(updateDelay)
	, _writeToLedsEnable(true)
	, _continuousOutput(continuousOutput)
	, _pause(false)
	, _currentConfigId(0)
{
	_log = Logger::getInstance("Smoothing");
	_timer.setSingleShot(false);
	_timer.setInterval(_updateInterval);

	selectConfig( addConfig(_settlingTime, ledUpdateFrequency_hz, updateDelay) );
	
	// add pause on cfg 1
	SMOOTHING_CFG cfg = {true, 100, 50, 0};
	_cfgList.append(cfg);
	Info( _log, "smoothing cfg %d: pause",  _cfgList.count()-1);

	connect(&_timer, SIGNAL(timeout()), this, SLOT(updateLeds()));
}

LinearColorSmoothing::~LinearColorSmoothing()
{
	// Make sure to switch off the underlying led-device (because switchOff is no longer forwarded)
	_ledDevice->switchOff();
	delete _ledDevice;
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
		_timer.start();
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
			_ledDevice->setLedValues(ledColors);
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
					_ledDevice->setLedValues(_outputQueue.front());
				}
				_outputQueue.pop_front();
			}
		}
	}
}


void LinearColorSmoothing::setEnable(bool enable)
{
	LedDevice::setEnable(enable);

	if (!enable)
	{
		_timer.stop();
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
	
	Info( _log, "smoothing cfg %d: interval: %d ms, settlingTime: %d ms, updateDelay: %d frames",  _cfgList.count()-1, cfg.updateInterval, cfg.settlingTime,  cfg.outputDelay );
	return _cfgList.count() - 1;
}

bool LinearColorSmoothing::selectConfig(unsigned cfg)
{
	if (_currentConfigId == cfg)
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
			_timer.stop();
			_updateInterval = _cfgList[cfg].updateInterval;
			_timer.setInterval(_updateInterval);
			_timer.start();
		}
		_currentConfigId = cfg;
		InfoIf( enabled() && !_pause, _log, "set smoothing cfg: %d, interval: %d ms, settlingTime: %d ms, updateDelay: %d frames",  _currentConfigId, _updateInterval, _settlingTime,  _outputDelay );
		InfoIf( _pause, _log, "set smoothing cfg: %d, pause",  _currentConfigId );

		return true;
	}
	
	// reset to default
	_currentConfigId = 0;
	return false;
}
	
