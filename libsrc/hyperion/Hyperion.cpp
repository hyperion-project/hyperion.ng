// STL includes
#include <exception>
#include <sstream>

// QT includes
#include <QString>
#include <QStringList>
#include <QThread>

// hyperion include
#include <hyperion/Hyperion.h>
#include <hyperion/MessageForwarder.h>
#include <hyperion/ImageProcessor.h>
#include <hyperion/ColorAdjustment.h>

// utils
#include <utils/hyperion.h>
#include <utils/GlobalSignals.h>
#include <utils/Logger.h>

// Leddevice includes
#include <leddevice/LedDeviceWrapper.h>

#include <hyperion/MultiColorAdjustment.h>
#include "LinearColorSmoothing.h"

// effect engine includes
#include <effectengine/EffectEngine.h>

// settingsManagaer
#include <hyperion/SettingsManager.h>

// BGEffectHandler
#include <hyperion/BGEffectHandler.h>

// CaptureControl (Daemon capture)
#include <hyperion/CaptureCont.h>

// Boblight
#include <boblightserver/BoblightServer.h>

Hyperion::Hyperion(quint8 instance)
	: QObject()
	, _instIndex(instance)
	, _settingsManager(new SettingsManager(instance, this))
	, _componentRegister(this)
	, _ledString(hyperion::createLedString(getSetting(settings::LEDS).array(), hyperion::createColorOrder(getSetting(settings::DEVICE).object())))
	, _imageProcessor(new ImageProcessor(_ledString, this))
	, _muxer(_ledString.leds().size(), this)
	, _raw2ledAdjustment(hyperion::createLedColorsAdjustment(_ledString.leds().size(), getSetting(settings::COLOR).object()))
	, _effectEngine(nullptr)
	, _messageForwarder(nullptr)
	, _log(Logger::getInstance("HYPERION"))
	, _hwLedCount()
	, _ledGridSize(hyperion::getLedLayoutGridSize(getSetting(settings::LEDS).array()))
	, _ledBuffer(_ledString.leds().size(), ColorRgb::BLACK)
{

}

Hyperion::~Hyperion()
{
	freeObjects();
}

void Hyperion::start()
{
	// forward settings changed to Hyperion
	connect(_settingsManager, &SettingsManager::settingsChanged, this, &Hyperion::settingsChanged);

	// get newVideoMode from HyperionIManager
	connect(this, &Hyperion::newVideoMode, this, &Hyperion::handleNewVideoMode);

	if (!_raw2ledAdjustment->verifyAdjustments())
	{
		Warning(_log, "At least one led has no color calibration, please add all leds from your led layout to an 'LED index' field!");
	}

	// handle hwLedCount
	_hwLedCount = qMax(unsigned(getSetting(settings::DEVICE).object()["hardwareLedCount"].toInt(getLedCount())), getLedCount());

	// init colororder vector
	for (const Led& led : _ledString.leds())
	{
		_ledStringColorOrder.push_back(led.colorOrder);
	}

	// connect Hyperion::update with Muxer visible priority changes as muxer updates independent
	connect(&_muxer, &PriorityMuxer::visiblePriorityChanged, this, &Hyperion::update);
	connect(&_muxer, &PriorityMuxer::visiblePriorityChanged, this, &Hyperion::handlPriorityChangedLedDevice);
	connect(&_muxer, &PriorityMuxer::visibleComponentChanged, this, &Hyperion::handleVisibleComponentChanged);

	// listens for ComponentRegister changes of COMP_ALL to perform core enable/disable actions
	// connect(&_componentRegister, &ComponentRegister::updatedComponentState, this, &Hyperion::updatedComponentState);

	// listen for settings updates of this instance (LEDS & COLOR)
	connect(_settingsManager, &SettingsManager::settingsChanged, this, &Hyperion::handleSettingsUpdate);

	// set color correction activity state
	const QJsonObject color = getSetting(settings::COLOR).object();

	// initialize leddevices
	QJsonObject ledDevice = getSetting(settings::DEVICE).object();
	ledDevice["currentLedCount"] = int(_hwLedCount); // Inject led count info

	_ledDeviceWrapper = new LedDeviceWrapper(this);
	connect(this, &Hyperion::compStateChangeRequest, _ledDeviceWrapper, &LedDeviceWrapper::handleComponentState);
	connect(this, &Hyperion::ledDeviceData, _ledDeviceWrapper, &LedDeviceWrapper::updateLeds);
	_ledDeviceWrapper->createLedDevice(ledDevice);

	// smoothing
	_deviceSmooth = new LinearColorSmoothing(getSetting(settings::SMOOTHING), this);
	connect(this, &Hyperion::settingsChanged, _deviceSmooth, &LinearColorSmoothing::handleSettingsUpdate);

	// create the message forwarder only on main instance
	if (_instIndex == 0)
		_messageForwarder = new MessageForwarder(this);

	// create the effect engine; needs to be initialized after smoothing!
	_effectEngine = new EffectEngine(this);
	connect(_effectEngine, &EffectEngine::effectListUpdated, this, &Hyperion::effectListUpdated);

	// initial startup effect
	hyperion::handleInitialEffect(this, getSetting(settings::FGEFFECT).object());

	// handle background effect
	_BGEffectHandler = new BGEffectHandler(this);

	// create the Daemon capture interface
	_captureCont = new CaptureCont(this);

	// forwards global signals to the corresponding slots
	connect(GlobalSignals::getInstance(), &GlobalSignals::registerGlobalInput, this, &Hyperion::registerInput);
	connect(GlobalSignals::getInstance(), &GlobalSignals::clearGlobalInput, this, &Hyperion::clear);
	connect(GlobalSignals::getInstance(), &GlobalSignals::setGlobalColor, this, &Hyperion::setColor);
	connect(GlobalSignals::getInstance(), &GlobalSignals::setGlobalImage, this, &Hyperion::setInputImage);

	// if there is no startup / background eff and no sending capture interface we probably want to push once BLACK (as PrioMuxer won't emit a prioritiy change)
	update();

	// boblight, can't live in global scope as it depends on layout
	_boblightServer = new BoblightServer(this, getSetting(settings::BOBLSERVER));
	connect(this, &Hyperion::settingsChanged, _boblightServer, &BoblightServer::handleSettingsUpdate);

	// instance inited, enter thread event loop
	emit started();
}

