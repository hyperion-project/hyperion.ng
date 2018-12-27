#pragma once

// STL includes
#include <vector>


// Qt includes
#include <QVector>

// hyperion incluse
#include <leddevice/LedDevice.h>
#include <utils/Components.h>

// settings
#include <utils/settings.h>

class QTimer;
class Logger;
class Hyperion;

/// Linear Smooting class
///
/// This class processes the requested led values and forwards them to the device after applying
/// a linear smoothing effect. This class can be handled as a generic LedDevice.
class LinearColorSmoothing : public LedDevice
{
	Q_OBJECT

public:
	/// Constructor
	/// @param LedDevice the led device
	/// @param config    The configuration document smoothing
	/// @param hyperion  The hyperion parent instance
	///
	LinearColorSmoothing(LedDevice *ledDevice, const QJsonDocument& config, Hyperion* hyperion);

	/// Destructor
	virtual ~LinearColorSmoothing();

	/// write updated values as input for the smoothing filter
	///
	/// @param ledValues The color-value per led
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector<ColorRgb> &ledValues);

	/// Switch the leds off
	virtual int switchOff();

	void setEnable(bool enable);
	void setPause(bool pause);
	bool pause() { return _pause; } ;
	bool enabled() { return LedDevice::enabled() && !_pause; };

	///
	/// @brief Add a new smoothing cfg which can be used with selectConfig()
	/// @param   settlingTime_ms       The buffer time
	/// @param   ledUpdateFrequency_hz The frequency of update
	/// @param   updateDelay           The delay
	///
	/// @return The index of the cfg which can be passed to selectConfig()
	///
	unsigned addConfig(int settlingTime_ms, double ledUpdateFrequency_hz=25.0, unsigned updateDelay=0);

	///
	/// @brief select a smoothing cfg given by cfg index from addConfig()
	/// @param   cfg     The index to use
	/// @param   force   Overwrite in any case the current values (used for cfg 0 settings udpate)
	///
	/// @return  On success return else false (and falls back to cfg 0)
	///
	bool selectConfig(unsigned cfg, const bool& force = false);

	///
	/// @ Helper methods to start the timer with delay (see delayStartSmooth())
	///
	void startTimerDelayed();
	void stopTimer();

public slots:
	///
	/// @brief Handle settings update from Hyperion Settingsmanager emit or this constructor
	/// @param type   settingyType from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(const settings::type& type, const QJsonDocument& config);

private slots:
	/// Timer callback which writes updated led values to the led device
	void updateLeds();

	/// Delay timer slot to workaround the leddevice reconstruction segfault (dangling pointer)
	void delayStartTimer();

	///
	/// @brief Handle component state changes
	/// @param component   The component
	/// @param state       The requested state
	///
	void componentStateChange(const hyperion::Components component, const bool state);

private:
	/**
	 * Pushes the colors into the output queue and popping the head to the led-device
	 *
	 * @param ledColors The colors to queue
	 */
	void queueColors(const std::vector<ColorRgb> & ledColors);

	/// The led device
	LedDevice * _ledDevice;

	/// Logger instance
	Logger* _log;

	/// Hyperion instance
	Hyperion* _hyperion;

	/// The interval at which to update the leds (msec)
	int64_t _updateInterval;

	/// The time after which the updated led values have been fully applied (msec)
	int64_t _settlingTime;

	/// The Qt timer object
	QTimer * _timer;

	/// The timestamp at which the target data should be fully applied
	int64_t _targetTime;

	/// The target led data
	std::vector<ColorRgb> _targetValues;

	/// The timestamp of the previously written led data
	int64_t _previousTime;

	/// The previously written led data
	std::vector<ColorRgb> _previousValues;

	/// The number of updates to keep in the output queue (delayed) before being output
	unsigned _outputDelay;
	/// The output queue
	std::list<std::vector<ColorRgb> > _outputQueue;

	/// Prevent sending data to device when no intput data is sent
	bool _writeToLedsEnable;

	/// Flag for dis/enable continuous output to led device regardless there is new data or not
	bool _continuousOutput;

	/// Flag for pausing
	bool _pause;

	struct SMOOTHING_CFG
	{
		bool     pause;
		int64_t  settlingTime;
		int64_t  updateInterval;
		unsigned outputDelay;
	};

	/// smooth config list
	QVector<SMOOTHING_CFG> _cfgList;

	unsigned _currentConfigId;
};
