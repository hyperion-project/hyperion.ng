// STL includes
#include<algorithm>

// QT includes
#include <QString>
#include <QStringList>
#include <QThread>
#include <QVariantMap>

// hyperion include
#include <hyperion/Hyperion.h>

#if defined(ENABLE_FORWARDER)
#include <forwarder/MessageForwarder.h>
#endif

#include <hyperion/ImageProcessor.h>
#include <hyperion/ColorAdjustment.h>

// utils
#include <utils/hyperion.h>
#include <utils/GlobalSignals.h>
#include <utils/Logger.h>
#include <utils/JsonUtils.h>
#include "utils/WaitTime.h"

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
	, _ledString()
	, _imageProcessor(nullptr)
	, _muxer(nullptr)
	, _raw2ledAdjustment(nullptr)
	, _ledDeviceWrapper(nullptr)
	, _deviceSmooth(nullptr)
#if defined(ENABLE_EFFECTENGINE)
	, _effectEngine(nullptr)
#endif
#if defined(ENABLE_FORWARDER)
	, _messageForwarder(nullptr)
#endif
	, _log(nullptr)
	, _hwLedCount(0)
	, _colorOrder("rgb")
	, _layoutGridSize()
	, _BGEffectHandler(nullptr)
	, _captureCont(nullptr)
	, _ledBuffer()
#if defined(ENABLE_BOBLIGHT_SERVER)
	, _boblightServer(nullptr)
#endif
{
	qRegisterMetaType<ComponentList>("ComponentList");

	QString const subComponent = "I"+QString::number(_instIndex);
	this->setProperty("instance", QVariant::fromValue(subComponent));

	_log= Logger::getInstance("HYPERION", subComponent);

	_settingsManager.reset(new SettingsManager(instance, this));
	_colorOrder = getSetting(settings::DEVICE).object()["colorOrder"].toString("rgb");
	_ledString = LedString::createLedString(getSetting(settings::LEDS).array(), hyperion::createColorOrder(_colorOrder));
	_raw2ledAdjustment.reset(hyperion::createLedColorsAdjustment(static_cast<int>(_ledString.leds().size()), getSetting(settings::COLOR).object()));
	_layoutGridSize = hyperion::getLedLayoutGridSize(getSetting(settings::LEDS).array());
	_ledBuffer = {_ledString.leds().size(), ColorRgb::BLACK};

	_componentRegister.reset(new ComponentRegister(this));
	_imageProcessor.reset(new ImageProcessor(_ledString, this));
}

Hyperion::~Hyperion()
{
}

