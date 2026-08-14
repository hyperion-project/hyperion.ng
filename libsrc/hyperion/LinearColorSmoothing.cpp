#include <hyperion/LinearColorSmoothing.h>

#include <cmath>
#include <chrono>
#include <thread>

#if defined(COMPILER_GCC)
#define ALWAYS_INLINE inline __attribute__((__always_inline__))
#elif defined(COMPILER_MSVC)
#define ALWAYS_INLINE __forceinline
#else
#define ALWAYS_INLINE inline
#endif

#include <QDateTime>
#include <QTimer>

#include <hyperion/Hyperion.h>

Q_LOGGING_CATEGORY(smoothing, "hyperion.smoothing")

/// Clamps the rounded values to the byte-interval of [0, 255].
ALWAYS_INLINE long clampRounded(const floatT x) {
	return static_cast<long>(qBound(0, qRound(x), 255));
}

// Constants
namespace {

/// The number of microseconds per millisecond = 1000.
const int64_t MS_PER_MICRO = 1000;

/// The number of microseconds per second = 1000000.
const int64_t MICROS_PER_SECOND = MS_PER_MICRO * MS_PER_MICRO;

/// The number of bits that are used for shifting the fixed point values
const int FPShift = (sizeof(uint64_t)*8 - (12 + 9));

/// The number of bits that are reduce the shifting when converting from fixed to floating point. 8 bits = 256 values
const int SmallShiftBis = sizeof(uint8_t)*8;

/// The number of bits that are used for shifting the fixed point values plus SmallShiftBis
const int FPShiftSmall = (sizeof(uint64_t)*8 - (12 + 9 + SmallShiftBis));

const char SETTINGS_KEY_SMOOTHING_TYPE[] = "type";

const char SETTINGS_KEY_SETTLING_TIME[] = "time_ms";
const char SETTINGS_KEY_UPDATE_FREQUENCY[] = "updateFrequency";
const char SETTINGS_KEY_OUTPUT_DELAY[] = "updateDelay";

const char SETTINGS_KEY_DECAY[] = "decay";
const char SETTINGS_KEY_INTERPOLATION_RATE[] = "interpolationRate";
const char SETTINGS_KEY_DITHERING[] = "dithering";

const int64_t DEFAULT_SETTLINGTIME = 200;	// in ms
const int DEFAULT_UPDATEFREQUENCY = 25;		// in Hz

constexpr std::chrono::milliseconds DEFAULT_UPDATEINTERVALL{MS_PER_MICRO/ DEFAULT_UPDATEFREQUENCY};
const unsigned DEFAULT_OUTPUTDEPLAY = 0;	// in frames

int updateIntervalMsFromFrequency(const double frequencyHz)
{
	return static_cast<int>(MS_PER_MICRO / frequencyHz);
}

int64_t outputIntervalMicrosFromFrequency(const double frequencyHz)
{
	return static_cast<int64_t>(std::llround(MICROS_PER_SECOND / frequencyHz));
}

void sleepUntilNextAction(const int64_t now, const int64_t nextActionExpected)
{
	const int64_t microsTillNextAction = nextActionExpected - now;
	const int64_t SLEEP_MAX_MICROS = 1000L; // We want to use usleep for up to 1ms
	const int64_t SLEEP_RES_MICROS = 100L; // Expected resolution is >= 100µs on stock linux

	if (microsTillNextAction > SLEEP_RES_MICROS)
	{
		const int64_t wait = std::min(microsTillNextAction - SLEEP_RES_MICROS, SLEEP_MAX_MICROS);
		std::this_thread::sleep_for(std::chrono::microseconds(wait));
	}
}
}

using namespace hyperion;

