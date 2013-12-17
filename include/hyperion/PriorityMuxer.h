#pragma once

// STL includes
#include <vector>
#include <map>
#include <cstdint>
#include <limits>

// QT includes
#include <QMap>

// Utils includes
#include <utils/ColorRgb.h>

///
/// The PriorityMuxer handles the priority channels. Led values input is written to the priority map
/// and the muxer keeps track of all active priorities. The current priority can be queried and per
/// priority the led colors.
///
class PriorityMuxer
{
public:
	///
	/// The information structure for a single priority channel
	///
	struct InputInfo
	{
		/// The priority of this channel
		int priority;

		/// The absolute timeout of the channel
		int64_t timeoutTime_ms;
		/// The colors for each led of the channel
		std::vector<ColorRgb> ledColors;
	};

	///
	/// Constructs the PriorityMuxer for the given number of leds (used to switch to black when
	/// there are no priority channels
	///
	/// @param ledCount The number of leds
	///
	PriorityMuxer(int ledCount);

	///
	/// Destructor
	///
	~PriorityMuxer();

	///
	/// Returns the current priority
	///
	/// @return The current priority
	///
	int getCurrentPriority() const;

	///
	/// Returns the state (enabled/disabled) of a specific priority channel
	/// @param priority The priority channel
	/// @return True if the priority channel exists else false
	///
	bool hasPriority(const int priority) const;

	///
	/// Returns the number of active priorities
	///
	/// @return The list with active priorities
	///
	QList<int> getPriorities() const;

	///
	/// Returns the information of a specified priority channel
	///
	/// @param priority The priority channel
	///
	/// @return The information for the specified priority channel
	///
	/// @throws std::runtime_error if the priority channel does not exist
	///
	const InputInfo& getInputInfo(const int priority) const;

	///
	/// Sets/Updates the data for a priority channel
	///
	/// @param[in] priority The priority of the channel
	/// @param[in] ledColors The led colors of the priority channel
	/// @param[in] timeoutTime_ms The absolute timeout time of the channel
	///
	void setInput(const int priority, const std::vector<ColorRgb>& ledColors, const int64_t timeoutTime_ms=-1);

	///
	/// Clears the specified priority channel
	///
	/// @param[in] priority  The priority of the channel to clear
	///
	void clearInput(const int priority);

	///
	/// Clears all priority channels
	///
	void clearAll();

	///
	/// Updates the current time. Channels with a configured time out will be checked and cleared if
	/// required.
	///
	/// @param[in] now The current time
	///
	void setCurrentTime(const int64_t& now);

private:
	/// The current priority (lowest value in _activeInputs)
	int _currentPriority;

	/// The mapping from priority channel to led-information
	QMap<int, InputInfo> _activeInputs;

	/// The information of the lowest priority channel
	InputInfo _lowestPriorityInfo;

	/// The lowest possible priority, which is used when no priority channels are active
	const static int LOWEST_PRIORITY = std::numeric_limits<int>::max();
};