void Hyperion::start()
{
	Debug(_log, "Hyperion instance starting...");

	// forward settings changed to Hyperion
	connect(_settingsManager.get(), &SettingsManager::settingsChanged, this, &Hyperion::settingsChanged);

	// get newVideoMode from HyperionIManager
	connect(this, &Hyperion::newVideoMode, this, &Hyperion::handleNewVideoMode);

	if (!_raw2ledAdjustment->verifyAdjustments())
	{
		Warning(_log, "At least one led has no color calibration, please add all leds from your led layout to an 'LED index' field!");
	}

	// handle hwLedCount
	_hwLedCount = getSetting(settings::DEVICE).object()["hardwareLedCount"].toInt(getLedCount());

	// Initialize colororder vector
	for (const Led& led : _ledString.leds())
	{
		_ledStringColorOrder.push_back(led.colorOrder);
	}

	_muxer.reset(new PriorityMuxer(static_cast<int>(_ledString.leds().size()), this));

	// connect Hyperion::update with Muxer visible priority changes as muxer updates independent
	connect(_muxer.get(), &PriorityMuxer::visiblePriorityChanged, this, &Hyperion::update);
	connect(_muxer.get(), &PriorityMuxer::visiblePriorityChanged, this, &Hyperion::handleSourceAvailability);
	connect(_muxer.get(), &PriorityMuxer::visibleComponentChanged, this, &Hyperion::handleVisibleComponentChanged);

	// listen for suspend/resume, idle requests to perform core activation/deactivation actions
	connect(this, &Hyperion::suspendRequest, this, &Hyperion::setSuspend);
	connect(this, &Hyperion::idleRequest, this, &Hyperion::setIdle);

	// listen for settings updates of this instance (LEDS & COLOR)
	connect(_settingsManager.get(), &SettingsManager::settingsChanged, this, &Hyperion::handleSettingsUpdate);

	#if 0
	// set color correction activity state
	const QJsonObject color = getSetting(settings::COLOR).object();
	#endif

	// initialize LED-devices
	QJsonObject const ledDeviceSettings = getSetting(settings::DEVICE).object();
	ledDeviceSettings["currentLedCount"] = _hwLedCount; // Inject led count info
	_colorOrder = getSetting(settings::DEVICE).object()["colorOrder"].toString("rgb");

	_ledDeviceWrapper.reset(new LedDeviceWrapper(this));
	connect(this, &Hyperion::compStateChangeRequest, _ledDeviceWrapper.get(), &LedDeviceWrapper::handleComponentState);
	connect(this, &Hyperion::ledDeviceData, _ledDeviceWrapper.get(), &LedDeviceWrapper::updateLeds);

	 _ledDeviceWrapper->createLedDevice(ledDeviceSettings);

	// smoothing
	_deviceSmooth.reset(new LinearColorSmoothing(getSetting(settings::SMOOTHING), this));
	connect(this, &Hyperion::settingsChanged, _deviceSmooth.get(), &LinearColorSmoothing::handleSettingsUpdate);

	_deviceSmooth->start();
	_muxer->start();

#if defined(ENABLE_FORWARDER)
	// create the message forwarder only on main instance
	if (_instIndex == 0)
	{
		_messageForwarder.reset(new MessageForwarder(this));
		_messageForwarder->handleSettingsUpdate(settings::NETFORWARD, getSetting(settings::NETFORWARD));
		#if defined(ENABLE_FLATBUF_SERVER) || defined(ENABLE_PROTOBUF_SERVER)
		connect(GlobalSignals::getInstance(), &GlobalSignals::setBufferImage, this, &Hyperion::forwardBufferMessage);
		#endif
	}
#endif

#if defined(ENABLE_EFFECTENGINE)
	// create the effect engine; needs to be initialized after smoothing!
	_effectEngine.reset(new EffectEngine(this));
	connect(_effectEngine.get(), &EffectEngine::effectListUpdated, this, &Hyperion::effectListUpdated);
	connect(this, &Hyperion::stopEffects, _effectEngine.get(), &EffectEngine::stopAllEffects);
#endif
	// initial startup effect
	hyperion::handleInitialEffect(this, getSetting(settings::FGEFFECT).object());

	// handle background effect
	_BGEffectHandler.reset(new BGEffectHandler(this));

	// create the Daemon capture interface
	_captureCont.reset(new CaptureCont(this));

	// forwards global signals to the corresponding slots
	connect(GlobalSignals::getInstance(), &GlobalSignals::registerGlobalInput, this, &Hyperion::registerInput);
	connect(GlobalSignals::getInstance(), &GlobalSignals::clearGlobalInput, this, &Hyperion::clear);
	connect(GlobalSignals::getInstance(), &GlobalSignals::setGlobalColor, this, &Hyperion::setColor);
	connect(GlobalSignals::getInstance(), &GlobalSignals::setGlobalImage, this, &Hyperion::setInputImage);

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

void Hyperion::stop()
{
	Debug(_log, "Hyperion instance %d is stopping", _instIndex);

	//Disconnect Background effect first that it does not kick in when other priorities are stopped
	_BGEffectHandler->disconnect();
	 _boblightServer.get()->stop();

	//Remove all priorities
	_muxer->clearAll(true);

#if defined(ENABLE_EFFECTENGINE)
	  _effectEngine->stopAllEffects();
 #endif

	 _ledDeviceWrapper->stopDevice();
	 _deviceSmooth->stop();
	 _muxer->stop();

	emit finished();
}

void Hyperion::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type == settings::COLOR)
	{
		const QJsonObject obj = config.object();
		// change in color recreate ledAdjustments
		_raw2ledAdjustment.reset(hyperion::createLedColorsAdjustment(static_cast<int>(_ledString.leds().size()), obj));

		if (!_raw2ledAdjustment->verifyAdjustments())
		{
			Warning(_log, "At least one led has no color calibration, please add all leds from your led layout to an 'LED index' field!");
		}
	}
	else if(type == settings::LEDS)
	{
		const QJsonArray leds = config.array();

		#if defined(ENABLE_EFFECTENGINE)
		// stop and cache all running effects, as effects depend heavily on LED-layout
		_effectEngine->cacheRunningEffects();
		#endif

		// ledstring, img processor, muxer, ledGridSize (effect-engine image based effects), _ledBuffer and ByteOrder of ledstring
		_colorOrder = getSetting(settings::DEVICE).object()["colorOrder"].toString("rgb");
		_ledString = LedString::createLedString(leds, hyperion::createColorOrder(_colorOrder));
		_imageProcessor->setLedString(_ledString);
		_muxer->updateLedColorsLength(static_cast<int>(_ledString.leds().size()));
		_layoutGridSize = hyperion::getLedLayoutGridSize(leds);

		std::vector<ColorRgb> color(_ledString.leds().size(), ColorRgb::BLACK);
		_ledBuffer = color;

		_ledStringColorOrder.clear();
		for (const Led& led : _ledString.leds())
		{
			_ledStringColorOrder.push_back(led.colorOrder);
		}

		// handle hwLedCount update
		_hwLedCount = getSetting(settings::DEVICE).object()["hardwareLedCount"].toInt(getLedCount());

		// change in leds are also reflected in adjustment
		_raw2ledAdjustment.reset(hyperion::createLedColorsAdjustment(static_cast<int>(_ledString.leds().size()), getSetting(settings::COLOR).object()));

		#if defined(ENABLE_EFFECTENGINE)
		// start cached effects
		_effectEngine->startCachedEffects();
		#endif

		refreshUpdate();
	}
	else if(type == settings::DEVICE)
	{
		QJsonObject dev = config.object();

		// handle hwLedCount update
		_hwLedCount = dev["hardwareLedCount"].toInt(getLedCount());

		// force ledString update, if device ByteOrder changed
		QString colorOrder = getSetting(settings::DEVICE).object()["colorOrder"].toString("rgb");
		if(_ledDeviceWrapper->getColorOrder() !=colorOrder)
		{
			_ledString = LedString::createLedString(getSetting(settings::LEDS).array(), hyperion::createColorOrder(colorOrder));
			_imageProcessor->setLedString(_ledString);

			_ledStringColorOrder.clear();
			for (const Led& led : _ledString.leds())
			{
				_ledStringColorOrder.push_back(led.colorOrder);
			}
		}

		// do always reinit until the led devices can handle dynamic changes
		dev["currentLedCount"] = _hwLedCount; // Inject led count info
		_ledDeviceWrapper->createLedDevice(dev);

		// TODO: Check, if framegrabber frequency is lower than latchtime..., if yes, stop
	}

	// update once to push single color sets / adjustments/ ledlayout resizes and update ledBuffer color
	refreshUpdate();
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
	return static_cast<int>(_ledString.leds().size());
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
	emit compStateChangeRequestAll(enable);
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