LinearColorSmoothing::LinearColorSmoothing(const QJsonObject &config, const QSharedPointer<Hyperion>& hyperionInstance)
	: QObject()
	, _smoothConfig(config)
	, _log(nullptr)
	, _hyperionWeak(hyperionInstance)
	, _prioMuxerWeak(nullptr)
	, _updateInterval(DEFAULT_UPDATEINTERVALL.count())
	, _settlingTime(DEFAULT_SETTLINGTIME)
	, _timer(nullptr)
	, _outputDelay(DEFAULT_OUTPUTDEPLAY)
	, _pause(false)
	, _outputIntervalMicros(DEFAULT_UPDATEINTERVALL.count() * MS_PER_MICRO)
	, _currentConfigId(SmoothingConfigID::SYSTEM)
	, _enabled(false)
	, _enabledSystemCfg(false)
	, _smoothingType(SmoothingType::Linear)
	, tempValues(std::vector<uint64_t>())
{
	QString subComponent{ "__" };
	QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
	if (hyperion)
	{
		subComponent = hyperion->property("instance").toString();
		_prioMuxerWeak = hyperion->getMuxerInstance();
	}
	_log= Logger::getInstance("SMOOTHING", subComponent);
	TRACK_SCOPE_SUBCOMPONENT();

	// init cfg (default)
	updateConfig(SmoothingConfigID::SYSTEM, DEFAULT_SETTLINGTIME, DEFAULT_UPDATEFREQUENCY, DEFAULT_OUTPUTDEPLAY);
	// add pause on cfg 1
	SmoothingCfg cfg {true, 0, 0};
	_cfgList.append(std::move(cfg));
}
LinearColorSmoothing::~LinearColorSmoothing()
{
	TRACK_SCOPE_SUBCOMPONENT();
}

void LinearColorSmoothing::start()
{
	Info(_log, "LinearColorSmoothing starting...");


	_timer.reset(new QTimer());
	_timer->setTimerType(Qt::PreciseTimer);

	//Start in pause mode, a new priority will activate smoothing (either start-effect or grabber)
	setPause(true);

	// listen for comp changes
	QObject::connect(_hyperionWeak.toStrongRef().get(), &Hyperion::compStateChangeRequest, this, &LinearColorSmoothing::componentStateChange);
	QObject::connect(_prioMuxerWeak.toStrongRef().get(), &PriorityMuxer::prioritiesChanged, this, &LinearColorSmoothing::handlePriorityUpdate);
	connect(_timer.get(), &QTimer::timeout, this, &LinearColorSmoothing::updateLeds);

	updateSettings(_smoothConfig);
}

void LinearColorSmoothing::stop()
{
	Debug(_log, "LinearColorSmoothing stopping...");

	QObject::disconnect(_prioMuxerWeak.toStrongRef().get(), &PriorityMuxer::prioritiesChanged, this, &LinearColorSmoothing::handlePriorityUpdate);

	setEnable(false);
	_timer->stop();

	Info(_log, "LinearColorSmoothing stopped");
}

void LinearColorSmoothing::updateSettings(const QJsonObject &config)
{
		setEnable(config["enable"].toBool(_enabled));
		_enabledSystemCfg = _enabled;

		auto settlingTime_ms = static_cast<int64_t>(config[SETTINGS_KEY_SETTLING_TIME].toInt(DEFAULT_SETTLINGTIME));
		const double updateFrequencyHz = config[SETTINGS_KEY_UPDATE_FREQUENCY].toDouble(DEFAULT_UPDATEFREQUENCY);
		int _updateInterval_ms = updateIntervalMsFromFrequency(updateFrequencyHz);

		SmoothingCfg cfg(false, settlingTime_ms, _updateInterval_ms);
		cfg._outputIntervalMicros = outputIntervalMicrosFromFrequency(updateFrequencyHz);

		const QString typeString = config[SETTINGS_KEY_SMOOTHING_TYPE].toString();

		if(typeString == SETTINGS_KEY_DECAY) {
			cfg._type = SmoothingType::Decay;
		}
		else {
			cfg._type = SmoothingType::Linear;
		}

		cfg._pause = false;
		cfg._outputDelay = static_cast<unsigned>(config[SETTINGS_KEY_OUTPUT_DELAY].toInt(DEFAULT_OUTPUTDEPLAY));

		cfg._interpolationRate = config[SETTINGS_KEY_INTERPOLATION_RATE].toDouble(DEFAULT_UPDATEFREQUENCY);
		cfg._dithering = config[SETTINGS_KEY_DITHERING].toBool(false);
		cfg._decay = config[SETTINGS_KEY_DECAY].toDouble(1.0);

		_cfgList[SmoothingConfigID::SYSTEM] = cfg;
		DebugIf(_enabled,_log,"%s", QSTRING_CSTR(getConfig(SmoothingConfigID::SYSTEM)));

		// if current id is 0, we need to apply the settings (forced)
		if (_currentConfigId == SmoothingConfigID::SYSTEM)
		{
			selectConfig(SmoothingConfigID::SYSTEM, true);
		}
}

