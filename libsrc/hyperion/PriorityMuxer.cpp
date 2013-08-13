
// STL includes
#include <algorithm>
#include <stdexcept>

// Hyperion includes
#include <hyperion/PriorityMuxer.h>

PriorityMuxer::PriorityMuxer() :
	mCurrentPriority(MAX_PRIORITY)
{
	// empty
}

PriorityMuxer::~PriorityMuxer()
{
	// empty
}

int PriorityMuxer::getCurrentPriority() const
{
	return mCurrentPriority;
}

QList<int> PriorityMuxer::getPriorities() const
{
	return mActiveInputs.keys();
}

bool PriorityMuxer::hasPriority(const int priority) const
{
	return mActiveInputs.contains(priority);
}

const PriorityMuxer::InputInfo& PriorityMuxer::getInputInfo(const int priority) const
{
	auto elemIt = mActiveInputs.find(priority);
	if (elemIt == mActiveInputs.end())
	{
		throw std::runtime_error("no such priority");
	}
	return elemIt.value();
}

void PriorityMuxer::setInput(const int priority, const std::vector<RgbColor>& ledColors, const int64_t timeoutTime_ms)
{
	InputInfo& input = mActiveInputs[priority];
	input.priority       = priority;
	input.timeoutTime_ms = timeoutTime_ms;
	input.ledColors      = ledColors;

	mCurrentPriority = std::min(mCurrentPriority, priority);
}

void PriorityMuxer::clearInput(const int priority)
{
	mActiveInputs.remove(priority);
	if (mCurrentPriority == priority)
	{
		if (mActiveInputs.empty())
		{
			mCurrentPriority = MAX_PRIORITY;
		}
		else
		{
			QList<int> keys = mActiveInputs.keys();
			mCurrentPriority = *std::min_element(keys.begin(), keys.end());
		}
	}
}

void PriorityMuxer::clearAll()
{
	mActiveInputs.clear();
	mCurrentPriority = MAX_PRIORITY;
}

void PriorityMuxer::setCurrentTime(const int64_t& now)
{
	mCurrentPriority = MAX_PRIORITY;

	for (auto infoIt = mActiveInputs.begin(); infoIt != mActiveInputs.end();)
	{
		if (infoIt->timeoutTime_ms != -1 && infoIt->timeoutTime_ms <= now)
		{
			infoIt = mActiveInputs.erase(infoIt);
		}
		else
		{
			mCurrentPriority = std::min(mCurrentPriority, infoIt->priority);
			++infoIt;
		}
	}
}
