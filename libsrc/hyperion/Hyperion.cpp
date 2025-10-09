// STL includes
#include<algorithm>

// QT includes
#include <QString>
#include <QStringList>
#include <QThread>
#include <QVariantMap>

// hyperion include
#include <hyperion/Hyperion.h>

#include <hyperion/ImageProcessor.h>
#include <hyperion/ColorAdjustment.h>

// utils
#include <utils/hyperion.h>
#include <utils/GlobalSignals.h>
#include <utils/Logger.h>
#include <utils/JsonUtils.h>
#include "utils/WaitTime.h"
#include "utils/TrackedMemory.h"

// LedDevice includes
#include <leddevice/LedDeviceWrapper.h>

#include <hyperion/MultiColorAdjustment.h>
#include <hyperion/LinearColorSmoothing.h>

#if defined(ENABLE_EFFECTENGINE)
// effect engine includes
#include <effectengine/EffectEngine.h>
#endif

// settingsManagaer
#include <hyperion/SettingsManager.h>

// BGEffectHandler
#include <hyperion/BGEffectHandler.h>

// CaptureControl (Daemon capture)
#include <hyperion/CaptureCont.h>

// Boblight
#if defined(ENABLE_BOBLIGHT_SERVER)
#include <boblightserver/BoblightServer.h>
#endif

// Constants
namespace {
	constexpr std::chrono::milliseconds DEFAULT_MAX_IMAGE_EMISSION_INTERVAL{ 40 }; // 25 Hz
	constexpr std::chrono::milliseconds DEFAULT_MAX_RAW_LED_DATA_EMISSION_INTERVAL{ 25 }; // 40 Hz
	constexpr std::chrono::milliseconds DEFAULT_MAX_LED_DEVICE_DATA_EMISSION_INTERVAL{ 5 }; // 200 Hz
} //End of constants

Hyperion::Hyperion(quint8 instance, QObject* parent)
	: QObject(parent)
	, _instIndex(instance)
	, _settingsManager(nullptr)
	, _componentRegister(nullptr)
	, _imageProcessor(nullptr)
	, _muxer(nullptr)
	, _raw2ledAdjustment(nullptr)
	, _ledDeviceWrapper(nullptr)
	, _deviceSmooth(nullptr)
#if defined(ENABLE_EFFECTENGINE)
	, _effectEngine(nullptr)
#endif
	, _log(nullptr)
	, _hwLedCount(0)
	, _layoutLedCount(0)
	, _colorOrder("rgb")
	, _BGEffectHandler(nullptr)
	, _captureCont(nullptr)
#if defined(ENABLE_BOBLIGHT_SERVER)
	, _boblightServer(nullptr)
#endif
	, _lastImageEmission(0)
	, _lastRawLedDataEmission(0)
	, _lastLedDeviceDataEmission(0)
	, _imageEmissionInterval(DEFAULT_MAX_IMAGE_EMISSION_INTERVAL)
	, _rawLedDataEmissionInterval(DEFAULT_MAX_RAW_LED_DATA_EMISSION_INTERVAL)
	, _ledDeviceDataEmissionInterval(DEFAULT_MAX_LED_DEVICE_DATA_EMISSION_INTERVAL)
{
	qRegisterMetaType<ComponentList>("ComponentList");
	qRegisterMetaType<Image<ColorRgb>>("ColorRgbImage");

	QString const subComponent = "I"+QString::number(_instIndex);
	this->setProperty("instance", QVariant::fromValue(subComponent));

	_log= Logger::getInstance("HYPERION", subComponent);
}

