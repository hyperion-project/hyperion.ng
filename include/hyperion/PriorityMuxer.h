#pragma once

// STL includes
#include <vector>
#include <cstdint>

// QT includes
#include <QMap>
#include <QObject>
#include <QMap>
#include <QVector>

// Utils includes
#include <utils/ColorRgb.h>
#include <utils/Image.h>
#include <utils/Components.h>

// global defines
#define SMOOTHING_MODE_DEFAULT 0
#define SMOOTHING_MODE_PAUSE   1

class QTimer;
class Logger;

///
/// The PriorityMuxer handles the priority channels. Led values input/ images are written to the priority map
/// and the muxer keeps track of all active priorities. The current priority can be queried and per
/// priority the led colors. Handles also manual/auto selection mode, provides a lot of signals to hook into priority related events
///
class PriorityMuxer : public QObject
{
	Q_OBJECT
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
		/// The raw Image (size should be preprocessed!)
		Image<ColorRgb> image;
		/// The component
		hyperion::Components componentId;
		/// Who set it
		QString origin;
		/// id of smoothing config
		unsigned smooth_cfg;
		/// specific owner description
		QString owner;
	};

	typedef QMap<int, InputInfo> InputsMap;

	//Foreground and Background priorities
	const static int FG_PRIORITY;
	const static int BG_PRIORITY;
	const static int MANUAL_SELECTED_PRIORITY;
	/// The lowest possible priority, which is used when no priority channels are active
	const static int LOWEST_PRIORITY;
	/// Timeout used to identify a non active priority
	const static int TIMEOUT_NOT_ACTIVE_PRIO;
	const static int REMOVE_CLEARED_PRIO;

	const static int ENDLESS;

	///
	/// Constructs the PriorityMuxer for the given number of LEDs (used to switch to black when
	/// there are no priority channels
	///
	/// @param ledCount The number of LEDs
	///
	PriorityMuxer(int ledCount, QObject * parent);

	///
	/// Destructor
	///
	~PriorityMuxer() override;

	///
	/// @brief Start/Stop the PriorityMuxer update timer; On disabled no priority and timeout updates will be performend
	/// @param  enable  The new state
	///
	void setEnable(bool enable);

	/// @brief Enable or disable auto source selection
	/// @param   enable   True if it should be enabled else false
	/// @param   update   True to update _currentPriority - INTERNAL usage.
	/// @return           True if changed has been applied, false if the state is unchanged
	///
	bool setSourceAutoSelectEnabled(bool enable, bool update = true);

	///
	/// @brief Get the state of source auto selection
	/// @return  True if enabled, else false
	///
	bool isSourceAutoSelectEnabled() const { return _sourceAutoSelectEnabled; }

	///
	/// @brief  Overwrite current lowest priority with manual selection; On success disables auto selection
	/// @param   priority  The
	/// @return            True on success, false if priority not found
	///
	bool setPriority(int priority);

	///
	/// @brief Update all LED-Colors with min length of >= 1 to fit the new led length
	/// @param[in] ledCount   The count of LEDs
	///
	void updateLedColorsLength(int ledCount);

	///
	/// Returns the current priority
	///
	/// @return The current priority
	///
	int getCurrentPriority() const { return _currentPriority; }

	///
	/// Returns the previous priority before current priority
	///
	/// @return The previous priority
	///
	int getPreviousPriority() const { return _previousPriority; }

	///
	/// Returns the state (enabled/disabled) of a specific priority channel
	/// @param priority The priority channel
	/// @return True if the priority channel exists else false
	///
	bool hasPriority(int priority) const;

	///
	/// Returns the number of active priorities
	///
	/// @return The list with active priorities
	///
	QList<int> getPriorities() const;

	///
	/// Returns the information of a specified priority channel.
	/// If a priority is no longer available the _lowestPriorityInfo (255) is returned
	///
	/// @param priority The priority channel
	///
	/// @return The information for the specified priority channel
	///
	InputInfo getInputInfo(int priority) const;

	///
	/// @brief  Register a new input by priority, the priority is not active (timeout -100 isn't muxer recognized) until you start to update the data with setInput()
	/// 		A repeated call to update the base data of a known priority won't overwrite their current timeout
	/// @param[in] priority    The priority of the channel
	/// @param[in] component   The component of the channel
	/// @param[in] origin      Who set the channel (CustomString@IP)
	/// @param[in] owner       Specific owner string, might be empty
	/// @param[in] smooth_cfg  The smooth id to use
	///
	void registerInput(int priority, hyperion::Components component, const QString& origin = "System", const QString& owner = "", unsigned smooth_cfg = SMOOTHING_MODE_DEFAULT);

	///
	/// @brief   Update the current color of a priority (previous registered with registerInput())
	/// @param  priority    The priority to update
	/// @param  ledColors   The colors
	/// @param  timeout_ms  The new timeout (defaults to -1 endless)
	/// @return             True on success, false when priority is not found
	///
	bool setInput(int priority, const std::vector<ColorRgb>& ledColors, int64_t timeout_ms = ENDLESS);

	///
	/// @brief   Update the current image of a priority (prev registered with registerInput())
	/// @param  priority    The priority to update
	/// @param  image       The new image
	/// @param  timeout_ms  The new timeout (defaults to -1 endless)
	/// @return             True on success, false when priority is not found
	///
	bool setInputImage(int priority, const Image<ColorRgb>& image, int64_t timeout_ms = ENDLESS);

	///
	/// @brief Set the given priority to inactive
	/// @param priority  The priority
	/// @return True on success false if not found
	///
	bool setInputInactive(int priority);

	///
	/// Clears the specified priority channel and update _currentPriority on success
	///
	/// @param[in] priority  The priority of the channel to clear
	/// @return              True if priority has been cleared else false (not found)
	///
	bool clearInput(int priority);

	///
	/// Clears all priority channels
	///
	void clearAll(bool forceClearAll=false);

