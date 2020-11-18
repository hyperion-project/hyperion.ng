// Qt includes
#include <QDateTime>
#include <QTimer>

#include "LinearColorSmoothing.h"
#include <hyperion/Hyperion.h>

#include <cmath>
#include <chrono>
#include <thread>

/// The number of microseconds per millisecond = 1000.
const int64_t MS_PER_MICRO = 1000;

#if defined(COMPILER_GCC)
#define ALWAYS_INLINE inline __attribute__((__always_inline__))
#elif defined(COMPILER_MSVC)
#define ALWAYS_INLINE __forceinline
#else
#define ALWAYS_INLINE inline
#endif

/// Clamps the rounded values to the byte-interval of [0, 255].
ALWAYS_INLINE long clampRounded(const floatT x) {
	return std::min(255L, std::max(0L, std::lroundf(x)));
}

/// The number of bits that are used for shifting the fixed point values
const int FPShift = (sizeof(uint64_t)*8 - (12 + 9));

/// The number of bits that are reduce the shifting when converting from fixed to floating point. 8 bits = 256 values
const int SmallShiftBis = sizeof(uint8_t)*8;

/// The number of bits that are used for shifting the fixed point values plus SmallShiftBis
const int FPShiftSmall = (sizeof(uint64_t)*8 - (12 + 9 + SmallShiftBis));

const char* SETTINGS_KEY_SMOOTHING_TYPE = "type";
const char* SETTINGS_KEY_INTERPOLATION_RATE = "interpolationRate";
const char* SETTINGS_KEY_OUTPUT_RATE = "outputRate";
const char* SETTINGS_KEY_DITHERING = "dithering";
const char* SETTINGS_KEY_DECAY = "decay";

using namespace hyperion;

const int64_t DEFAUL_SETTLINGTIME = 200;													// settlingtime in ms
const int DEFAUL_UPDATEFREQUENCY = 25;													// updatefrequncy in hz

constexpr std::chrono::milliseconds DEFAUL_UPDATEINTERVALL{1000/ DEFAUL_UPDATEFREQUENCY};
const unsigned DEFAUL_OUTPUTDEPLAY = 0;														// outputdelay in ms

LinearColorSmoothing::LinearColorSmoothing(const QJsonDocument &config, Hyperion *hyperion)
	: QObject(hyperion)
	, _log(Logger::getInstance("SMOOTHING"))
	, _hyperion(hyperion)
	, _updateInterval(DEFAUL_UPDATEINTERVALL.count())
	, _settlingTime(DEFAUL_SETTLINGTIME)
	, _timer(new QTimer(this))
	, _outputDelay(DEFAUL_OUTPUTDEPLAY)
	, _smoothingType(SmoothingType::Linear)
	, _writeToLedsEnable(false)
	, _continuousOutput(false)
	, _pause(false)
	, _currentConfigId(0)
	, _enabled(false)
	, tempValues(std::vector<uint64_t>(0, 0L))
{
	// init cfg 0 (default)
	addConfig(DEFAUL_SETTLINGTIME, DEFAUL_UPDATEFREQUENCY, DEFAUL_OUTPUTDEPLAY);
	handleSettingsUpdate(settings::SMOOTHING, config);
	selectConfig(0, true);

	// add pause on cfg 1
	SMOOTHING_CFG cfg = {SmoothingType::Linear, false, 0, 0, 0, 0, 0, false, 1};
	_cfgList.append(cfg);

	// listen for comp changes
	connect(_hyperion, &Hyperion::compStateChangeRequest, this, &LinearColorSmoothing::componentStateChange);
	// timer
	connect(_timer, &QTimer::timeout, this, &LinearColorSmoothing::updateLeds);

	//Debug(_log, "LinearColorSmoothing sizeof floatT == %d", (sizeof(floatT)));
}