void LinearColorSmoothing::handleSettingsUpdate(settings::type type, const QJsonDocument &config)
{
	if (type == settings::type::SMOOTHING)
	{
		updateSettings(config.object());
	}
}

void LinearColorSmoothing::handlePriorityUpdate(int priority)
{
	const PriorityMuxer::InputInfo priorityInfo = _prioMuxerWeak.toStrongRef()->getInputInfo(priority);
	int smooth_cfg = priorityInfo.smooth_cfg;
	if (smooth_cfg != _currentConfigId || smooth_cfg == SmoothingConfigID::EFFECT_DYNAMIC)
	{
		this->selectConfig(smooth_cfg, false);
	}
}

int LinearColorSmoothing::write(const QVector<ColorRgb> &ledValues)
{
	_targetTime = micros() + (MS_PER_MICRO * _settlingTime);
	_targetValues = ledValues;

	rememberFrame(ledValues);

	// received a new target color
	if (_previousValues.empty())
	{
		// not initialized yet
		_previousWriteTime = micros();
		_previousValues = ledValues;
		_previousInterpolationTime = micros();

		if (!_pause)
		{
			_timer->start(_updateInterval);
		}
	}

	return 0;
}

int LinearColorSmoothing::updateLedValues(const QVector<ColorRgb> &ledValues)
{
	int retval = 0;
	if (!_enabled)
	{
		retval = -1;
	}
	else
	{
		retval = write(ledValues);
	}
	return retval;
}

void LinearColorSmoothing::intitializeComponentVectors(const size_t ledCount)
{
	// (Re-)Initialize the color-vectors that store the Mean-Value
	if (_ledCount != ledCount)
	{
		_ledCount = ledCount;

		const size_t len = 3 * ledCount;

		meanValues = std::vector<floatT>(len, 0.0F);
		residualErrors = std::vector<floatT>(len, 0.0F);
		tempValues = std::vector<uint64_t>(len, 0L);
	}

	// Zero the temp vector
	std::fill(tempValues.begin(), tempValues.end(), 0L);
}

void LinearColorSmoothing::writeDirect()
{
	const int64_t now = micros();
	_previousValues = _targetValues;
	_previousWriteTime = now;

	queueColors(_previousValues);
}


void LinearColorSmoothing::writeFrame()
{
	const int64_t now = micros();
	_previousWriteTime = now;
	queueColors(_previousValues);
}


ALWAYS_INLINE int64_t LinearColorSmoothing::micros()
{
	const auto now = std::chrono::high_resolution_clock::now();
	return (std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch())).count();
}

