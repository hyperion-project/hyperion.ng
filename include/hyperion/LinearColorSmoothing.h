#ifndef LINEARCOLORSMOOTHING_H
#define LINEARCOLORSMOOTHING_H

// STL includes
#include <vector>
#include <deque>

// Qt includes
#include <QVector>

// hyperion includes
#include <leddevice/LedDevice.h>
#include <utils/Components.h>
#include <hyperion/PriorityMuxer.h>

// settings
#include <utils/settings.h>

// The type of float
#define floatT float // Select double, float or __fp16

class QTimer;
class Logger;
class Hyperion;

enum SmoothingConfigID
{
	SYSTEM = 0,
	PAUSE = 1,
	EFFECT_DYNAMIC = 2,
	EFFECT_SPECIFIC = 3
};

/// Linear Smoothing class
///
/// This class processes the requested led values and forwards them to the device after applying
/// a smoothing effect to LED colors. This class can be handled as a generic LedDevice.
///
/// Currently, two types of smoothing are supported:
///
///  - Linear: A linear smoothing effect that interpolates the previous to the target colors.
///  - Decay: A temporal smoothing effect that uses a decay based algorithm that interpolates
///           colors based on the age of previous frames and a given decay-power.
///
///           The smoothing is performed on a history of relevant LED-color frames that are
///           incorporated in the smoothing window (given by the configured settling time).
///
///           For each moment, all ingress frames that were received during the smoothing window
///           are reduced to the concrete color values using a weighted moving average. This is
///           done by applying a decay-controlled weighting-function to individual the colors of
///           each frame.
///
///           Decay
///           =====
///           The decay-power influences the weight of individual frames based on their 'age'.
///
///           * A decay value of 1 indicates linear decay. The colors are given by the moving average
///           with a weight that is strictly proportionate to the fraction of time each frame was
///           visible during the smoothing window. As a result, equidistant frames will have an
///           equal share when calculating an intermediate frame.
///
///           * A decay value greater than 1 indicates non-linear decay. With higher powers, the
///           decay is stronger. I.e. newer frames in the smoothing window will have more influence
///           on colors of intermediate frames than older ones.
///
///           Dithering
///           =========
///           A temporal dithering algorithm is used to minimize rounding errors, when downsampling
///           the average color values to the 8-bit RGB resolution of the LED-device. Effectively,
///           this performs diffusion of the residual errors across multiple egress frames.
///
///

class LinearColorSmoothing : public QObject
{
	Q_OBJECT

public:
	/// Constructor
	/// @param config    The configuration document smoothing
	/// @param hyperion  The hyperion parent instance
	///
	LinearColorSmoothing(const QJsonDocument &config, Hyperion *hyperion);
	~LinearColorSmoothing() override;

	/// LED values as input for the smoothing filter
	///
	/// @param ledValues The color-value per led
	/// @return Zero on success else negative
	///
	virtual int updateLedValues(const std::vector<ColorRgb> &ledValues);

	void setEnable(bool enable);
	void setPause(bool pause);
	bool pause() const { return _pause; }
	bool enabled() const { return _enabled && !_pause; }

	///
	/// @brief Add a new smoothing configuration which can be used with selectConfig()
	/// @param   settlingTime_ms       The buffer time
	/// @param   ledUpdateFrequency_hz The frequency of update
	/// @param   updateDelay           The delay
	///
	/// @return The index of the configuration, which can be passed to selectConfig()
	///
	unsigned addConfig(int settlingTime_ms, double ledUpdateFrequency_hz = 25.0, unsigned updateDelay = 0);

	///
	/// @brief Update a smoothing cfg which can be used with selectConfig()
	///	       In case the ID does not exist, a smoothing cfg is added
	///
	/// @param   cfgID				   Smoothing configuration item to be updated
	/// @param   settlingTime_ms       The buffer time
	/// @param   ledUpdateFrequency_hz The frequency of update
	/// @param   updateDelay           The delay
	///
	/// @return The index of the configuration, which can be passed to selectConfig()
	///
	unsigned updateConfig(int cfgID, int settlingTime_ms, double ledUpdateFrequency_hz = 25.0, unsigned updateDelay = 0);