void LinearColorSmoothing::handleSettingsUpdate(settings::type type, const QJsonDocument &config)
{
	if (type == settings::SMOOTHING)
	{
		//	std::cout << "LinearColorSmoothing::handleSettingsUpdate" << std::endl;
		//	std::cout << config.toJson().toStdString() << std::endl;

		QJsonObject obj = config.object();
		if (enabled() != obj["enable"].toBool(true))
		{
			setEnable(obj["enable"].toBool(true));
		}

		_continuousOutput = obj["continuousOutput"].toBool(true);

		SMOOTHING_CFG cfg = {SmoothingType::Linear,true, 0, 0, 0, 0, 0, false, 1};

		const QString typeString = obj[SETTINGS_KEY_SMOOTHING_TYPE].toString();

		if(typeString == "linear") {
			cfg.smoothingType = SmoothingType::Linear;
		} else if(typeString == "decay") {
			cfg.smoothingType = SmoothingType::Decay;
		}

		cfg.pause = false;
		cfg.settlingTime = static_cast<int64_t>(obj["time_ms"].toInt(DEFAUL_SETTLINGTIME));
		cfg.updateInterval = static_cast<int>(1000.0 / obj["updateFrequency"].toDouble(DEFAUL_UPDATEFREQUENCY));
		cfg.outputRate = obj[SETTINGS_KEY_OUTPUT_RATE].toDouble(DEFAUL_UPDATEFREQUENCY);
		cfg.interpolationRate = obj[SETTINGS_KEY_INTERPOLATION_RATE].toDouble(DEFAUL_UPDATEFREQUENCY);
		cfg.outputDelay = static_cast<unsigned>(obj["updateDelay"].toInt(DEFAUL_OUTPUTDEPLAY));
		cfg.dithering = obj[SETTINGS_KEY_DITHERING].toBool(false);
		cfg.decay = obj[SETTINGS_KEY_DECAY].toDouble(1.0);

		//Debug( _log, "smoothing cfg_id %d: pause: %d bool, settlingTime: %d ms, interval: %d ms (%u Hz), updateDelay: %u frames",  _currentConfigId, cfg.pause, cfg.settlingTime, cfg.updateInterval, unsigned(1000.0/cfg.updateInterval), cfg.outputDelay );
		_cfgList[0] = cfg;

		// if current id is 0, we need to apply the settings (forced)
		if (_currentConfigId == 0)
		{
			//Debug( _log, "_currentConfigId == 0");
			selectConfig(0, true);
		}
		else
		{
			//Debug( _log, "_currentConfigId != 0");
		}
	}
}

int LinearColorSmoothing::write(const std::vector<ColorRgb> &ledValues)
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

		//Debug( _log, "Start Smoothing timer: settlingTime: %d ms, interval: %d ms (%u Hz), updateDelay: %u frames", _settlingTime, _updateInterval, unsigned(1000.0/_updateInterval), _outputDelay );
		QMetaObject::invokeMethod(_timer, "start", Qt::QueuedConnection, Q_ARG(int, _updateInterval));
	}

	return 0;
}

int LinearColorSmoothing::updateLedValues(const std::vector<ColorRgb> &ledValues)
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
	_writeToLedsEnable = _continuousOutput;
}


void LinearColorSmoothing::writeFrame()
{
	const int64_t now = micros();
	_previousWriteTime = now;
	queueColors(_previousValues);
	_writeToLedsEnable = _continuousOutput;
}


ALWAYS_INLINE int64_t LinearColorSmoothing::micros() const
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

	// The number of leds present in each frame
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
		residualErrors[3 * i + 0] = fr - ir;
		residualErrors[3 * i + 1] = fg - ig;
		residualErrors[3 * i + 2] = fb - ib;
	}
}