void Hyperion::start()
{
	Debug(_log, "Hyperion instance starting...");

	_settingsManager.reset(new SettingsManager(_instIndex, this));

	// link settings changed with the current Hyperion instance
	connect(_settingsManager.get(), &SettingsManager::settingsChanged, this, &Hyperion::settingsChanged);
	// listen for settings updates of this instance (LEDS & COLOR)
	connect(_settingsManager.get(), &SettingsManager::settingsChanged, this, &Hyperion::handleSettingsUpdate);

	_componentRegister = MAKE_TRACKED_SHARED(ComponentRegister, this);

	// get newVideoMode from HyperionIManager
	connect(this, &Hyperion::newVideoMode, this, &Hyperion::handleNewVideoMode);

	// handle hwLedCount
	_hwLedCount = getSetting(settings::DEVICE).object()["hardwareLedCount"].toInt(1);
	_colorOrder = getSetting(settings::DEVICE).object()["colorOrder"].toString("rgb");

	_muxer = MAKE_TRACKED_SHARED(PriorityMuxer, _hwLedCount, this);

	// connect Hyperion::update with Muxer visible priority changes as muxer updates independent
	connect(_muxer.get(), &PriorityMuxer::visiblePriorityChanged, this, &Hyperion::update);
	connect(_muxer.get(), &PriorityMuxer::visiblePriorityChanged, this, &Hyperion::handleSourceAvailability);
	connect(_muxer.get(), &PriorityMuxer::visibleComponentChanged, this, &Hyperion::handleVisibleComponentChanged);

	QJsonArray const ledLayout = getSetting(settings::LEDS).array();
	updateLedLayout(ledLayout);
	_ledBuffer = std::vector<ColorRgb>(static_cast<size_t>(_hwLedCount), ColorRgb::BLACK);

	// smoothing
	_deviceSmooth.reset(new LinearColorSmoothing(getSetting(settings::SMOOTHING).object(), this));
	connect(this, &Hyperion::settingsChanged, _deviceSmooth.get(), &LinearColorSmoothing::handleSettingsUpdate);
	_deviceSmooth->start();

	// initialize LED-devices
	QJsonObject const ledDeviceSettings = getSetting(settings::DEVICE).object();

	_ledDeviceWrapper.reset(new LedDeviceWrapper(this));
	connect(this, &Hyperion::compStateChangeRequest, _ledDeviceWrapper.get(), &LedDeviceWrapper::handleComponentState);
	connect(this, &Hyperion::ledDeviceData, _ledDeviceWrapper.get(), &LedDeviceWrapper::updateLeds);

	_ledDeviceWrapper->createLedDevice(ledDeviceSettings);

	// listen for suspend/resume, idle requests to perform core activation/deactivation actions
	connect(this, &Hyperion::suspendRequest, this, &Hyperion::setSuspend);
	connect(this, &Hyperion::idleRequest, this, &Hyperion::setIdle);
	
	_muxer->start();

#if defined(ENABLE_EFFECTENGINE)
	// create the effect engine; needs to be initialized after smoothing!
	_effectEngine = MAKE_TRACKED_SHARED(EffectEngine, this);
	connect(_effectEngine.get(), &EffectEngine::effectListUpdated, this, &Hyperion::effectListUpdated);
	connect(this, &Hyperion::stopEffects, _effectEngine.get(), &EffectEngine::stopAllEffects);
#endif
	// initial startup effect
	hyperion::handleInitialEffect(this, getSetting(settings::FGEFFECT).object());

	// handle background effect
	_BGEffectHandler.reset(new BGEffectHandler(this));

	// create the Daemon capture interface
	_captureCont.reset(new CaptureCont(this));

	// link global signals with the corresponding slots
	connect(GlobalSignals::getInstance(), &GlobalSignals::registerGlobalInput, this, &Hyperion::registerInput);
	connect(GlobalSignals::getInstance(), &GlobalSignals::clearGlobalInput, this, &Hyperion::clear);
	connect(GlobalSignals::getInstance(), &GlobalSignals::setGlobalColor, this, &Hyperion::setColor);
	connect(GlobalSignals::getInstance(), &GlobalSignals::setGlobalImage, this, &Hyperion::setInputImage);

	// Limit LED data emission, if high rumber of LEDs configured
	_rawLedDataEmissionInterval = (_ledString.leds().size() > 1000) ? 2 * DEFAULT_MAX_RAW_LED_DATA_EMISSION_INTERVAL : DEFAULT_MAX_RAW_LED_DATA_EMISSION_INTERVAL;
	_ledDeviceDataEmissionInterval = (_hwLedCount > 1000) ? 2 * DEFAULT_MAX_LED_DEVICE_DATA_EMISSION_INTERVAL : DEFAULT_MAX_LED_DEVICE_DATA_EMISSION_INTERVAL;

	// Set up timers to throttle specific signals
	_imageTimer.start();  // Start the elapsed timer for image signal throttling
	_rawLedDataTimer.start();  // Start the elapsed timer for rawLedColors throttling
	_ledDeviceDataTimer.start(); // Start the elapsed timer for LED-Device data throttling

	// if there is no startup / background effect and no sending capture interface we probably want to push once BLACK (as PrioMuxer won't emit a priority change)
	refreshUpdate();

#if defined(ENABLE_BOBLIGHT_SERVER)
	// boblight, can't live in global scope as it depends on layout
	_boblightServer.reset(new BoblightServer(this, getSetting(settings::BOBLSERVER)));
	connect(this, &Hyperion::settingsChanged, _boblightServer.get(), &BoblightServer::handleSettingsUpdate);
#endif

	// instance initiated, enter thread event loop
	emit started();
}

