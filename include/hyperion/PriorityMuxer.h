#pragma once

// STL includes
#include <vector>
#include <map>
#include <cstdint>
#include <limits>

// QT includes
#include <QMap>

// Utils includes
#include <utils/RgbColor.h>

// Hyperion includes
#include <hyperion/LedDevice.h>

class PriorityMuxer
{
public:
	struct InputInfo
	{
		int priority;

		int64_t timeoutTime_ms;
		std::vector<RgbColor> ledColors;
	};

	PriorityMuxer(int ledCount);

	~PriorityMuxer();

	int getCurrentPriority() const;

	bool hasPriority(const int priority) const;

	QList<int> getPriorities() const;

	const InputInfo& getInputInfo(const int priority) const;

	void setInput(const int priority, const std::vector<RgbColor>& ledColors, const int64_t timeoutTime_ms=-1);

	void clearInput(const int priority);

	void clearAll();

	void setCurrentTime(const int64_t& now);

private:
	int _currentPriority;

	QMap<int, InputInfo> _activeInputs;

	InputInfo _lowestPriorityInfo;

	const static int LOWEST_PRIORITY = std::numeric_limits<int>::max();
};