signals:

	///
	/// @brief Emits whenever the visible priority has changed
	/// @param  priority  The new visible priority
	///
	void visiblePriorityChanged(int priority);

	///
	/// @brief Emits whenever the current visible component changed
	/// @param comp  The new component
	///
	void visibleComponentChanged(hyperion::Components comp);

	///
	/// @brief Emits whenever something changes which influences the priorities listing

	///        Emits also in 1s interval when a COLOR or EFFECT is running with a timeout > -1
	/// @param  currentPriority The current priority at time of emit
	/// @param  activeInputs The current active input map at time of emit

	///
	void prioritiesChanged(int currentPriority, InputsMap activeInputs);

	///
	/// internal used signal to resolve treading issues with timer
	///
	void signalTimeTrigger();

private slots:
	///
	/// Slot which is called to adapt to 1s interval for signal prioritiesChanged()
	///
	void timeTrigger();

	///
	/// Updates the current priorities. Channels with a configured time out will be checked and cleared if
	/// required. Cleared priorities will be removed.
	///
	void updatePriorities();

private:
	///
	/// @brief Get the component of the given priority
	/// @return The component
	///
	hyperion::Components getComponentOfPriority(int priority) const;

	/// Logger instance
	Logger* _log;

	/// The current priority (lowest value in _activeInputs)
	int _currentPriority;

	/// The previous priority before current priority
	int _previousPriority;

	/// The manual select priority set with setPriority
	int _manualSelectedPriority;

	// The last visible component
	hyperion::Components _prevVisComp = hyperion::COMP_INVALID;

	/// The mapping from priority channel to led-information
	InputsMap _activeInputs;

	/// The information of the lowest priority channel
	InputInfo _lowestPriorityInfo;

	// Reflect the state of auto select
	bool _sourceAutoSelectEnabled;

	// Timer to update Muxer times independent
	QTimer* _updateTimer;

	QTimer* _timer;
	QTimer* _blockTimer;
};