void LinearColorSmoothing::assembleAndDitherFrame()
{
	if (meanValues.empty())
	{
		return;
	}

	// The number of LEDs present in each frame
	const size_t N = _targetValues.size();

	for (size_t i = 0; i < N; ++i)
	{
		// Add residuals for error diffusion (temporal dithering)
		const floatT fr = meanValues[3 * i + 0] + residualErrors[3 * i + 0];
		const floatT fg = meanValues[3 * i + 1] + residualErrors[3 * i + 1];
		const floatT fb = meanValues[3 * i + 2] + residualErrors[3 * i + 2];

		// Convert to to 8-bit value
		const long ir = clampRounded(fr);
		const long ig = clampRounded(fg);
		const long ib = clampRounded(fb);

		// Update the colors
		ColorRgb &prev = _previousValues[i];
		prev.red = static_cast<uint8_t>(ir);
		prev.green = static_cast<uint8_t>(ig);
		prev.blue = static_cast<uint8_t>(ib);

		// Determine the component errors
		residualErrors[3 * i + 0] = fr - static_cast<floatT>(ir);
		residualErrors[3 * i + 1] = fg - static_cast<floatT>(ig);
		residualErrors[3 * i + 2] = fb - static_cast<floatT>(ib);
	}
}

void LinearColorSmoothing::assembleFrame()
{
	if (meanValues.empty())
	{
		return;
	}

	// The number of LEDs present in each frame
	const size_t N = _targetValues.size();

	for (size_t i = 0; i < N; ++i)
	{
		// Convert to to 8-bit value
		const long ir = clampRounded(meanValues[3 * i + 0]);
		const long ig = clampRounded(meanValues[3 * i + 1]);
		const long ib = clampRounded(meanValues[3 * i + 2]);

		// Update the colors
		ColorRgb &prev = _previousValues[i];
		prev.red = static_cast<uint8_t>(ir);
		prev.green = static_cast<uint8_t>(ig);
		prev.blue = static_cast<uint8_t>(ib);
	}
}

ALWAYS_INLINE void LinearColorSmoothing::aggregateComponents(const QVector<ColorRgb>& colors, std::vector<uint64_t>& weighted, const floatT weight) {
	// Determine the integer-scale by converting the weight to fixed point
	const auto scale = static_cast<uint64_t>((static_cast<uint64_t>(1L)<<FPShift) * static_cast<double>(weight));

	const size_t N = colors.size();

	for (size_t i = 0; i < N; ++i)
	{
		const ColorRgb &color = colors[i];

		// Scale the colors
		const uint64_t red = scale * color.red;
		const uint64_t green = scale * color.green;
		const uint64_t blue = scale * color.blue;

		// Accumulate in the vector
		weighted[3 * i + 0] += red;
		weighted[3 * i + 1] += green;
		weighted[3 * i + 2] += blue;
	}
}

void LinearColorSmoothing::interpolateFrame()
{
	const int64_t now = micros();

	// The number of leds present in each frame
	const size_t N = _targetValues.size();

	intitializeComponentVectors(N);

	/// Time where the frame has been shown
	int64_t frameStart;

	/// Time where the frame display would have ended
	int64_t frameEnd = now;

	/// Time where the current window has started
	const int64_t windowStart = now - (MS_PER_MICRO * _settlingTime);

	/// The total weight of the frames that were included in our window; sum of the individual weights
	floatT fs = 0.0F;

	// To calculate the mean component we iterate over all relevant frames;
	// from the most recent to the oldest frame that still clips our moving-average window given by time (now)
	for (auto it = _frameQueue.rbegin(); it != _frameQueue.rend() && frameEnd > windowStart; ++it)
	{
		// Starting time of a frame in the window is clipped to the window start
		frameStart = std::max(windowStart, it->time);

		// Weight the current frame relative to the overall window based on start and end times
		const floatT weight = _weightFrame(frameStart, frameEnd, windowStart);
		fs += weight;

		// Aggregate the RGB components of this frame's LED colors using the individual weighting
		aggregateComponents(it->colors, tempValues, weight);

		// The previous (earlier) frame display has ended when the current frame stared to show,
		// so we can use this as the frame-end time for next iteration
		frameEnd = frameStart;
	}

	/// The inverse scaling factor for the color components, clamped to (0, 1.0]; 1.0 for fs < 1, 1 : fs otherwise
	const floatT inv_fs = ((fs < static_cast<floatT>(1.0)) ? static_cast<floatT>(1.0) : static_cast<floatT>(1.0) / fs) / (1 << SmallShiftBis);

	// Normalize the mean component values for the window (fs)
	for (size_t i = 0; i < 3 * N; ++i)
	{
		meanValues[i] = static_cast<floatT>(tempValues[i] >> FPShiftSmall) * inv_fs;
	}

	_previousInterpolationTime = now;
}

