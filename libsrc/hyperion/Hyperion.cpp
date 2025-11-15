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
#include "utils/MemoryTracker.h"

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

Hyperion::Hyperion(quint8 instance, QObject* parent)
	: QObject(parent)
	, _instIndex(instance)
	, _settingsManager(nullptr)
	, _componentRegister(nullptr)
	, _imageProcessor(nullptr)
	, _raw2ledAdjustment(nullptr)
	, _muxer(nullptr)
	, _ledDeviceWrapper(nullptr)
	, _deviceSmooth(nullptr)
	, _captureCont(nullptr)
	, _BGEffectHandler(nullptr)
#if defined(ENABLE_EFFECTENGINE)	
	, _effectEngine(nullptr)
#endif	
#if defined(ENABLE_BOBLIGHT_SERVER)
	, _boblightServer(nullptr)
#endif
	, _log(nullptr)
	, _hwLedCount(0)
	, _layoutLedCount(0)
	, _colorOrder("rgb")
{
	qRegisterMetaType<ComponentList>("ComponentList");
	qRegisterMetaType<Image<ColorRgb>>("ColorRgbImage");

	QString const subComponent = "I" + QString::number(_instIndex);
	this->setProperty("instance", QVariant::fromValue(subComponent));

	_log = Logger::getInstance("HYPERION", subComponent);
	TRACK_SCOPE_SUBCOMPONENT();
}

Hyperion::~Hyperion()
{
	Debug(_log, "Hyperion instance [%u] is stopping...", _instIndex);
	TRACK_SCOPE_SUBCOMPONENT();
}

void Hyperion::start()
{
	Debug(_log, "Hyperion instance starting...");

	_settingsManager.reset(new SettingsManager(_instIndex));

	// link settings changed with the current Hyperion instance
	connect(_settingsManager.get(), &SettingsManager::settingsChanged, this, &Hyperion::settingsChanged);
	// listen for settings updates of this instance (LEDS & COLOR)
	connect(_settingsManager.get(), &SettingsManager::settingsChanged, this, &Hyperion::handleSettingsUpdate);

	_componentRegister = MAKE_TRACKED_SHARED(ComponentRegister, sharedFromThis());
	connect(this, &Hyperion::isSetNewComponentState, _componentRegister.get(), &ComponentRegister::setNewComponentState);

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
	_ledBuffer = QVector<ColorRgb>(static_cast<size_t>(_hwLedCount), ColorRgb::BLACK);

	// smoothing
	_deviceSmooth = MAKE_TRACKED_SHARED(LinearColorSmoothing, getSetting(settings::SMOOTHING).object(), sharedFromThis());

	connect(this, &Hyperion::settingsChanged, _deviceSmooth.get(), &LinearColorSmoothing::handleSettingsUpdate);
	_deviceSmooth->start();

	// initialize LED-devices
	QJsonObject const ledDeviceSettings = getSetting(settings::DEVICE).object();

	_ledDeviceWrapper = MAKE_TRACKED_SHARED(LedDeviceWrapper, sharedFromThis());
	connect(this, &Hyperion::compStateChangeRequest, _ledDeviceWrapper.get(), &LedDeviceWrapper::handleComponentState);
	connect(this, &Hyperion::ledDeviceData, _ledDeviceWrapper.get(), &LedDeviceWrapper::updateLeds);

	_ledDeviceWrapper->createLedDevice(ledDeviceSettings);

	// listen for suspend/resume, idle requests to perform core activation/deactivation actions
	connect(this, &Hyperion::suspendRequest, this, &Hyperion::setSuspend);
	connect(this, &Hyperion::idleRequest, this, &Hyperion::setIdle);

	_muxer->start();

#if defined(ENABLE_EFFECTENGINE)
	// create the effect engine; needs to be initialized after smoothing!
	_effectEngine = MAKE_TRACKED_SHARED(EffectEngine, sharedFromThis());
	connect(_effectEngine.get(), &EffectEngine::effectListUpdated, this, &Hyperion::effectListUpdated);
	connect(this, &Hyperion::stopEffects, _effectEngine.get(), &EffectEngine::stopAllEffects);
#endif
	// initial startup effect
	hyperion::handleInitialEffect(this, getSetting(settings::FGEFFECT).object());

	// handle background effect
	_BGEffectHandler = MAKE_TRACKED_SHARED(BGEffectHandler, sharedFromThis());

	// create the Daemon capture interface
	_captureCont = MAKE_TRACKED_SHARED(CaptureCont, sharedFromThis());
	_captureCont->start();

	// link global signals with the corresponding slots
	connect(GlobalSignals::getInstance(), &GlobalSignals::registerGlobalInput, this, &Hyperion::registerInput);
	connect(GlobalSignals::getInstance(), &GlobalSignals::clearGlobalInput, this, &Hyperion::clear);
	connect(GlobalSignals::getInstance(), &GlobalSignals::setGlobalColor, this, &Hyperion::setColor);
	connect(GlobalSignals::getInstance(), &GlobalSignals::setGlobalImage, this, &Hyperion::setInputImage);

	// if there is no startup / background effect and no sending capture interface we probably want to push once BLACK (as PrioMuxer won't emit a priority change)
	refreshUpdate();

#if defined(ENABLE_BOBLIGHT_SERVER)
	// boblight, can't live in global scope as it depends on layout
	_boblightServer = MAKE_TRACKED_SHARED(BoblightServer, sharedFromThis(), getSetting(settings::BOBLSERVER));

	connect(this, &Hyperion::settingsChanged, _boblightServer.get(), &BoblightServer::handleSettingsUpdate);
#endif

	// instance initiated, enter thread event loop
	emit started();
}