void Hyperion::stop(const QString name)
{
	Debug(_log, "Hyperion instance [%u] - %s is stopping.", _instIndex, QSTRING_CSTR(name));

	//Disconnect Background effect first that it does not kick in when other priorities are stopped
	_BGEffectHandler->disconnect();

#if defined(ENABLE_BOBLIGHT_SERVER)
	_boblightServer->stop();
#endif

	//Remove all priorities
	_muxer->clearAll(true);

#if defined(ENABLE_EFFECTENGINE)
	  _effectEngine->stopAllEffects();
 #endif

	 _ledDeviceWrapper->stopDevice();
	 _deviceSmooth->stop();
	 _muxer->stop();

	emit finished(name);
}

void Hyperion::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type == settings::COLOR)
	{

		updateLedColorAdjustment(_layoutLedCount, config.object());
		refreshUpdate();
	}
	else if(type == settings::LEDS)
	{
		#if defined(ENABLE_EFFECTENGINE)
		// stop and cache all running effects, as effects depend heavily on LED-layout
		_effectEngine->cacheRunningEffects();
		#endif

		updateLedLayout(config.array());

		#if defined(ENABLE_EFFECTENGINE)
		// start cached effects
		_effectEngine->startCachedEffects();
		#endif

		refreshUpdate();
	}
	else if(type == settings::DEVICE)
	{
		QJsonObject const deviceConfig = config.object();

		// Recreate LED-Device with new configuration
		_ledDeviceWrapper->createLedDevice(deviceConfig);
		_hwLedCount = _ledDeviceWrapper->getLedCount();
		_colorOrder = _ledDeviceWrapper->getColorOrder();

		updateLedLayout(getSetting(settings::LEDS).array());
		_ledBuffer.resize(static_cast<std::vector<ColorRgb>::size_type>(_hwLedCount), ColorRgb::BLACK);
	}
}

void Hyperion::updateLedColorAdjustment(int ledCount, const QJsonObject& colors)
{
	// change in LEDs are also reflected in adjustment
	_raw2ledAdjustment.reset(hyperion::createLedColorsAdjustment(ledCount, colors));
	if (!_raw2ledAdjustment->verifyAdjustments())
	{
		Warning(_log, "At least one LED has no color calibration, please add all LEDs from your LED layout to an 'LED index' field!");
	}
}