void LinearColorSmoothing::performDecay(const int64_t now) {
	/// The target time when next frame interpolation should be performed
	const int64_t interpolationTarget = _previousInterpolationTime + _interpolationIntervalMicros;

	/// The target time when next write operation should be performed
	const int64_t writeTarget = _previousWriteTime + _outputIntervalMicros;

	/// Whether a frame interpolation is pending
	const bool interpolatePending = now > interpolationTarget;

	/// Whether a write is pending
	const bool writePending = now > writeTarget;

	// Check whether a new interpolation frame is due
	if (interpolatePending)
	{
		interpolateFrame();
		++_interpolationCounter;

		// Assemble the frame now when no dithering is applied
		if(!_dithering) {
			assembleFrame();
		}
	}

	// Check whether to frame output is due
	if (writePending)
	{
		// Dither the frame to diffuse rounding errors
		if(_dithering) {
			assembleAndDitherFrame();
		}

		writeFrame();
		++_renderedCounter;
	}

	// Check for sleep when no operation is pending.
	// As our QTimer is not capable of sub 1ms timing but instead performs spinning -
	// we have to do µsec-sleep to free CPU time; otherwise the thread would consume 100% CPU time.
	if(_updateInterval <= 0 && !(interpolatePending || writePending)) {
		const int64_t nextActionExpected = std::min(interpolationTarget, writeTarget);
		sleepUntilNextAction(now, nextActionExpected);
	}

	// Write stats every 30 sec
	if ((now > (_renderedStatTime + 30 * 1000000)) && (_renderedCounter > _renderedStatCounter))
	{
		Debug(_log, "decay - rendered frames [%d] (%f/s), interpolated frames [%d] (%f/s) in [%f ms]"
			  , _renderedCounter - _renderedStatCounter
			  , (static_cast<floatT>(1.0) * (_renderedCounter - _renderedStatCounter) / ((now - _renderedStatTime) / 1000000.0F))
			  , _interpolationCounter - _interpolationStatCounter
			  , (static_cast<floatT>(1.0) * (_interpolationCounter - _interpolationStatCounter) / ((now - _renderedStatTime) / 1000000.0F))
			  , (now - _renderedStatTime) / static_cast<floatT>(1000.0)
			  );
		_renderedStatTime = now;
		_renderedStatCounter = _renderedCounter;
		_interpolationStatCounter = _interpolationCounter;
	}
}

void LinearColorSmoothing::performLinear(const int64_t now) {
	const int64_t writeTarget = _previousWriteTime + _outputIntervalMicros;
	const bool writePending = now > writeTarget;

	if (!writePending)
	{
		if (_updateInterval <= 0)
		{
			sleepUntilNextAction(now, writeTarget);
		}
		return;
	}

	const int64_t deltaTime = _targetTime - now;
	const floatT k = static_cast<floatT>(1.0) - static_cast<floatT>(1.0) * static_cast<floatT>(deltaTime) / static_cast<floatT>(_targetTime - _previousWriteTime);
	const size_t N = _previousValues.size();

	for (size_t i = 0; i < N; ++i)
	{
		const ColorRgb &target = _targetValues[i];
		ColorRgb &prev         = _previousValues[i];

		const int reddif   = target.red   - prev.red;
		const int greendif = target.green - prev.green;
		const int bluedif  = target.blue  - prev.blue;

		prev.red   = static_cast<uint8_t>(prev.red   + (reddif   < 0 ? -1:1) * static_cast<int>(std::ceil(k * static_cast<float>(std::abs(reddif)))));
		prev.green = static_cast<uint8_t>(prev.green + (greendif < 0 ? -1:1) * static_cast<int>(std::ceil(k * static_cast<float>(std::abs(greendif)))));
		prev.blue  = static_cast<uint8_t>(prev.blue  + (bluedif  < 0 ? -1:1) * static_cast<int>(std::ceil(k * static_cast<float>(std::abs(bluedif)))));
	}

	writeFrame();
}