void LinearColorSmoothing::assembleFrame()
{
	if (meanValues.empty())
	{
		return;
	}

	// The number of leds present in each frame
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

ALWAYS_INLINE void LinearColorSmoothing::aggregateComponents(const std::vector<ColorRgb>& colors, std::vector<uint64_t>& weighted, const floatT weight) {
	// Determine the integer-scale by converting the weight to fixed point
	const uint64_t scale = (static_cast<uint64_t>(1L)<<FPShift) * static_cast<double>(weight);

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
	const floatT inv_fs = ((fs < 1.0F) ? 1.0F : 1.0F / fs) / (1 << SmallShiftBis);

	// Normalize the mean component values for the window (fs)
	for (size_t i = 0; i < 3 * N; ++i)
	{
		meanValues[i] = (tempValues[i] >> FPShiftSmall) * inv_fs;
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
		const int64_t microsTillNextAction = nextActionExpected - now;
		const int64_t SLEEP_MAX_MICROS = 1000L; // We want to use usleep for up to 1ms
		const int64_t SLEEP_RES_MICROS = 100L; // Expected resolution is >= 100µs on stock linux

		if(microsTillNextAction > SLEEP_RES_MICROS) {
			const int64_t wait = std::min(microsTillNextAction - SLEEP_RES_MICROS, SLEEP_MAX_MICROS);
			//usleep(wait);
			std::this_thread::sleep_for(std::chrono::microseconds(wait));
		}
	}

	// Write stats every 30 sec
	if ((now > (_renderedStatTime + 30 * 1000000)) && (_renderedCounter > _renderedStatCounter))
	{
		Debug(_log, "decay - rendered frames [%d] (%f/s), interpolated frames [%d] (%f/s) in [%f ms]"
		, _renderedCounter - _renderedStatCounter
		, (1.0F * (_renderedCounter - _renderedStatCounter) / ((now - _renderedStatTime) / 1000000.0F))
		, _interpolationCounter - _interpolationStatCounter
		, (1.0F * (_interpolationCounter - _interpolationStatCounter) / ((now - _renderedStatTime) / 1000000.0F))
		, (now - _renderedStatTime) / 1000.0F
		);
		_renderedStatTime = now;
		_renderedStatCounter = _renderedCounter;
		_interpolationStatCounter = _interpolationCounter;
	}
}

void LinearColorSmoothing::performLinear(const int64_t now) {
	const int64_t deltaTime = _targetTime - now;
	const float k = 1.0F - 1.0F * deltaTime / (_targetTime - _previousWriteTime);
	const size_t N = _previousValues.size();

	for (size_t i = 0; i < N; ++i)
	{
		const ColorRgb &target = _targetValues[i];
		ColorRgb &prev         = _previousValues[i];

		const int reddif   = target.red   - prev.red;
		const int greendif = target.green - prev.green;
		const int bluedif  = target.blue  - prev.blue;

		prev.red   += (reddif   < 0 ? -1:1) * std::ceil(k * std::abs(reddif));
		prev.green += (greendif < 0 ? -1:1) * std::ceil(k * std::abs(greendif));
		prev.blue  += (bluedif  < 0 ? -1:1) * std::ceil(k * std::abs(bluedif));
	}

	writeFrame();
}

void LinearColorSmoothing::updateLeds()
{
	const int64_t now = micros();
	const int64_t deltaTime = _targetTime - now;

	//Debug(_log, "elapsed Time [%d], _targetTime [%d] - now [%d], deltaTime [%d]", now -_previousWriteTime, _targetTime, now, deltaTime);
	if (deltaTime < 0)
	{
		writeDirect();
		return;
	}

	switch (_smoothingType)
	{
	case Decay:
		performDecay(now);
		break;

	case Linear:
		// Linear interpolation is default
	default:
		performLinear(now);
		break;
	}
}

void LinearColorSmoothing::rememberFrame(const std::vector<ColorRgb> &ledColors)
{
	//Debug(_log, "rememberFrame -  before _frameQueue.size() [%d]", _frameQueue.size());

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
		//Debug(_log, "rememberFrame -  erasing %d frames", p);
		_frameQueue.erase(_frameQueue.begin(), _frameQueue.begin() + p);
	}

	// Append the latest frame at back of the queue
	const REMEMBERED_FRAME frame = REMEMBERED_FRAME(now, ledColors);
	_frameQueue.push_back(frame);

	//Debug(_log, "rememberFrame -  after _frameQueue.size() [%d]", _frameQueue.size());
}


void LinearColorSmoothing::clearRememberedFrames()
{
	_frameQueue.clear();

	_ledCount = 0;
	meanValues.clear();
	residualErrors.clear();
	tempValues.clear();
}