void Hyperion::updateLedLayout(const QJsonArray& ledLayout)
{
	_ledString = LedString::createLedString(ledLayout, hyperion::createColorOrder(_colorOrder), _hwLedCount);
	_layoutLedCount = static_cast<int>(_ledString.leds().size());
	_layoutGridSize = hyperion::getLedLayoutGridSize(ledLayout);

	_ledStringColorOrder.clear();
	for (const Led& led : _ledString.leds())
	{
		_ledStringColorOrder.push_back(led.colorOrder);
	}

	updateLedColorAdjustment(_layoutLedCount, getSetting(settings::COLOR).object());

	if (_imageProcessor.isNull())
	{
		_imageProcessor = MAKE_TRACKED_SHARED(ImageProcessor, _ledString, this);
	}
	else
	{
		_imageProcessor->setLedString(_ledString);
	}

	_muxer->updateLedColorsLength(_layoutLedCount);

	if (_layoutLedCount < static_cast<int>(_ledBuffer.size()))
	{
		std::fill(_ledBuffer.begin() + _layoutLedCount, _ledBuffer.end(), ColorRgb{0, 0, 0});
	}
}

QJsonDocument Hyperion::getSetting(settings::type type) const
{
	return _settingsManager->getSetting(type);
}

// TODO: Remove function, if UI is able to handle full configuration
QJsonObject Hyperion::getQJsonConfig(quint8 inst) const
{
	const QJsonObject instanceConfig = _settingsManager->getSettings(inst);
	const QJsonObject globalConfig = _settingsManager->getSettings({},QStringList());
	return JsonUtils::mergeJsonObjects(instanceConfig, globalConfig);
}

QPair<bool, QStringList> Hyperion::saveSettings(const QJsonObject& config)
{
	return _settingsManager->saveSettings(config);
}

int Hyperion::getLatchTime() const
{
	return _ledDeviceWrapper->getLatchTime();
}

unsigned Hyperion::addSmoothingConfig(int settlingTime_ms, double ledUpdateFrequency_hz, unsigned updateDelay)
{
	return _deviceSmooth->addConfig(settlingTime_ms, ledUpdateFrequency_hz, updateDelay);
}

unsigned Hyperion::updateSmoothingConfig(unsigned id, int settlingTime_ms, double ledUpdateFrequency_hz, unsigned updateDelay)
{
	return _deviceSmooth->updateConfig(id, settlingTime_ms, ledUpdateFrequency_hz, updateDelay);
}

int Hyperion::getLedCount() const
{
	return _layoutLedCount;
}

void Hyperion::setSourceAutoSelect(bool state)
{
	_muxer->setSourceAutoSelectEnabled(state);
}

bool Hyperion::setVisiblePriority(int priority)
{
	return _muxer->setPriority(priority);
}

bool Hyperion::sourceAutoSelectEnabled() const
{
	return _muxer->isSourceAutoSelectEnabled();
}

void Hyperion::setNewComponentState(hyperion::Components component, bool state)
{
	_componentRegister->setNewComponentState(component, state);
}

std::map<hyperion::Components, bool> Hyperion::getAllComponents() const
{
	return _componentRegister->getRegister();
}

int Hyperion::isComponentEnabled(hyperion::Components comp) const
{
	return _componentRegister->isComponentEnabled(comp);
}

void Hyperion::setSuspend(bool isSuspend)
{
	bool const enable = !isSuspend;
	emit compStateChangeRequest(hyperion::COMP_ALL, enable);
}

void Hyperion::setIdle(bool isIdle)
{
	clear(-1);

	bool const enable = !isIdle;
	emit compStateChangeRequestAll(enable, {hyperion::COMP_LEDDEVICE, hyperion::COMP_SMOOTHING} );
}

void Hyperion::registerInput(int priority, hyperion::Components component, const QString& origin, const QString& owner, unsigned smooth_cfg)
{
	_muxer->registerInput(priority, component, origin, owner, smooth_cfg);
}