void LinearColorSmoothing::updateLeds()
{
	const int64_t now = micros();
	const int64_t deltaTime = _targetTime - now;

	if (deltaTime < 0)
	{
		writeDirect();
		return;
	}

	if (_smoothingType == SmoothingType::Decay)
	{
		performDecay(now);
	}
	else
	{
		performLinear(now);
	}
}

void LinearColorSmoothing::rememberFrame(const QVector<ColorRgb> &ledColors)
{
	const int64_t now = micros();

	// Maintain the queue by removing outdated frames
	const int64_t windowStart = now - (MS_PER_MICRO * _settlingTime);

	int p = -1; // Start with -1 instead of 0, so we keep the last frame at least partially clipping the window

	// As the frames are ordered chronologically we scan from the front (oldest) till we find the first fresh frame
	for (auto it = _frameQueue.begin(); it != _frameQueue.end() && it->time < windowStart; ++it)
	{
		++p;
	}

	if (p > 0)
	{
		_frameQueue.erase(_frameQueue.begin(), _frameQueue.begin() + p);
	}

	// Append the latest frame at back of the queue
	const auto frame = REMEMBERED_FRAME(now, ledColors);
	_frameQueue.push_back(frame);
}


void LinearColorSmoothing::clearRememberedFrames()
{
	_frameQueue.clear();

	_ledCount = 0;
	meanValues.clear();
	residualErrors.clear();
	tempValues.clear();
}

void LinearColorSmoothing::queueColors(const QVector<ColorRgb> &ledColors)
{
	assert (!ledColors.empty());

	if (_outputDelay == 0)
	{
		// No output delay => immediate write
		if (!_pause)
		{
			QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
			if (hyperion)
			{
				emit hyperion->ledDeviceData(ledColors);
			}
		}
		return;
	}

	// Push new colors in the delay-buffer
	_outputQueue.push_back(ledColors);

	// If the delay-buffer is filled pop the front and write to device
	if (_outputQueue.size() <= _outputDelay)
	{
		return;
	}

	if (!_pause)
	{
		QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
		if (hyperion)
		{
			emit hyperion->ledDeviceData(_outputQueue.front());
		}
	}
	_outputQueue.pop_front();
}

void LinearColorSmoothing::clearQueuedColors()
{
	_timer->stop();
	_previousValues.clear();

	_targetValues.clear();

	clearRememberedFrames();
}

void LinearColorSmoothing::componentStateChange(hyperion::Components component, bool state)
{
	if (component == hyperion::COMP_SMOOTHING)
	{
		setEnable(state);
	}
}

void LinearColorSmoothing::setEnable(bool enable)
{
	if ( _enabled != enable)
	{
		_enabled = enable;
		if (!_enabled)
		{
			clearQueuedColors();
		}
		// update comp register
		QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
		if (hyperion)
		{
			hyperion->setNewComponentState(hyperion::COMP_SMOOTHING, enable);
		}
	}
}

void LinearColorSmoothing::setPause(bool pause)
{
	_pause = pause;
}

