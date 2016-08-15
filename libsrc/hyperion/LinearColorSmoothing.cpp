// Qt includes
#include <QDateTime>

#include "LinearColorSmoothing.h"

using namespace hyperion;

LinearColorSmoothing::LinearColorSmoothing( LedDevice * ledDevice, double ledUpdateFrequency_hz, int settlingTime_ms, unsigned updateDelay, bool continuousOutput)
	: LedDevice()
	, _ledDevice(ledDevice)
	, _updateInterval(1000 / ledUpdateFrequency_hz)
	, _settlingTime(settlingTime_ms)
	, _timer()
	, _outputDelay(updateDelay)
	, _writeToLedsEnable(true)
	, _continuousOutput(continuousOutput)
	, _bypass(false)
{
	_log = Logger::getInstance("Smoothing");
	_timer.setSingleShot(false);
	_timer.setInterval(_updateInterval);

	connect(&_timer, SIGNAL(timeout()), this, SLOT(updateLeds()));

	Info( _log, "Created linear-smoothing with interval: %d ms, settlingTime: %d ms, updateDelay: %d frames",
	      _updateInterval, settlingTime_ms,  _outputDelay );
}

LinearColorSmoothing::~LinearColorSmoothing()
{
	// Make sure to switch off the underlying led-device (because switchOff is no longer forwarded)
	_ledDevice->switchOff();
	delete _ledDevice;
}

int LinearColorSmoothing::write(const std::vector<ColorRgb> &ledValues)
{
	if (_bypass)
	{
		_ledDevice->write(ledValues);
	}
	else
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

		for (size_t i = 0; i < _previousValues.size(); ++i)
		{
			ColorRgb & prev = _previousValues[i];
			ColorRgb & target = _targetValues[i];

			prev.red   += k * (target.red   - prev.red);
			prev.green += k * (target.green - prev.green);
			prev.blue  += k * (target.blue  - prev.blue);
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
		if ( _writeToLedsEnable )
			_ledDevice->write(ledColors);
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
				_ledDevice->write(_outputQueue.front());
				_outputQueue.pop_front();
			}
		}
	}
}

void LinearColorSmoothing::componentStateChanged(const hyperion::Components component, bool enable)
{
	if (component == COMP_SMOOTHING && _bypass == enable)
	{
		_bypass = !enable;
		Info(_log, "change state to %s", (enable ? "enabled" : "disabled") );
	}
}