void LinearColorSmoothing::queueColors(const std::vector<ColorRgb> &ledColors)
{
	//Debug(_log, "queueColors -  _outputDelay[%d] _outputQueue.size() [%d], _writeToLedsEnable[%d]", _outputDelay, _outputQueue.size(), _writeToLedsEnable);
	if (_outputDelay == 0)
	{
		// No output delay => immediate write
		if (_writeToLedsEnable && !_pause)
		{
			//			if ( ledColors.size() == 0 )
			//				qFatal ("No LedValues! - in LinearColorSmoothing::queueColors() - _outputDelay == 0");
			//			else
			emit _hyperion->ledDeviceData(ledColors);
		}
	}
	else
	{
		// Push new colors in the delay-buffer
		if (_writeToLedsEnable)
		{
			_outputQueue.push_back(ledColors);
		}

		// If the delay-buffer is filled pop the front and write to device
		if (!_outputQueue.empty())
		{
			if (_outputQueue.size() > _outputDelay || !_writeToLedsEnable)
			{
				if (!_pause)
				{
					emit _hyperion->ledDeviceData(_outputQueue.front());
				}
				_outputQueue.pop_front();
			}
		}
	}
}

void LinearColorSmoothing::clearQueuedColors()
{
	QMetaObject::invokeMethod(_timer, "stop", Qt::QueuedConnection);
	_previousValues.clear();

	_targetValues.clear();

	clearRememberedFrames();
}

void LinearColorSmoothing::componentStateChange(hyperion::Components component, bool state)
{
	_writeToLedsEnable = state;
	if (component == hyperion::COMP_LEDDEVICE)
	{
		clearQueuedColors();
	}

	if (component == hyperion::COMP_SMOOTHING)
	{
		setEnable(state);
	}
}

void LinearColorSmoothing::setEnable(bool enable)
{
	_enabled = enable;
	if (!_enabled)
	{
		clearQueuedColors();
	}
	// update comp register
	_hyperion->setNewComponentState(hyperion::COMP_SMOOTHING, enable);
}

void LinearColorSmoothing::setPause(bool pause)
{
	_pause = pause;
}

unsigned LinearColorSmoothing::addConfig(int settlingTime_ms, double ledUpdateFrequency_hz, unsigned updateDelay)
{
	SMOOTHING_CFG cfg = {
		SmoothingType::Linear,
		false,
		settlingTime_ms,
		static_cast<int>(1000.0 / ledUpdateFrequency_hz),
		ledUpdateFrequency_hz,
		ledUpdateFrequency_hz,
		updateDelay,
		false,
		1
	};
	_cfgList.append(cfg);

	//Debug( _log, "smoothing cfg %d: pause: %d bool, settlingTime: %d ms, interval: %d ms (%u Hz), updateDelay: %u frames",  _cfgList.count()-1, cfg.pause, cfg.settlingTime, cfg.updateInterval, unsigned(1000.0/cfg.updateInterval), cfg.outputDelay );
	return _cfgList.count() - 1;
}

unsigned LinearColorSmoothing::updateConfig(unsigned cfgID, int settlingTime_ms, double ledUpdateFrequency_hz, unsigned updateDelay)
{
	unsigned updatedCfgID = cfgID;
	if (cfgID < static_cast<unsigned>(_cfgList.count()))
	{
		SMOOTHING_CFG cfg = {
			SmoothingType::Linear,
			false,
			settlingTime_ms,
			static_cast<int>(1000.0 / ledUpdateFrequency_hz),
			ledUpdateFrequency_hz,
			ledUpdateFrequency_hz,
			updateDelay,
			false,
			1};
		_cfgList[updatedCfgID] = cfg;
	}
	else
	{
		updatedCfgID = addConfig(settlingTime_ms, ledUpdateFrequency_hz, updateDelay);
	}
	//	Debug( _log, "smoothing updatedCfgID %u: settlingTime: %d ms, "
	//				 "interval: %d ms (%u Hz), updateDelay: %u frames",  cfgID, _settlingTime, int64_t(1000.0/ledUpdateFrequency_hz), unsigned(ledUpdateFrequency_hz), updateDelay );
	return updatedCfgID;
}

