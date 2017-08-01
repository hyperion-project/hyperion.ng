#include <iostream>
// STL includes
#include <algorithm>
#include <stdexcept>

// Hyperion includes
#include <hyperion/PriorityMuxer.h>

PriorityMuxer::PriorityMuxer(int ledCount)
	: _currentPriority(LOWEST_PRIORITY)
	, _activeInputs()
	, _lowestPriorityInfo()
{
	_lowestPriorityInfo.priority       = LOWEST_PRIORITY;
	_lowestPriorityInfo.timeoutTime_ms = 0;
	_lowestPriorityInfo.ledColors      = std::vector<ColorRgb>(ledCount, {0, 0, 0});
	_lowestPriorityInfo.componentId    = hyperion::COMP_COLOR;
	_lowestPriorityInfo.origin         = "System";

	_activeInputs[_currentPriority] = _lowestPriorityInfo;

	// do a reuqest after blocking timer runs out
	connect(&_timer, SIGNAL(timeout()), this, SLOT(emitReq()));
	_timer.setSingleShot(true);
	_blockTimer.setSingleShot(true);
}

PriorityMuxer::~PriorityMuxer()
{
	// empty
}

int PriorityMuxer::getCurrentPriority() const
{
	return _currentPriority;
}

QList<int> PriorityMuxer::getPriorities() const
{
	return _activeInputs.keys();
}

bool PriorityMuxer::hasPriority(const int priority) const
{
	return (priority == LOWEST_PRIORITY) ? true : _activeInputs.contains(priority);
}

const PriorityMuxer::InputInfo& PriorityMuxer::getInputInfo(const int priority) const
{
	auto elemIt = _activeInputs.find(priority);
	if (elemIt == _activeInputs.end())
	{
		throw std::runtime_error("HYPERION (prioritymuxer) ERROR: no such priority");
	}
	return elemIt.value();
}

void PriorityMuxer::setInput(const int priority, const std::vector<ColorRgb>& ledColors, const int64_t timeoutTime_ms, hyperion::Components component, const QString origin)
{
	InputInfo& input     = _activeInputs[priority];
	input.priority       = priority;
	input.timeoutTime_ms = timeoutTime_ms;
	input.ledColors      = ledColors;
	input.componentId    = component;
	input.origin         = origin;
	_currentPriority     = std::min(_currentPriority, priority);
}

void PriorityMuxer::clearInput(const int priority)
{
	if (priority < LOWEST_PRIORITY)
	{
		_activeInputs.remove(priority);
		if (_currentPriority == priority)
		{
			QList<int> keys = _activeInputs.keys();
			_currentPriority = *std::min_element(keys.begin(), keys.end());
		}
	}
}

void PriorityMuxer::clearAll()
{
	for(auto key : _activeInputs.keys())
	{
		if (key < LOWEST_PRIORITY-1)
		{
			_activeInputs.remove(key);
		}
	}
}

void PriorityMuxer::setCurrentTime(const int64_t& now)
{
	_currentPriority = LOWEST_PRIORITY;

	for (auto infoIt = _activeInputs.begin(); infoIt != _activeInputs.end();)
	{
		if (infoIt->timeoutTime_ms > 0 && infoIt->timeoutTime_ms <= now)
		{
			infoIt = _activeInputs.erase(infoIt);
		}
		else
		{			
			_currentPriority = std::min(_currentPriority, infoIt->priority);
			
			// call emitReq when effect or color is running with timeout > -1, blacklist prio 255
			if(infoIt->priority < 254 && infoIt->timeoutTime_ms > -1 && (infoIt->componentId == hyperion::COMP_EFFECT || infoIt->componentId == hyperion::COMP_COLOR))
			{
				emitReq();
			}
			++infoIt;
		}
	}
}

void PriorityMuxer::emitReq()
{
	if(_blockTimer.isActive())
	{
		_timer.start(500);
	}
	else
	{
		emit timerunner();
		_blockTimer.start(1000);
	}
}
