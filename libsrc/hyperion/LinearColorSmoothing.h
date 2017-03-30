#pragma once

// STL includes
#include <vector>


// Qt includes
#include <QTimer>

// hyperion incluse
#include <leddevice/LedDevice.h>
#include <utils/Components.h>

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
	/// @param LedUpdatFrequency The frequency at which the leds will be updated (Hz)
	/// @param settingTime The time after which the updated led values have been fully applied (sec)
	/// @param updateDelay The number of frames to delay outgoing led updates
	LinearColorSmoothing(LedDevice *ledDevice, double ledUpdateFrequency, int settlingTime, unsigned updateDelay, bool continuousOutput);

	/// Destructor
	virtual ~LinearColorSmoothing();

	/// write updated values as input for the smoothing filter
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb> &ledValues);

	/// Switch the leds off
	virtual int switchOff();

	void setEnable(bool enable);
	void setPause(bool pause);
	bool pause() { return _pause; } ;
	bool enabled() { return LedDevice::enabled() && !_pause; };

private slots:
	/// Timer callback which writes updated led values to the led device
	void updateLeds();

private:
	/**
	 * Pushes the colors into the output queue and popping the head to the led-device
	 *
	 * @param ledColors The colors to queue
	 */
	void queueColors(const std::vector<ColorRgb> & ledColors);

	/// The led device
	LedDevice * _ledDevice;

	/// The interval at which to update the leds (msec)
	const int64_t _updateInterval;

	/// The time after which the updated led values have been fully applied (msec)
	const int64_t _settlingTime;

	/// The Qt timer object
	QTimer _timer;

	/// The timestamp at which the target data should be fully applied
	int64_t _targetTime;

	/// The target led data
	std::vector<ColorRgb> _targetValues;

	/// The timestamp of the previously written led data
	int64_t _previousTime;

	/// The previously written led data
	std::vector<ColorRgb> _previousValues;

	/// The number of updates to keep in the output queue (delayed) before being output
	const unsigned _outputDelay;
	/// The output queue
	std::list<std::vector<ColorRgb> > _outputQueue;

	/// Prevent sending data to device when no intput data is sent
	bool _writeToLedsEnable;
	
	/// Flag for dis/enable continuous output to led device regardless there is new data or not
	bool _continuousOutput;

	/// Flag for pausing
	bool _pause;
};