void Hyperion::stop()
{
	emit finished();
	thread()->wait();
}

void Hyperion::freeObjects()
{
	// switch off all leds
	clear(-1,true);

	// delete components on exit of hyperion core
	delete _boblightServer;
	delete _captureCont;
	delete _effectEngine;
	delete _raw2ledAdjustment;
	delete _messageForwarder;
	delete _settingsManager;
	delete _ledDeviceWrapper;
}

void Hyperion::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
//	std::cout << "Hyperion::handleSettingsUpdate" << std::endl;
//	std::cout << config.toJson().toStdString() << std::endl;

	if(type == settings::COLOR)
	{
		const QJsonObject obj = config.object();
		// change in color recreate ledAdjustments
		delete _raw2ledAdjustment;
		_raw2ledAdjustment = hyperion::createLedColorsAdjustment(_ledString.leds().size(), obj);

		if (!_raw2ledAdjustment->verifyAdjustments())
		{
			Warning(_log, "At least one led has no color calibration, please add all leds from your led layout to an 'LED index' field!");
		}
	}
	else if(type == settings::LEDS)
	{
		const QJsonArray leds = config.array();

		// stop and cache all running effects, as effects depend heavily on ledlayout
		_effectEngine->cacheRunningEffects();

		// ledstring, img processor, muxer, ledGridSize (eff engine image based effects), _ledBuffer and ByteOrder of ledstring
		_ledString = hyperion::createLedString(leds, hyperion::createColorOrder(getSetting(settings::DEVICE).object()));
		_imageProcessor->setLedString(_ledString);
		_muxer.updateLedColorsLength(_ledString.leds().size());
		_ledGridSize = hyperion::getLedLayoutGridSize(leds);

		std::vector<ColorRgb> color(_ledString.leds().size(), ColorRgb{0,0,0});
		_ledBuffer = color;

		_ledStringColorOrder.clear();
		for (const Led& led : _ledString.leds())
		{
			_ledStringColorOrder.push_back(led.colorOrder);
		}

		// handle hwLedCount update
		_hwLedCount = qMax(unsigned(getSetting(settings::DEVICE).object()["hardwareLedCount"].toInt(getLedCount())), getLedCount());

		// change in leds are also reflected in adjustment
		delete _raw2ledAdjustment;
		_raw2ledAdjustment = hyperion::createLedColorsAdjustment(_ledString.leds().size(), getSetting(settings::COLOR).object());

		// start cached effects
		_effectEngine->startCachedEffects();
	}
	else if(type == settings::DEVICE)
	{
		QJsonObject dev = config.object();

		// handle hwLedCount update
		_hwLedCount = qMax(unsigned(dev["hardwareLedCount"].toInt(getLedCount())), getLedCount());

		// force ledString update, if device ByteOrder changed
		if(_ledDeviceWrapper->getColorOrder() != dev["colorOrder"].toString("rgb"))
		{
			_ledString = hyperion::createLedString(getSetting(settings::LEDS).array(), hyperion::createColorOrder(dev));
			_imageProcessor->setLedString(_ledString);

			_ledStringColorOrder.clear();
			for (const Led& led : _ledString.leds())
			{
				_ledStringColorOrder.push_back(led.colorOrder);
			}
		}

		// do always reinit until the led devices can handle dynamic changes
		dev["currentLedCount"] = int(_hwLedCount); // Inject led count info
		_ledDeviceWrapper->createLedDevice(dev);

		// TODO: Check, if framegrabber frequency is lower than latchtime..., if yes, stop
	}
	else if(type == settings::SMOOTHING)
	{
		_deviceSmooth->handleSettingsUpdate( type, config);
	}

	// update once to push single color sets / adjustments/ ledlayout resizes and update ledBuffer color
	update();
}