void Hyperion::stop(const QString name)
{
	Debug(_log, "Hyperion instance [%u] - %s is stopping.", _instIndex, QSTRING_CSTR(name));

	//Stop Background effect first that it does not kick in when other priorities are stopped
	_BGEffectHandler->stop();

	_captureCont->stop();

#if defined(ENABLE_BOBLIGHT_SERVER)
	_boblightServer->stop();
	_boblightServer.clear();
#endif

	//Remove all priorities
	_muxer->clearAll(true);

#if defined(ENABLE_EFFECTENGINE)
	_effectEngine->stopAllEffects();
	_effectEngine.clear();
#endif

	_muxer->stop();
	_deviceSmooth->stop();

	_raw2ledAdjustment.reset();

	_ledDeviceWrapper->stopDevice();

	//Clear all objects maintained/shared by <Hyperion> being the master
	_BGEffectHandler.clear();
	_captureCont.clear();
	_deviceSmooth.clear();
	_ledDeviceWrapper.clear();
	_muxer.clear();
	_imageProcessor.clear();
	_componentRegister.clear();
	_settingsManager.reset();

	emit finished(name);
}

void Hyperion::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if (type == settings::COLOR)
	{

		updateLedColorAdjustment(_layoutLedCount, config.object());
		refreshUpdate();
	}
	else if (type == settings::LEDS)
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
	else if (type == settings::DEVICE)
	{
		QJsonObject const deviceConfig = config.object();

		// Recreate LED-Device with new configuration
		_ledDeviceWrapper->createLedDevice(deviceConfig);
		_hwLedCount = _ledDeviceWrapper->getLedCount();
		_colorOrder = _ledDeviceWrapper->getColorOrder();

		updateLedLayout(getSetting(settings::LEDS).array());
		_ledBuffer.fill(ColorRgb::BLACK, _hwLedCount);
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
		_imageProcessor = MAKE_TRACKED_SHARED(ImageProcessor, _ledString, sharedFromThis());
	}
	else
	{
		_imageProcessor->setLedString(_ledString);
	}

	_muxer->updateLedColorsLength(_layoutLedCount);

	if (_layoutLedCount < static_cast<int>(_ledBuffer.size()))
	{
		std::fill(_ledBuffer.begin() + _layoutLedCount, _ledBuffer.end(), ColorRgb{ 0, 0, 0 });
	}
}

QJsonDocument Hyperion::getSetting(settings::type type) const
{
	return _settingsManager->getSetting(type);
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
	if (_componentRegister.isNull())
	{
		Debug(_log, "ComponentRegister is not initialized, cannot set state for component '%s'", componentToString(component));
	}

	emit isSetNewComponentState(component, state);
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
	emit compStateChangeRequestAll(enable, { hyperion::COMP_LEDDEVICE, hyperion::COMP_SMOOTHING });
}

void Hyperion::registerInput(int priority, hyperion::Components component, const QString& origin, const QString& owner, unsigned smooth_cfg)
{
	_muxer->registerInput(priority, component, origin, owner, smooth_cfg);
}