bool Hyperion::setInput(int priority, const std::vector<ColorRgb>& ledColors, int timeout_ms, bool clearEffect)
{
	if(_muxer->setInput(priority, ledColors, timeout_ms))
	{
		#if defined(ENABLE_EFFECTENGINE)
		// clear effect if this call does not come from an effect
		if(clearEffect)
		{
			_effectEngine->channelCleared(priority);
		}
		#endif

		// if this priority is visible, update immediately
		if(priority == _muxer->getCurrentPriority())
		{
			refreshUpdate();
		}

		return true;
	}
	return false;
}

bool Hyperion::setInputImage(int priority, const Image<ColorRgb>& image, int64_t timeout_ms, bool clearEffect)
{
	if (!_muxer->hasPriority(priority))
	{
		emit GlobalSignals::getInstance()->globalRegRequired(priority);
		return false;
	}

	if(_muxer->setInputImage(priority, image, timeout_ms))
	{
		#if defined(ENABLE_EFFECTENGINE)
		// clear effect if this call does not come from an effect
		if(clearEffect)
		{
			_effectEngine->channelCleared(priority);
		}
		#endif

		// if this priority is visible, update immediately
		if(priority == _muxer->getCurrentPriority())
		{
			refreshUpdate();
		}

		return true;
	}
	return false;
}

bool Hyperion::setInputInactive(int priority)
{
	return _muxer->setInputInactive(priority);
}

void Hyperion::setColor(int priority, const std::vector<ColorRgb> &ledColors, int timeout_ms, const QString &origin, bool clearEffects)
{
	#if defined(ENABLE_EFFECTENGINE)
	// clear effect if this call does not come from an effect
	if (clearEffects)
	{
		_effectEngine->channelCleared(priority);
	}
	#endif

	// create full led vector from single/multiple colors
	std::vector<ColorRgb> newLedColors;

	size_t const size = static_cast<size_t>(_layoutLedCount);
	newLedColors.reserve(size);

	if (ledColors.size() == 1)
	{
		// Special case: single color, fill the entire vector
		newLedColors.resize(size, ledColors[0]);
	}
	else
	{
		// General case: multiple colors
		size_t const fullCopies = size / ledColors.size();
		size_t const remainder = size % ledColors.size();

		// Add full copies of `ledColors`
		for (size_t i = 0; i < fullCopies; ++i)
		{
			newLedColors.insert(newLedColors.end(), ledColors.begin(), ledColors.end());
		}

		// Add remaining elements
		if (remainder > 0)
		{
			newLedColors.insert(newLedColors.end(), ledColors.begin(), ledColors.begin() + remainder);
		}
	}

	// register color
	registerInput(priority, hyperion::COMP_COLOR, origin);

	// write color to muxer
	setInput(priority, newLedColors, timeout_ms);
}

QStringList Hyperion::getAdjustmentIds() const
{
	return _raw2ledAdjustment->getAdjustmentIds();
}

ColorAdjustment * Hyperion::getAdjustment(const QString& identifier) const
{
	return _raw2ledAdjustment->getAdjustment(identifier);
}

void Hyperion::adjustmentsUpdated()
{
	emit adjustmentChanged();
	refreshUpdate();
}

bool Hyperion::clear(int priority, bool forceClearAll)
{
	bool isCleared = false;
	if (priority < 0)
	{
		_muxer->clearAll(forceClearAll);

		#if defined(ENABLE_EFFECTENGINE)
		// send clearall signal to the effect engine
		_effectEngine->allChannelsCleared();
		#endif

		isCleared = true;
	}
	else
	{
		#if defined(ENABLE_EFFECTENGINE)
		// send clear signal to the effect engine
		// (outside the check so the effect gets cleared even when the effect is not sending colors)
		_effectEngine->channelCleared(priority);
		#endif

		if (_muxer->clearInput(priority))
		{
			isCleared = true;
		}
	}
	return isCleared;
}