QJsonDocument Hyperion::getSetting(settings::type type) const
{
	return _settingsManager->getSetting(type);
}

bool Hyperion::saveSettings(const QJsonObject& config, bool correct)
{
	return _settingsManager->saveSettings(config, correct);
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

unsigned Hyperion::getLedCount() const
{
	return _ledString.leds().size();
}

void Hyperion::setSourceAutoSelect(bool state)
{
	_muxer.setSourceAutoSelectEnabled(state);
}

bool Hyperion::setVisiblePriority(int priority)
{
	return _muxer.setPriority(priority);
}

bool Hyperion::sourceAutoSelectEnabled() const
{
	return _muxer.isSourceAutoSelectEnabled();
}

void Hyperion::setNewComponentState(hyperion::Components component, bool state)
{
	_componentRegister.setNewComponentState(component, state);
}

std::map<hyperion::Components, bool> Hyperion::getAllComponents() const
{
	return _componentRegister.getRegister();
}

int Hyperion::isComponentEnabled(hyperion::Components comp) const
{
	return _componentRegister.isComponentEnabled(comp);
}

void Hyperion::registerInput(int priority, hyperion::Components component, const QString& origin, const QString& owner, unsigned smooth_cfg)
{
	_muxer.registerInput(priority, component, origin, owner, smooth_cfg);
}

bool Hyperion::setInput(int priority, const std::vector<ColorRgb>& ledColors, int timeout_ms, bool clearEffect)
{
	if(_muxer.setInput(priority, ledColors, timeout_ms))
	{
		// clear effect if this call does not come from an effect
		if(clearEffect)
			_effectEngine->channelCleared(priority);

		// if this priority is visible, update immediately
		if(priority == _muxer.getCurrentPriority())
		{
			update();
		}

		return true;
	}
	return false;
}

bool Hyperion::setInputImage(int priority, const Image<ColorRgb>& image, int64_t timeout_ms, bool clearEffect)
{
	if (!_muxer.hasPriority(priority))
	{
		emit GlobalSignals::getInstance()->globalRegRequired(priority);
		return false;
	}

	if(_muxer.setInputImage(priority, image, timeout_ms))
	{
		// clear effect if this call does not come from an effect
		if(clearEffect)
			_effectEngine->channelCleared(priority);

		// if this priority is visible, update immediately
		if(priority == _muxer.getCurrentPriority())
		{
			update();
		}

		return true;
	}
	return false;
}

bool Hyperion::setInputInactive(quint8 priority)
{
	return _muxer.setInputInactive(priority);
}

void Hyperion::setColor(int priority, const std::vector<ColorRgb> &ledColors, int timeout_ms, const QString &origin, bool clearEffects)
{
	// clear effect if this call does not come from an effect
	if (clearEffects)
		_effectEngine->channelCleared(priority);

	// create full led vector from single/multiple colors
	size_t size = _ledString.leds().size();
	std::vector<ColorRgb> newLedColors;
	while (true)
	{
		for (const auto &entry : ledColors)
		{
			newLedColors.emplace_back(entry);
			if (newLedColors.size() == size)
				goto end;
		}
	}
end:

	if (getPriorityInfo(priority).componentId != hyperion::COMP_COLOR)
		clear(priority);

	// register color
	registerInput(priority, hyperion::COMP_COLOR, origin);

	// write color to muxer & queuePush
	setInput(priority, newLedColors, timeout_ms);
	if (timeout_ms <= 0)
		_muxer.queuePush();
}

QStringList Hyperion::getAdjustmentIds() const
{
	return _raw2ledAdjustment->getAdjustmentIds();
}

ColorAdjustment * Hyperion::getAdjustment(const QString& id) const
{
	return _raw2ledAdjustment->getAdjustment(id);
}

void Hyperion::adjustmentsUpdated()
{
	emit adjustmentChanged();
	update();
}

bool Hyperion::clear(int priority, bool forceClearAll)
{
	if (priority < 0)
	{
		_muxer.clearAll(forceClearAll);

		// send clearall signal to the effect engine
		_effectEngine->allChannelsCleared();
		return true;
	}
	else
	{
		// send clear signal to the effect engine
		// (outside the check so the effect gets cleared even when the effect is not sending colors)
		_effectEngine->channelCleared(priority);

		if (_muxer.clearInput(priority))
			return true;
	}
	return false;
}

int Hyperion::getCurrentPriority() const
{
	return _muxer.getCurrentPriority();
}

bool Hyperion::isCurrentPriority(int priority) const
{
	return getCurrentPriority() == priority;
}

QList<int> Hyperion::getActivePriorities() const
{
	return _muxer.getPriorities();
}

Hyperion::InputInfo Hyperion::getPriorityInfo(int priority) const
{
	return _muxer.getInputInfo(priority);
}

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

QJsonObject Hyperion::getQJsonConfig() const
{
	return _settingsManager->getSettings();
}

int Hyperion::setEffect(const QString &effectName, int priority, int timeout, const QString & origin)
{
	return _effectEngine->runEffect(effectName, priority, timeout, origin);
}

int Hyperion::setEffect(const QString &effectName, const QJsonObject &args, int priority, int timeout, const QString &pythonScript, const QString &origin, const QString &imageData)
{
	return _effectEngine->runEffect(effectName, args, priority, timeout, pythonScript, origin, 0, imageData);
}

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

void Hyperion::handlPriorityChangedLedDevice(const quint8& priority)
{
	quint8 previousPriority = _muxer.getPreviousPriority();

	Debug(_log,"priority[%u], previousPriority[%u]", priority, previousPriority);
	if ( priority == PriorityMuxer::LOWEST_PRIORITY)
	{
		Debug(_log,"No source left -> switch LED-Device off");
		emit _ledDeviceWrapper->switchOff();
	}
	else
	{
		if ( previousPriority == PriorityMuxer::LOWEST_PRIORITY )
		{
			Debug(_log,"new source available -> switch LED-Device on");
			emit _ledDeviceWrapper->switchOn();
		}
	}
}

void Hyperion::update()
{
	// Obtain the current priority channel
	int priority = _muxer.getCurrentPriority();
	const PriorityMuxer::InputInfo priorityInfo = _muxer.getInputInfo(priority);

	// copy image & process OR copy ledColors from muxer
	Image<ColorRgb> image = priorityInfo.image;
	if(image.size() > 3)
	{
		emit currentImage(image);
		_ledBuffer = _imageProcessor->process(image);
	}
	else
	{
		_ledBuffer = priorityInfo.ledColors;
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
		i++;
	}

	// fill additional hw leds with black
	if ( _hwLedCount > _ledBuffer.size() )
	{
		_ledBuffer.resize(_hwLedCount, ColorRgb::BLACK);
	}

	// Write the data to the device
	if (_ledDeviceWrapper->enabled())
	{
		// Smoothing is disabled
		if  (! _deviceSmooth->enabled())
		{
			//std::cout << "Hyperion::update()> Non-Smoothing - "; LedDevice::printLedValues ( _ledBuffer);
			emit ledDeviceData(_ledBuffer);
		}
		else
		{
			_deviceSmooth->selectConfig(priorityInfo.smooth_cfg);

			// feed smoothing in pause mode to maintain a smooth transistion back to smooth mode
			if (_deviceSmooth->enabled() || _deviceSmooth->pause())
			{
				_deviceSmooth->updateLedValues(_ledBuffer);
			}
		}
	}
	//else
	//{
	//	/LEDDevice is disabled
	//	Debug(_log, "LEDDevice is disabled - no update required");
	//}
}