bool Hyperion::setInputInactive(quint8 priority)
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
	size_t const size = _ledString.leds().size();
	std::vector<ColorRgb> newLedColors;
	while (true)
	{
		for (const auto &entry : ledColors)
		{
			newLedColors.emplace_back(entry);
			if (newLedColors.size() == size)
			{
				goto end;
			}
		}
	}
end:

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
	return _muxer->getCurrentPriority();
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
QString Hyperion::saveEffect(const QJsonObject& obj)
{
	return _effectEngine->saveEffect(obj);
}

QString Hyperion::deleteEffect(const QString& effectName)
{
	return _effectEngine->deleteEffect(effectName);
}

std::list<EffectDefinition> Hyperion::getEffects() const
{
	return _effectEngine->getEffects();
}

std::list<ActiveEffectDefinition> Hyperion::getActiveEffects() const
{
	return _effectEngine->getActiveEffects();
}

std::list<EffectSchema> Hyperion::getEffectSchemas() const
{
	return _effectEngine->getEffectSchemas();
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

	// copy image & process OR copy ledColors from muxer
	Image<ColorRgb> image = priorityInfo.image;
	if (image.width() > 1 || image.height() > 1)
	{
		emit currentImage(image);
		_ledBuffer = _imageProcessor->process(image);
	}
	else
	{
		_ledBuffer = priorityInfo.ledColors;

		if (_ledString.hasBlackListedLeds())
		{
			for (unsigned long const id : _ledString.blacklistedLedIds())
			{
				if (id > _ledBuffer.size()-1)
				{
					break;
				}
				_ledBuffer.at(id) = ColorRgb::BLACK;
			}
		}
	}

	// emit rawLedColors before transform
	emit rawLedColors(_ledBuffer);

	_raw2ledAdjustment->applyAdjustment(_ledBuffer);

	int i = 0;
	for (ColorRgb& color : _ledBuffer)
	{
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

	// fill additional hardware LEDs with black
	if ( _hwLedCount > static_cast<int>(_ledBuffer.size()) )
	{
		_ledBuffer.resize(static_cast<size_t>(_hwLedCount), ColorRgb::BLACK);
	}

	// Write the data to the device
	//if (_ledDeviceWrapper->isEnabled())
	if (_ledDeviceWrapper->isOn())
	{
		// Smoothing is disabled
		if  (! _deviceSmooth->enabled())
		{
			emit ledDeviceData(_ledBuffer);
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