bool Hyperion::setInput(int priority, const QVector<ColorRgb>& ledColors, int timeout_ms, bool clearEffect)
{
	if (_muxer->setInput(priority, ledColors, timeout_ms))
	{
#if defined(ENABLE_EFFECTENGINE)
		// clear effect if this call does not come from an effect
		if (clearEffect)
		{
			_effectEngine->channelCleared(priority);
		}
#endif

		// if this priority is visible, update immediately
		if (priority == _muxer->getCurrentPriority())
		{
			update();
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

	if (_muxer->setInputImage(priority, image, timeout_ms))
	{
#if defined(ENABLE_EFFECTENGINE)
		// clear effect if this call does not come from an effect
		if (clearEffect)
		{
			_effectEngine->channelCleared(priority);
		}
#endif

		// if this priority is visible, update immediately
		if (priority == _muxer->getCurrentPriority())
		{
			update();
		}

		return true;
	}
	return false;
}

bool Hyperion::setInputInactive(int priority)
{
	return _muxer->setInputInactive(priority);
}

void Hyperion::setColor(int priority, const QVector<ColorRgb>& ledColors, int timeout_ms, const QString& origin, bool clearEffects)
{
#if defined(ENABLE_EFFECTENGINE)
	// clear effect if this call does not come from an effect
	if (clearEffects)
	{
		_effectEngine->channelCleared(priority);
	}
#endif

	// create full led vector from single/multiple colors

	QVector<ColorRgb> newLedColors;
	newLedColors.resize(_layoutLedCount);

	if (!ledColors.isEmpty())
	{
		const int ledCount   = _layoutLedCount;
		const auto colorCount = ledColors.size();

		if (colorCount == 1)
		{
			// Special case: single color, fill the entire vector
			newLedColors.fill(ledColors[0]);
		}
		else
		{
			// General case: multiple colors, repeat colors if necessary to fill the entire vector
			for (int i = 0; i < ledCount; ++i)
			{
				newLedColors[i] = ledColors[i % colorCount];		
			}
		}
	}
	else
	{
		// no color provided, set all to black
		newLedColors.fill(ColorRgb::BLACK);
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

ColorAdjustment* Hyperion::getAdjustment(const QString& identifier) const
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

int Hyperion::setEffect(const QString& effectName, int priority, int timeout, const QString& origin)
{
	return _effectEngine->runEffect(effectName, priority, timeout, origin);
}

int Hyperion::setEffect(const QString& effectName, const QJsonObject& args, int priority, int timeout, const QString& pythonScript, const QString& origin, const QString& imageData)
{
	return _effectEngine->runEffect(effectName, args, priority, timeout, pythonScript, origin, 0, imageData);
}
#endif

void Hyperion::setLedMappingType(int mappingType)
{
	if (mappingType != _imageProcessor->getUserLedMappingType())
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
	_imageProcessor->setBlackbarDetectDisable(comp == hyperion::COMP_EFFECT);
	_imageProcessor->setHardLedMappingType((comp == hyperion::COMP_EFFECT) ? 0 : -1);
	_raw2ledAdjustment->setBacklightEnabled(comp != hyperion::COMP_COLOR && comp != hyperion::COMP_EFFECT);
}

void Hyperion::handleSourceAvailability(int priority)
{
	int const previousPriority = _muxer->getPreviousPriority();

	if (priority == PriorityMuxer::LOWEST_PRIORITY)
	{
		// Keep LED-device on, as background effect will kick-in shortly
		if (!_BGEffectHandler->_isEnabled())
		{
			Debug(_log, "No source left -> Pause output processing and switch LED-Device off");
			emit _ledDeviceWrapper->switchOff();
			_deviceSmooth->setPause(true);
		}
	}
	else
	{
		if (previousPriority == PriorityMuxer::LOWEST_PRIORITY)
		{
			if (_ledDeviceWrapper->isEnabled())
			{
				Debug(_log, "new source available -> LED-Device is enabled, switch LED-device on and resume output processing");
				emit _ledDeviceWrapper->switchOn();
			}
			else
			{
				Debug(_log, "new source available -> LED-Device not enabled, cannot switch on LED-device");
			}
			_deviceSmooth->setPause(false);
		}
	}
}

void Hyperion::applyBlacklist(QVector<ColorRgb>& ledColors)
{
	if (_ledString.hasBlackListedLeds())
	{
		for (const auto& id : _ledString.blacklistedLedIds())
		{
			if (id < ledColors.size())
			{
				ledColors[id] = ColorRgb::BLACK;
			}
		}
	}
}

void Hyperion::applyColorOrder(QVector<ColorRgb>& ledColors) const
{
	assert(ledColors.size() >= _ledStringColorOrder.size());

	// Only apply color order for LEDs defined by layout
	for (auto i = 0; i < _ledStringColorOrder.size(); ++i)
	{
		auto& color = ledColors[i];
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
}

void Hyperion::writeToLeds()
{
	if (_ledDeviceWrapper->isOn())
	{
		// Smoothing is disabled
		if (!_deviceSmooth->enabled())
		{
				emit ledDeviceData(_ledBuffer);
		}
		else
		{
			// device is enabled, feed smoothing in pause mode to maintain a smooth transition back to smooth mode
			if (!_deviceSmooth->pause())
			{
				_deviceSmooth->updateLedValues(_ledBuffer);
			}
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
	const PriorityMuxer::InputInfo& priorityInfo = _muxer->getInputInfo(_muxer->getCurrentPriority());

	// copy image & process OR copy ledColors from muxer
	const Image<ColorRgb>& image = priorityInfo.image;

	if (image.isNull())
	{
		qDebug() << "Empty image - skip update";
		return;
	}

	QVector<ColorRgb> ledColors;
	if (image.width() > 1 || image.height() > 1)
	{
		emit currentImage(image);  // Emit the image signal at the controlled rate
		ledColors = _imageProcessor->process(image);
	}
	else
	{
		ledColors = priorityInfo.ledColors;
	}

	emit rawLedColors(ledColors);
	applyBlacklist(ledColors);

	// Start transformations
	_raw2ledAdjustment->applyAdjustment(ledColors);

	applyColorOrder(ledColors);

	// Copy elements from ledColors to _ledBuffer up to the size of _ledBuffer
	std::copy_n(ledColors.begin(), std::min<qsizetype>(_ledBuffer.size(), ledColors.size()), _ledBuffer.begin());

	writeToLeds();
}