int Hyperion::getCurrentPriority() const
{
	return  _muxer.isNull() ? PriorityMuxer::LOWEST_PRIORITY : _muxer->getCurrentPriority();
}

bool Hyperion::isCurrentPriority(int priority) const
{
	return getCurrentPriority() == priority;
}

QList<int> Hyperion::getActivePriorities() const
{
	return _muxer->getPriorities();
}

Hyperion::InputsMap Hyperion::getPriorityInfo() const
{
    return _muxer->getInputInfo();
}

Hyperion::InputInfo Hyperion::getPriorityInfo(int priority) const
{
	return _muxer->getInputInfo(priority);
}

#if defined(ENABLE_EFFECTENGINE)
std::list<ActiveEffectDefinition> Hyperion::getActiveEffects() const
{
	return _effectEngine->getActiveEffects();
}

int Hyperion::setEffect(const QString &effectName, int priority, int timeout, const QString & origin)
{
	return _effectEngine->runEffect(effectName, priority, timeout, origin);
}

int Hyperion::setEffect(const QString &effectName, const QJsonObject &args, int priority, int timeout, const QString &pythonScript, const QString &origin, const QString &imageData)
{
	return _effectEngine->runEffect(effectName, args, priority, timeout, pythonScript, origin, 0, imageData);
}
#endif

void Hyperion::setLedMappingType(int mappingType)
{
	if(mappingType != _imageProcessor->getUserLedMappingType())
	{
		_imageProcessor->setLedMappingType(mappingType);
		emit imageToLedsMappingChanged(mappingType);
	}
}

int Hyperion::getLedMappingType() const
{
	return _imageProcessor->getUserLedMappingType();
}

void Hyperion::setVideoMode(VideoMode mode)
{
	emit videoMode(mode);
}

VideoMode Hyperion::getCurrentVideoMode() const
{
	return _currVideoMode;
}

QString Hyperion::getActiveDeviceType() const
{
	return _ledDeviceWrapper->getActiveDeviceType();
}

void Hyperion::handleVisibleComponentChanged(hyperion::Components comp)
{
	_imageProcessor->setBlackbarDetectDisable((comp == hyperion::COMP_EFFECT));
	_imageProcessor->setHardLedMappingType((comp == hyperion::COMP_EFFECT) ? 0 : -1);
	_raw2ledAdjustment->setBacklightEnabled((comp != hyperion::COMP_COLOR && comp != hyperion::COMP_EFFECT));
}

void Hyperion::handleSourceAvailability(int priority)
{
	int const previousPriority = _muxer->getPreviousPriority();

	if ( priority == PriorityMuxer::LOWEST_PRIORITY)
	{
		// Keep LED-device on, as background effect will kick-in shortly
		if (!_BGEffectHandler->_isEnabled())
		{
			Debug(_log,"No source left -> Pause output processing and switch LED-Device off");
			emit _ledDeviceWrapper->switchOff();
			_deviceSmooth->setPause(true);
		}
	}
	else
	{
		if ( previousPriority == PriorityMuxer::LOWEST_PRIORITY )
		{
			if(_ledDeviceWrapper->isEnabled())
			{
				Debug(_log,"new source available -> LED-Device is enabled, switch LED-device on and resume output processing");
				emit _ledDeviceWrapper->switchOn();
			}
			else
			{
				Debug(_log,"new source available -> LED-Device not enabled, cannot switch on LED-device");
			}
			_deviceSmooth->setPause(false);
		}
	}
}

void Hyperion::refreshUpdate()
{
	wait(_ledDeviceWrapper->getLatchTime());
	update();
}