bool LinearColorSmoothing::selectConfig(unsigned cfg, bool force)
{
	if (_currentConfigId == cfg && !force)
	{
		//Debug( _log, "selectConfig SAME as before, not FORCED - _currentConfigId [%u], force [%d]", cfg, force);
		//Debug( _log, "current smoothing cfg: %d, settlingTime: %d ms, interval: %d ms (%u Hz), updateDelay: %u frames",  _currentConfigId, _settlingTime, _updateInterval, unsigned(1000.0/_updateInterval), _outputDelay );
		return true;
	}

	//Debug( _log, "selectConfig FORCED - _currentConfigId [%u], force [%d]", cfg, force);
	if (cfg < static_cast<uint>(_cfgList.count()) )
	{
		_smoothingType = _cfgList[cfg].smoothingType;
		_settlingTime = _cfgList[cfg].settlingTime;
		_outputDelay = _cfgList[cfg].outputDelay;
		_pause = _cfgList[cfg].pause;
		_outputRate = _cfgList[cfg].outputRate;
		_outputIntervalMicros = int64_t(1000000.0 / _outputRate); // 1s = 1e6 µs
		_interpolationRate = _cfgList[cfg].interpolationRate;
		_interpolationIntervalMicros = int64_t(1000000.0 / _interpolationRate);
		_dithering = _cfgList[cfg].dithering;
		_decay = _cfgList[cfg].decay;
		_invWindow = 1.0F / (MS_PER_MICRO * _settlingTime);

		// Set _weightFrame based on the given decay
		const float decay = _decay;
		const floatT inv_window = _invWindow;

		// For decay != 1 use power-based approach for calculating the moving average values
		if(std::abs(decay - 1.0F) > std::numeric_limits<float>::epsilon()) {
			// Exponential Decay
			_weightFrame = [inv_window,decay](const int64_t fs, const int64_t fe, const int64_t ws) {
				const floatT s = (fs - ws) * inv_window;
				const floatT t = (fe - ws) * inv_window;

				return (decay + 1) * (std::pow(t, decay) - std::pow(s, decay));
			};
		} else {
			// For decay == 1 use linear interpolation of the moving average values
			// Linear Decay
			_weightFrame = [inv_window](const int64_t fs, const int64_t fe, const int64_t /*ws*/) {
				// Linear weighting = (end - start) * scale
				return static_cast<floatT>((fe - fs) * inv_window);
			};
		}

		_renderedStatTime = micros();
		_renderedCounter = 0;
		_renderedStatCounter = 0;
		_interpolationCounter = 0;
		_interpolationStatCounter = 0;

		if (_cfgList[cfg].updateInterval != _updateInterval)
		{

			QMetaObject::invokeMethod(_timer, "stop", Qt::QueuedConnection);
			_updateInterval = _cfgList[cfg].updateInterval;
			if (this->enabled() && this->_writeToLedsEnable)
			{
				//Debug( _log, "_cfgList[cfg].updateInterval != _updateInterval - Restart timer - _updateInterval [%d]", _updateInterval);
				QMetaObject::invokeMethod(_timer, "start", Qt::QueuedConnection, Q_ARG(int, _updateInterval));
			}
			else
			{
				//Debug( _log, "Smoothing disabled, do NOT restart timer");
			}
		}
		_currentConfigId = cfg;
		// Debug( _log, "current smoothing cfg: %d, settlingTime: %d ms, interval: %d ms (%u Hz), updateDelay: %u frames",  _currentConfigId, _settlingTime, _updateInterval, unsigned(1000.0/_updateInterval), _outputDelay );
		//	DebugIf( enabled() && !_pause, _log, "set smoothing cfg: %u settlingTime: %d ms, interval: %d ms,  updateDelay: %u frames",  _currentConfigId, _settlingTime, _updateInterval,  _outputDelay );
		// DebugIf( _pause, _log, "set smoothing cfg: %d, pause",  _currentConfigId );

		const float thalf = (1.0-std::pow(1.0/2, 1.0/_decay))*_settlingTime;
		Debug( _log, "cfg [%d]:  Type: %s - Time: %d ms, outputRate %f Hz, interpolationRate: %f Hz, timer: %d ms, Dithering: %d, Decay: %f -> HalfTime: %f ms", cfg, _smoothingType == SmoothingType::Decay ? "decay" : "linear", _settlingTime, _outputRate, _interpolationRate, _updateInterval, _dithering ? 1 : 0, _decay, thalf);

		return true;
	}

	// reset to default
	_currentConfigId = 0;
	return false;
}