unsigned LinearColorSmoothing::addConfig(int settlingTime_ms, double ledUpdateFrequency_hz, unsigned updateDelay)
{
	SmoothingCfg cfg {
		false,
		settlingTime_ms,
		updateIntervalMsFromFrequency(ledUpdateFrequency_hz),
		SmoothingType::Linear,
		ledUpdateFrequency_hz,
		updateDelay
	};
	cfg._outputIntervalMicros = outputIntervalMicrosFromFrequency(ledUpdateFrequency_hz);
	_cfgList.append(std::move(cfg));
	auto cfgID = static_cast<unsigned>(_cfgList.count() - 1);

	qCDebug(smoothing) << "Added new smoothing config id" << cfgID << ":" << getConfig(cfgID);

	return cfgID;
}

unsigned LinearColorSmoothing::updateConfig(int cfgID, int settlingTime_ms, double ledUpdateFrequency_hz, unsigned updateDelay)
{
	int updatedCfgID = cfgID;
	if (cfgID < _cfgList.count())
	{
		SmoothingCfg cfg {
			false,
			settlingTime_ms,
			updateIntervalMsFromFrequency(ledUpdateFrequency_hz),
			SmoothingType::Linear,
			ledUpdateFrequency_hz,
			updateDelay
		};
		cfg._outputIntervalMicros = outputIntervalMicrosFromFrequency(ledUpdateFrequency_hz);
		_cfgList[updatedCfgID] = cfg;
		Debug(_log,"%s", QSTRING_CSTR(getConfig(updatedCfgID)));
	}
	else
	{
		updatedCfgID = addConfig(settlingTime_ms, ledUpdateFrequency_hz, updateDelay);
	}
	return updatedCfgID;
}

bool LinearColorSmoothing::selectConfig(int cfgID, bool force)
{
	if (_currentConfigId == cfgID && !force)
	{
		return true;
	}

	if (cfgID >= _cfgList.count() )
	{
		// reset to default
		_currentConfigId = SmoothingConfigID::SYSTEM;
		return false;
	}

	_smoothingType = _cfgList[cfgID]._type;
	_settlingTime = _cfgList[cfgID]._settlingTime;
	_outputDelay = _cfgList[cfgID]._outputDelay;
	_pause = _cfgList[cfgID]._pause;
	_outputIntervalMicros = _cfgList[cfgID]._outputIntervalMicros;
	_interpolationRate = _cfgList[cfgID]._interpolationRate;
	_interpolationIntervalMicros = int64_t(MICROS_PER_SECOND / _interpolationRate);
	_dithering = _cfgList[cfgID]._dithering;
	_decay = static_cast<floatT>(_cfgList[cfgID]._decay);
	_invWindow = static_cast<floatT>(1.0) / (MS_PER_MICRO * static_cast<floatT>(_settlingTime));

	// Set _weightFrame based on the given decay
	const floatT decay = _decay;
	const floatT inv_window = _invWindow;

	// For decay != 1 use power-based approach for calculating the moving average values
	if(std::abs(decay - static_cast<floatT>(1.0)) > std::numeric_limits<floatT>::epsilon()) {
		// Exponential Decay
		_weightFrame = [inv_window,decay](const int64_t fs, const int64_t fe, const int64_t ws) {
			const floatT s = static_cast<floatT>(fs - ws) * inv_window;
			const floatT t = static_cast<floatT>(fe - ws) * inv_window;

			return (decay + floatT(1)) * (std::pow(t, decay) - std::pow(s, decay));
		};
	} 
	else 
	{
		// For decay == 1 use linear interpolation of the moving average values
		// Linear Decay
		_weightFrame = [inv_window](const int64_t fs, const int64_t fe, const int64_t /*ws*/) {
			// Linear weighting = (end - start) * scale
			return (static_cast<floatT>(fe) - static_cast<floatT>(fs)) * inv_window;
		};
	}

	_renderedStatTime = micros();
	_renderedCounter = 0;
	_renderedStatCounter = 0;
	_interpolationCounter = 0;
	_interpolationStatCounter = 0;

	//Enable smoothing for effects with smoothing
	if (cfgID >= SmoothingConfigID::EFFECT_DYNAMIC)
	{
		Debug(_log,"Run Effect with Smoothing enabled");
		_enabledSystemCfg = _enabled;
		setEnable(true);
	}
	else
	{
		// Restore enabled state after running an effect with smoothing
		setEnable(_enabledSystemCfg);
	}

	if (_cfgList[cfgID]._updateInterval != _updateInterval)
	{
		_timer->stop();
		_updateInterval = _cfgList[cfgID]._updateInterval;
		if (this->enabled() && !_pause && _targetValues.empty())
		{
			_timer->start(_updateInterval);
		}
	}
	_currentConfigId = cfgID;
	DebugIf(_enabled, _log,"%s", QSTRING_CSTR(getConfig(_currentConfigId)));

	return true;
}