	///
	/// @brief select a smoothing configuration given by cfg index from addConfig()
	/// @param   cfgID   The index to use
	/// @param   force   Overwrite in any case the current values (used for cfg 0 settings update)
	///
	/// @return  On success return else false (and falls back to cfg 0)
	///
	bool selectConfig(int cfgID, bool force = false);

public slots:
	///
	/// @brief Handle settings update from Hyperion Settingsmanager emit or this constructor
	/// @param type   settingType from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(settings::type type, const QJsonDocument &config);

private slots:
	/// Timer callback which writes updated led values to the led device
	void updateLeds();

	///
	/// @brief Handle component state changes
	/// @param component   The component
	/// @param state       The requested state
	///
	void componentStateChange(hyperion::Components component, bool state);

private:
	/**
	 * Pushes the colors into the output queue and popping the head to the led-device
	 *
	 * @param ledColors The colors to queue
	 */
	void queueColors(const std::vector<ColorRgb> &ledColors);
	void clearQueuedColors();

	/// write updated values as input for the smoothing filter
	///
	/// @param ledValues The color-value per led
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector<ColorRgb> &ledValues);

	QString getConfig(int cfgID);

	/// Logger instance
	Logger *_log;

	/// Hyperion instance
	Hyperion *_hyperion;

	/// priority muxer instance
	PriorityMuxer* _prioMuxer;

	/// The interval at which to update the leds (msec)
	int _updateInterval;

	/// The time after which the updated led values have been fully applied (msec)
	int64_t _settlingTime;

	/// The Qt timer object
	QTimer *_timer;

	/// The timestamp at which the target data should be fully applied
	int64_t _targetTime;

	/// The target led data
	std::vector<ColorRgb> _targetValues;

	/// The timestamp of the previously written led data
	int64_t _previousWriteTime;

	/// The timestamp of the previously data interpolation
	int64_t _previousInterpolationTime;

	/// The previously written led data
	std::vector<ColorRgb> _previousValues;

	/// The number of updates to keep in the output queue (delayed) before being output
	unsigned _outputDelay;

	/// The output queue
	std::deque<std::vector<ColorRgb>> _outputQueue;

	/// A frame of led colors used for temporal smoothing
	class REMEMBERED_FRAME
	{
		public:
		/// The time this frame was received
		int64_t time;

		/// The led colors
		std::vector<ColorRgb> colors;

		REMEMBERED_FRAME ( REMEMBERED_FRAME && ) = default;
		REMEMBERED_FRAME ( const REMEMBERED_FRAME & ) = default;
		REMEMBERED_FRAME & operator= ( const REMEMBERED_FRAME & ) = default;

		REMEMBERED_FRAME(int64_t time, const std::vector<ColorRgb> colors)
		: time(time)
		, colors(colors)
		{}
	};


	/// The queue of temporarily remembered frames
	std::deque<REMEMBERED_FRAME> _frameQueue;

	/// Flag for pausing
	bool _pause;

	/// The rate at which color frames should be written to LED device.
	double _outputRate;

	/// The interval time in microseconds for writing of LED Frames.
	int64_t _outputIntervalMicros;

	/// The rate at which interpolation of LED frames should be performed.
	double _interpolationRate;

	/// The interval time in microseconds for interpolation of LED Frames.
	int64_t _interpolationIntervalMicros;

	/// Whether to apply temporal dithering to diffuse rounding errors when downsampling to 8-bit RGB colors.
	bool _dithering;

	/// The decay power > 0. A value of exactly 1 is linear decay, higher numbers indicate a faster decay rate.
	double _decay;

	/// Value of 1.0 / settlingTime; inverse of the window size used for weighting of frames.
	floatT _invWindow;

	enum class SmoothingType { Linear = 0, Decay = 1 };

	class SmoothingCfg
	{
	public:
		/// Whether to pause output
		bool _pause;

		/// The time of the smoothing window.
		int64_t _settlingTime;

		/// The interval time in milliseconds of the timer used for scheduling LED update operations. A value of 0 indicates sub-millisecond timing.
		int _updateInterval;

		/// The type of smoothing to perform
		SmoothingType _type;

		/// The rate at which color frames should be written to LED device.
		double _outputRate;

		/// The rate at which interpolation of LED frames should be performed.
		double _interpolationRate;

		/// The number of frames the output is delayed
		unsigned _outputDelay;

		/// Whether to apply temporal dithering to diffuse rounding errors when downsampling to 8-bit RGB colors. Improves color accuracy.
		bool _dithering;

		/// The decay power > 0. A value of exactly 1 is linear decay, higher numbers indicate a faster decay rate.
		double _decay;

		SmoothingCfg();
		SmoothingCfg(bool pause, int64_t settlingTime, int updateInterval, SmoothingType type = SmoothingType::Linear, double outputRate = 0, double interpolationRate = 0, unsigned outputDelay = 0, bool dithering = false, double decay = 1);

		static QString EnumToString(SmoothingType type);
	};

	/// smoothing configurations
	QVector<SmoothingCfg> _cfgList;

	int _currentConfigId;
	bool _enabled;

	/// The type of smoothing to perform
	SmoothingType _smoothingType;

	/// Pushes the colors into the frame queue and cleans outdated frames from memory.
	///
	/// @param ledColors The next colors to queue
	void rememberFrame(const std::vector<ColorRgb> &ledColors);

	/// Frees the LED frames that were queued for calculating the moving average.
	void clearRememberedFrames();

	/// (Re-)Initializes the color-component vectors with given number of values.
	///
	/// @param ledCount The number of colors.
	void intitializeComponentVectors(size_t ledCount);

	/// The number of led component-values that must be held per color; i.e. size of the color vectors reds / greens / blues
	size_t _ledCount = 0;

	/// The average component colors red, green, blue of the leds
	std::vector<floatT> meanValues;

	/// The residual component errors of the leds
	std::vector<floatT> residualErrors;

	/// The accumulated led color values in 64-bit fixed point domain
	std::vector<uint64_t> tempValues;

	/// Writes the target frame RGB data to the LED device without any interpolation.
	void writeDirect();

	/// Writes the assembled RGB data to the LED device.
	void writeFrame();

	/// Assembles a frame of LED colors in order to write RGB data to the LED device.
	/// Temporal dithering is applied to diffuse the downsampling error for RGB color components.
	void assembleAndDitherFrame();

	/// Assembles a frame of LED colors in order to write RGB data to the LED device.
	/// No dithering is applied, RGB color components are just rounded to nearest integer.
	void assembleFrame();

	/// Prepares a frame of LED colors by interpolating using the current smoothing window
	void interpolateFrame();

	/// Performs a decay-based smoothing effect. The frames are interpolated based on their age and a given decay-power.
	///
	/// The ingress frames that were received during the current smoothing window are reduced using a weighted moving average
	/// by applying the weighting-function to the color components of each frame.
	///
	/// When downsampling the average color values to the 8-bit RGB resolution of the LED device, rounding errors are minimized
	/// by temporal dithering algorithm (error diffusion of residual errors).
	void performDecay(int64_t now);

	/// Performs a linear smoothing effect
	void performLinear(int64_t now);

	/// Aggregates the RGB components of the LED colors using the given weight and updates weighted accordingly
	///
	/// @param colors The LED colors to aggregate.
	/// @param weighted The target vector, that accumulates the terms.
	/// @param weight The weight to use.
	static inline void aggregateComponents(const std::vector<ColorRgb>& colors, std::vector<uint64_t>& weighted, const floatT weight);

	/// Gets the current time in microseconds from high precision system clock.
	static inline int64_t micros() ;

	/// The time, when the rendering statistics were logged previously
	int64_t _renderedStatTime;

	/// The total number of frames that were rendered to the LED device
	int64_t _renderedCounter;

	/// The count of frames that have been rendered to the LED device when statistics were shown previously
	int64_t _renderedStatCounter;

	/// The total number of frames that were interpolated using the smoothing algorithm
	int64_t _interpolationCounter;

	/// The count of frames that have been interpolated when statistics were shown previously
	int64_t _interpolationStatCounter;

	/// Frame weighting function for finding the frame's integral value
	///
	/// @param frameStart The start of frame time.
	/// @param frameEnd The end of frame time.
	/// @param windowStart The window start time.
	/// @returns The frame weight.
	std::function<floatT(int64_t, int64_t, int64_t)> _weightFrame;
};

#endif // LINEARCOLORSMOOTHING_H
