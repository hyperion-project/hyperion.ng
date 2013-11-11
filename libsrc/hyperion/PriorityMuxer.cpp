
// STL includes
#include <algorithm>
#include <stdexcept>

// Hyperion includes
#include <hyperion/PriorityMuxer.h>

PriorityMuxer::PriorityMuxer(int ledCount) :
	_currentPriority(LOWEST_PRIORITY),
	_activeInputs(),
	_lowestPriorityInfo()
{
	_lowestPriorityInfo.priority = LOWEST_PRIORITY;
	_lowestPriorityInfo.timeoutTime_ms = -1;
	_lowestPriorityInfo.ledColors = std::vector<ColorRgb>(ledCount, {0, 0, 0});
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
	return _activeInputs.contains(priority);
}

const PriorityMuxer::InputInfo& PriorityMuxer::getInputInfo(const int priority) const
{
	if (priority == LOWEST_PRIORITY)
	{
		return _lowestPriorityInfo;
	}

	auto elemIt = _activeInputs.find(priority);
	if (elemIt == _activeInputs.end())
	{
		throw std::runtime_error("no such priority");
	}
	return elemIt.value();
}

void PriorityMuxer::setInput(const int priority, const std::vector<ColorRgb>& ledColors, const int64_t timeoutTime_ms)
{
	InputInfo& input = _activeInputs[priority];
	input.priority       = priority;
	input.timeoutTime_ms = timeoutTime_ms;
	input.ledColors      = ledColors;

	_currentPriority = std::min(_currentPriority, priority);
}

void PriorityMuxer::clearInput(const int priority)
{
	_activeInputs.remove(priority);
	if (_currentPriority == priority)
	{
		if (_activeInputs.empty())
		{
			_currentPriority = LOWEST_PRIORITY;
		}
		else
		{
			QList<int> keys = _activeInputs.keys();
			_currentPriority = *std::min_element(keys.begin(), keys.end());
		}
	}
}

void PriorityMuxer::clearAll()
{
	_activeInputs.clear();
	_currentPriority = LOWEST_PRIORITY;
}

void PriorityMuxer::setCurrentTime(const int64_t& now)
{
	_currentPriority = LOWEST_PRIORITY;

	for (auto infoIt = _activeInputs.begin(); infoIt != _activeInputs.end();)
	{
		if (infoIt->timeoutTime_ms != -1 && infoIt->timeoutTime_ms <= now)
		{
			infoIt = _activeInputs.erase(infoIt);
		}
		else
		{
			_currentPriority = std::min(_currentPriority, infoIt->priority);
			++infoIt;
		}
	}
}