void Hyperion::update()
{
	// Obtain the current priority channel
	int const priority = _muxer->getCurrentPriority();
	const PriorityMuxer::InputInfo priorityInfo = _muxer->getInputInfo(priority);

	std::vector<ColorRgb> ledColors;

	// copy image & process OR copy ledColors from muxer
	Image<ColorRgb> const image = priorityInfo.image;
	if (image.width() > 1 || image.height() > 1)
	{
		_imageEmissionInterval = (image.width() > 1280) ?  2 * DEFAULT_MAX_IMAGE_EMISSION_INTERVAL : DEFAULT_MAX_IMAGE_EMISSION_INTERVAL;
		// Throttle the emission of currentImage(image) signal
		qint64 elapsedImageEmissionTime = _imageTimer.elapsed();
		if (elapsedImageEmissionTime - _lastImageEmission >= _imageEmissionInterval.count())
		{
			_lastImageEmission = elapsedImageEmissionTime;
			emit currentImage(image);  // Emit the image signal at the controlled rate
		}
		ledColors = _imageProcessor->process(image);
	}
	else
	{
		ledColors = priorityInfo.ledColors;
		if (_ledString.hasBlackListedLeds())
		{
			for (unsigned long const id : _ledString.blacklistedLedIds())
			{
				if (id > ledColors.size()-1)
				{
					break;
				}
				ledColors.at(id) = ColorRgb::BLACK;
			}
		}
	}

	// Throttle the emission of rawLedColors(_ledBuffer) signal
	qint64 elapsedRawLedDataEmissionTime = _rawLedDataTimer.elapsed();
	if (elapsedRawLedDataEmissionTime - _lastRawLedDataEmission >= _rawLedDataEmissionInterval.count())
	{
		_lastRawLedDataEmission = elapsedRawLedDataEmissionTime;
		emit rawLedColors(ledColors);  // Emit the rawLedColors signal at the controlled rate
	}

	// Start transformations
	_raw2ledAdjustment->applyAdjustment(ledColors);

	assert(ledColors.size() >= _ledStringColorOrder.size());

	// Only apply color order for LEDs defined by layout
	for (size_t i=0; i < _ledStringColorOrder.size(); ++i)
	{
		ColorRgb& color = ledColors.at(i);
		// correct the color byte order
		switch (_ledStringColorOrder.at(i))
		{
		case ColorOrder::ORDER_RGB:
			// leave as it is
			break;
		case ColorOrder::ORDER_BGR:
			std::swap(color.red, color.blue);
			break;
		case ColorOrder::ORDER_RBG:
			std::swap(color.green, color.blue);
			break;
		case ColorOrder::ORDER_GRB:
			std::swap(color.red, color.green);
			break;
		case ColorOrder::ORDER_GBR:
			std::swap(color.red, color.green);
			std::swap(color.green, color.blue);
			break;

		case ColorOrder::ORDER_BRG:
			std::swap(color.red, color.blue);
			std::swap(color.green, color.blue);
			break;
		}
	}

	// Copy elements from ledColors to _ledBuffer up to the size of _ledBuffer
	std::copy_n(ledColors.begin(), std::min(_ledBuffer.size(), ledColors.size()), _ledBuffer.begin());

	if (_ledDeviceWrapper->isOn())
	{
		// Smoothing is disabled
		if  (! _deviceSmooth->enabled())
		{
			// Throttle the emission of LED-Device data signal
			qint64 elapsedLedDeviceDataEmissionTime = _ledDeviceDataTimer.elapsed();
			if (elapsedLedDeviceDataEmissionTime - _lastLedDeviceDataEmission >= _ledDeviceDataEmissionInterval.count())
			{
				_lastLedDeviceDataEmission = elapsedLedDeviceDataEmissionTime;
				emit ledDeviceData(_ledBuffer);
			}
		}
		else
		{
			// feed smoothing in pause mode to maintain a smooth transition back to smooth mode
			if (_deviceSmooth->enabled() || _deviceSmooth->pause())
			{
				_deviceSmooth->updateLedValues(_ledBuffer);
			}
		}
	}
}