QString LinearColorSmoothing::getConfig(int cfgID)
{
	QString configText;

	if (cfgID < _cfgList.count())
	{
		SmoothingCfg cfg = _cfgList[cfgID];

		configText = QString ("[%1] - Type: %2, Pause: %3")
					 .arg(cfgID)
					 .arg(SmoothingCfg::EnumToString(cfg._type),(cfg._pause) ? "true" : "false") ;

		switch (cfg._type) {
		case SmoothingType::Decay:
		{
			const double thalf = (1.0-std::pow(1.0/2, 1.0/_decay))*static_cast<double>(_settlingTime);
			configText += QString (", Interpolation rate: %1Hz, Dithering: %2, decay: %3 -> Halftime: %4ms")
						  .arg(cfg._interpolationRate,0,'f',2)
						  .arg((cfg._dithering) ? "true" : "false")
						  .arg(cfg._decay,0,'f',2)
						  .arg(thalf,0,'f',2);
			[[fallthrough]];
		}

		case SmoothingType::Linear:
		{
			const double updateIntervalMs = static_cast<double>(cfg._outputIntervalMicros) / MS_PER_MICRO;
			const double updateFrequencyHz = static_cast<double>(MICROS_PER_SECOND) / static_cast<double>(cfg._outputIntervalMicros);
			configText += QString (", Settling time: %1ms, Interval: %2ms (%3Hz)")
						  .arg(cfg._settlingTime)
					  .arg(updateIntervalMs, 0, 'f', 2)
					  .arg(updateFrequencyHz, 0, 'f', 2);
			break;
		}
		}

		configText += QString (", delay: %1 frames")
					  .arg(cfg._outputDelay);
	}

	return configText;
}

LinearColorSmoothing::SmoothingCfg::SmoothingCfg() :
	_pause(false),
	_settlingTime(DEFAULT_SETTLINGTIME),
	_updateInterval(DEFAULT_UPDATEINTERVALL.count()),
	_outputIntervalMicros(DEFAULT_UPDATEINTERVALL.count() * MS_PER_MICRO),
	_type(SmoothingType::Linear)
{
}

LinearColorSmoothing::SmoothingCfg::SmoothingCfg(bool pause, int64_t settlingTime, int updateInterval, SmoothingType type, double interpolationRate, unsigned outputDelay, bool dithering, double decay) :
	_pause(pause),
	_settlingTime(settlingTime),
	_updateInterval(updateInterval),
	_outputIntervalMicros(updateInterval * MS_PER_MICRO),
	_type(type),
	_interpolationRate(interpolationRate),
	_outputDelay(outputDelay),
	_dithering(dithering),
	_decay(decay)
{
}

QString LinearColorSmoothing::SmoothingCfg::EnumToString(SmoothingType type)
{
	if (type == SmoothingType::Linear) {
		return QString("Linear");
	}

	if (type == SmoothingType::Decay)
	{
		return QString("Decay");
	}

	return QString("Unknown");
}
