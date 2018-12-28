
// STL includes
#include <exception>
#include <sstream>
#include <unistd.h>

// QT includes
#include <QDateTime>
#include <QThread>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QCryptographicHash>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QHostInfo>

// hyperion include
#include <hyperion/Hyperion.h>
#include <hyperion/MessageForwarder.h>
#include <hyperion/ImageProcessor.h>
#include <hyperion/ColorAdjustment.h>

// utils
#include <utils/hyperion.h>

// Leddevice includes
#include <leddevice/LedDevice.h>
#include <leddevice/LedDeviceFactory.h>

#include <hyperion/MultiColorAdjustment.h>
#include "LinearColorSmoothing.h"

// effect engine includes
#include <effectengine/EffectEngine.h>

// Hyperion Daemon
#include <../src/hyperiond/hyperiond.h>

// settingsManagaer
#include <hyperion/SettingsManager.h>

// BGEffectHandler
#include <hyperion/BGEffectHandler.h>

// CaptureControl (Daemon capture)
#include <hyperion/CaptureCont.h>

// Boblight
#include <boblightserver/BoblightServer.h>

Hyperion* Hyperion::_hyperion = nullptr;

Hyperion* Hyperion::initInstance( HyperionDaemon* daemon, const quint8& instance, const QString configFile, const QString rootPath)
{
	if ( Hyperion::_hyperion != nullptr )
		throw std::runtime_error("Hyperion::initInstance can be called only one time");
	Hyperion::_hyperion = new Hyperion(daemon, instance, configFile, rootPath);

	return Hyperion::_hyperion;
}

Hyperion* Hyperion::getInstance()
{
	if ( Hyperion::_hyperion == nullptr )
		throw std::runtime_error("Hyperion::getInstance used without call of Hyperion::initInstance before");

	return Hyperion::_hyperion;
}

MessageForwarder * Hyperion::getForwarder()
{
	return _messageForwarder;
}

Hyperion::Hyperion(HyperionDaemon* daemon, const quint8& instance, const QString configFile, const QString rootPath)
	: _daemon(daemon)
	, _settingsManager(new SettingsManager(this, instance, configFile))
	, _componentRegister(this)
	, _ledString(hyperion::createLedString(getSetting(settings::LEDS).array(), hyperion::createColorOrder(getSetting(settings::DEVICE).object())))
	, _ledStringClone(hyperion::createLedStringClone(getSetting(settings::LEDS).array(), hyperion::createColorOrder(getSetting(settings::DEVICE).object())))
	, _imageProcessor(new ImageProcessor(_ledString, this))
	, _muxer(_ledString.leds().size())
	, _raw2ledAdjustment(hyperion::createLedColorsAdjustment(_ledString.leds().size(), getSetting(settings::COLOR).object()))
	, _effectEngine(nullptr)
	, _messageForwarder(new MessageForwarder(this, getSetting(settings::NETFORWARD)))
	, _configFile(configFile)
	, _rootPath(rootPath)
	, _log(Logger::getInstance("HYPERION"))
	, _hwLedCount()
	, _configHash()
	, _ledGridSize(hyperion::getLedLayoutGridSize(getSetting(settings::LEDS).array()))
	, _prevCompId(hyperion::COMP_INVALID)
	, _ledBuffer(_ledString.leds().size(), ColorRgb::BLACK)
{
	if (!_raw2ledAdjustment->verifyAdjustments())
	{
		Warning(_log, "At least one led has no color calibration, please add all leds from your led layout to an 'LED index' field!");
	}

	// handle hwLedCount
	_hwLedCount = qMax(unsigned(getSetting(settings::DEVICE).object()["hardwareLedCount"].toInt(getLedCount())), getLedCount());

	// init colororder vector
	for (Led& led : _ledString.leds())
	{
		_ledStringColorOrder.push_back(led.colorOrder);
	}
	for (Led& led : _ledStringClone.leds())
	{
		_ledStringColorOrder.insert(_ledStringColorOrder.begin() + led.index, led.colorOrder);
	}

	// connect Hyperion::update with Muxer visible priority changes as muxer updates independent
	connect(&_muxer, &PriorityMuxer::visiblePriorityChanged, this, &Hyperion::update);

	// listens for ComponentRegister changes of COMP_ALL to perform core enable/disable actions
	connect(&_componentRegister, &ComponentRegister::updatedComponentState, this, &Hyperion::updatedComponentState);

	// listen for settings updates of this instance (LEDS & COLOR)
	connect(_settingsManager, &SettingsManager::settingsChanged, this, &Hyperion::handleSettingsUpdate);

	// set color correction activity state
	const QJsonObject color = getSetting(settings::COLOR).object();

	// initialize leddevices
	QJsonObject ledDevice = getSetting(settings::DEVICE).object();
	ledDevice["currentLedCount"] = int(_hwLedCount); // Inject led count info

	_device       = LedDeviceFactory::construct(ledDevice);
	_deviceSmooth = new LinearColorSmoothing(_device, getSetting(settings::SMOOTHING), this);
	connect(this, &Hyperion::settingsChanged, _deviceSmooth, &LinearColorSmoothing::handleSettingsUpdate);

	getComponentRegister().componentStateChanged(hyperion::COMP_LEDDEVICE, _device->componentState());

	// create the effect engine and pipe the updateEmit; must be initialized after smoothing!
	_effectEngine = new EffectEngine(this,getSetting(settings::EFFECTS).object());
	connect(_effectEngine, &EffectEngine::effectListUpdated, this, &Hyperion::effectListUpdated);

	// setup config state checks and initial shot
	checkConfigState();
	if(_fsWatcher.addPath(_configFile))
	{
		QObject::connect(&_fsWatcher, &QFileSystemWatcher::fileChanged, this, &Hyperion::checkConfigState);
	}
	else
	{
		_cTimer = new QTimer(this);
		Warning(_log,"Filesystem Observer failed for file: %s, use fallback timer", _configFile.toStdString().c_str());
		connect(_cTimer, SIGNAL(timeout()), this, SLOT(checkConfigState()));
		_cTimer->start(2000);
	}

	// initial startup effect
	hyperion::handleInitialEffect(this, getSetting(settings::FGEFFECT).object());
	// handle background effect
	_BGEffectHandler = new BGEffectHandler(this);

	// create the Daemon capture interface
	_captureCont = new CaptureCont(this);

	// if there is no startup / background eff and no sending capture interface we probably want to push once BLACK (as PrioMuxer won't emit a prioritiy change)
	update();

	// boblight, can't live in global scope as it depends on layout

	_boblightServer = new BoblightServer(this, getSetting(settings::BOBLSERVER));
	connect(this, &Hyperion::settingsChanged, _boblightServer, &BoblightServer::handleSettingsUpdate);
}

Hyperion::~Hyperion()
{
	freeObjects(false);
}

void Hyperion::freeObjects(bool emitCloseSignal)
{
	// switch off all leds
	clearall(true);
	_device->switchOff();

	if (emitCloseSignal)
	{
		emit closing();
	}

	// delete components on exit of hyperion core
	delete _boblightServer;
	delete _captureCont;
	delete _effectEngine;
	//delete _deviceSmooth;
	delete _device;
	delete _raw2ledAdjustment;
	delete _messageForwarder;
	delete _settingsManager;
}

void Hyperion::handleSettingsUpdate(const settings::type& type, const QJsonDocument& config)
{
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

		// lock update()
		_lockUpdate = true;
		// stop and cache all running effects, as effects depend heavily on ledlayout
		_effectEngine->cacheRunningEffects();

		// ledstring, clone, img processor, muxer, ledGridSize (eff engine image based effects), _ledBuffer and ByteOrder of ledstring
		_ledString = hyperion::createLedString(leds, hyperion::createColorOrder(getSetting(settings::DEVICE).object()));
		_ledStringClone = hyperion::createLedStringClone(leds, hyperion::createColorOrder(getSetting(settings::DEVICE).object()));
		_imageProcessor->setLedString(_ledString);
		_muxer.updateLedColorsLength(_ledString.leds().size());
		_ledGridSize = hyperion::getLedLayoutGridSize(leds);

		std::vector<ColorRgb> color(_ledString.leds().size(), ColorRgb{0,0,0});
		_ledBuffer = color;

		_ledStringColorOrder.clear();
		for (Led& led : _ledString.leds())
		{
			_ledStringColorOrder.push_back(led.colorOrder);
		}
		for (Led& led : _ledStringClone.leds())
		{
			_ledStringColorOrder.insert(_ledStringColorOrder.begin() + led.index, led.colorOrder);
		}

		// handle hwLedCount update
		_hwLedCount = qMax(unsigned(getSetting(settings::DEVICE).object()["hardwareLedCount"].toInt(getLedCount())), getLedCount());

		// update led count in device
		_device->setLedCount(_hwLedCount);

		// change in leds are also reflected in adjustment
		delete _raw2ledAdjustment;
		_raw2ledAdjustment = hyperion::createLedColorsAdjustment(_ledString.leds().size(), getSetting(settings::COLOR).object());

		// start cached effects
		_effectEngine->startCachedEffects();

		// unlock
		_lockUpdate = false;
	}
	else if(type == settings::DEVICE)
	{
		_lockUpdate = true;
		QJsonObject dev = config.object();

		// handle hwLedCount update
		_hwLedCount = qMax(unsigned(dev["hardwareLedCount"].toInt(getLedCount())), getLedCount());

		// force ledString update, if device ByteOrder changed
		if(_device->getColorOrder() != dev["colorOrder"].toString("rgb"))
		{
			_ledString = hyperion::createLedString(getSetting(settings::LEDS).array(), hyperion::createColorOrder(dev));
			_ledStringClone = hyperion::createLedStringClone(getSetting(settings::LEDS).array(), hyperion::createColorOrder(dev));
			_imageProcessor->setLedString(_ledString);
		}

	/*	// reinit led device type on change
		if(_device->getActiveDevice() != dev["type"].toString("file").toLower())
		{
		}
		// update led count
		_device->setLedCount(_hwLedCount);
	*/
		// do always reinit
		// TODO segfaulting in LinearColorSmoothing::queueColor triggert from QTimer because of device->setLEdValues (results from gdb debugging and testing)
		bool wasEnabled = _deviceSmooth->enabled();
		_deviceSmooth->stopTimer();
		delete _device;
		dev["currentLedCount"] = int(_hwLedCount); // Inject led count info
		_device = LedDeviceFactory::construct(dev);
		getComponentRegister().componentStateChanged(hyperion::COMP_LEDDEVICE, _device->componentState());
		if(wasEnabled)
			_deviceSmooth->startTimerDelayed();
		_lockUpdate = false;
	}
	// update once to push single color sets / adjustments/ ledlayout resizes and update ledBuffer color
	update();
}

QJsonDocument Hyperion::getSetting(const settings::type& type)
{
	return _settingsManager->getSetting(type);
}

bool Hyperion::saveSettings(QJsonObject config, const bool& correct)
{
	return _settingsManager->saveSettings(config, correct);
}

QString Hyperion::getConfigFileName() const
{
	QFileInfo cF(_configFile);
	return cF.fileName();
}

int Hyperion::getLatchTime() const
{
  return _device->getLatchTime();
}

unsigned Hyperion::addSmoothingConfig(int settlingTime_ms, double ledUpdateFrequency_hz, unsigned updateDelay)
{
	return _deviceSmooth->addConfig(settlingTime_ms, ledUpdateFrequency_hz, updateDelay);
}

unsigned Hyperion::getLedCount() const
{
	return _ledString.leds().size();
}

void Hyperion::checkConfigState(QString cfile)
{
	// Check config modifications
	QFile f(_configFile);
	if (f.open(QFile::ReadOnly))
	{
		QCryptographicHash hash(QCryptographicHash::Sha1);
		if (hash.addData(&f))
		{
			if (_configHash.size() == 0)
			{
				_configHash = hash.result();
			}
			_configMod = _configHash != hash.result() ? true : false;
		}
	}
	f.close();

	if(_prevConfigMod != _configMod)
	{
		_prevConfigMod = _configMod;
	}

	// Check config writeable
	QFile file(_configFile);
	QFileInfo fileInfo(file);
	_configWrite = fileInfo.isWritable() && fileInfo.isReadable() ? true : false;

	if(_prevConfigWrite != _configWrite)
	{
		_prevConfigWrite = _configWrite;
	}
}

void Hyperion::setSourceAutoSelectEnabled(bool enabled)
{
	if(_muxer.setSourceAutoSelectEnabled(enabled))
		update();
}

bool Hyperion::setCurrentSourcePriority(int priority )
{
	return _muxer.setPriority(priority);
}

bool Hyperion::sourceAutoSelectEnabled()
{
	return _muxer.isSourceAutoSelectEnabled();
}

void Hyperion::setComponentState(const hyperion::Components component, const bool state)
{
	switch (component)
	{
		case hyperion::COMP_LEDDEVICE:
			_device->setEnable(state);
			getComponentRegister().componentStateChanged(hyperion::COMP_LEDDEVICE, _device->componentState());
			break;

		default:
			emit componentStateChanged(component, state);
	}
}

void Hyperion::registerInput(const int priority, const hyperion::Components& component, const QString& origin, const QString& owner, unsigned smooth_cfg)
{
	_muxer.registerInput(priority, component, origin, owner, smooth_cfg);
}

const bool Hyperion::setInput(const int priority, const std::vector<ColorRgb>& ledColors, int timeout_ms, const bool& clearEffect)
{
	if(_muxer.setInput(priority, ledColors, timeout_ms))
	{
		// clear effect if this call does not come from an effect
		if(clearEffect)
			_effectEngine->channelCleared(priority);

		// if this priority is visible, update immediately
		if(priority == _muxer.getCurrentPriority())
			update();

		return true;
	}
	return false;
}

const bool Hyperion::setInputImage(const int priority, const Image<ColorRgb>& image, int64_t timeout_ms, const bool& clearEffect)
{
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

const bool Hyperion::setInputInactive(const quint8& priority)
{
	return _muxer.setInputInactive(priority);
}

void Hyperion::setColor(int priority, const ColorRgb &color, const int timeout_ms, const QString& origin, bool clearEffects)
{
	// clear effect if this call does not come from an effect
	if(clearEffects)
		_effectEngine->channelCleared(priority);

	// create led vector from single color
	std::vector<ColorRgb> ledColors(_ledString.leds().size(), color);

	// register color
	registerInput(priority, hyperion::COMP_COLOR, origin);

	// write color to muxer
	setInput(priority, ledColors, timeout_ms);
}

const QStringList & Hyperion::getAdjustmentIds() const
{
	return _raw2ledAdjustment->getAdjustmentIds();
}

ColorAdjustment * Hyperion::getAdjustment(const QString& id)
{
	return _raw2ledAdjustment->getAdjustment(id);
}

void Hyperion::adjustmentsUpdated()
{
	emit adjustmentChanged();
	update();
}

const bool Hyperion::clear(int priority)
{
	// send clear signal to the effect engine
	// (outside the check so the effect gets cleared even when the effect is not sending colors)
	_effectEngine->channelCleared(priority);

	if(_muxer.clearInput(priority))
		return true;

	return false;
}

void Hyperion::clearall(bool forceClearAll)
{
	_muxer.clearAll(forceClearAll);

	// send clearall signal to the effect engine
	_effectEngine->allChannelsCleared();
}

int Hyperion::getCurrentPriority() const
{
	return _muxer.getCurrentPriority();
}

bool Hyperion::isCurrentPriority(const int priority) const
{
	return getCurrentPriority() == priority;
}

QList<int> Hyperion::getActivePriorities() const
{
	return _muxer.getPriorities();
}

const Hyperion::InputInfo Hyperion::getPriorityInfo(const int priority) const
{
	return _muxer.getInputInfo(priority);
}

void Hyperion::reloadEffects()
{
	_effectEngine->readEffects();
}

const std::list<EffectDefinition> & Hyperion::getEffects() const
{
	return _effectEngine->getEffects();
}

const std::list<ActiveEffectDefinition> & Hyperion::getActiveEffects()
{
	return _effectEngine->getActiveEffects();
}

const std::list<EffectSchema> & Hyperion::getEffectSchemas()
{
	return _effectEngine->getEffectSchemas();
}

const QJsonObject& Hyperion::getQJsonConfig()
{
	return _settingsManager->getSettings();
}

int Hyperion::setEffect(const QString &effectName, int priority, int timeout, const QString & origin)
{
	return _effectEngine->runEffect(effectName, priority, timeout, origin);
}

int Hyperion::setEffect(const QString &effectName, const QJsonObject &args, int priority, int timeout, const QString & pythonScript, const QString & origin)
{
	return _effectEngine->runEffect(effectName, args, priority, timeout, pythonScript, origin);
}

void Hyperion::setLedMappingType(const int& mappingType)
{
	if(mappingType != _imageProcessor->getUserLedMappingType())
	{
		_imageProcessor->setLedMappingType(mappingType);
		emit imageToLedsMappingChanged(mappingType);
	}
}

const int & Hyperion::getLedMappingType()
{
	return _imageProcessor->getUserLedMappingType();
}

void Hyperion::setVideoMode(const VideoMode& mode)
{
	emit videoMode(mode);
}

const VideoMode & Hyperion::getCurrentVideoMode()
{
	return _daemon->getVideoMode();
}

const QString & Hyperion::getActiveDevice()
{
	return _device->getActiveDevice();
}

void Hyperion::updatedComponentState(const hyperion::Components comp, const bool state)
{
	if(comp == hyperion::COMP_ALL)
	{
		if(state)
		{
			// first muxer to update all inputs
			_muxer.setEnable(state);
		}
		else
		{
			_muxer.setEnable(state);
		}
	}
}

void Hyperion::update()
{
	if(_lockUpdate)
		return;

	// the ledbuffer resize for hwledcount needs to be reverted
	if(_hwLedCount > _ledBuffer.size())
		_ledBuffer.resize(getLedCount());

	// Obtain the current priority channel
	int priority = _muxer.getCurrentPriority();
	const PriorityMuxer::InputInfo priorityInfo = _muxer.getInputInfo(priority);

	// eval comp change
	bool compChanged = false;
	if (priorityInfo.componentId != _prevCompId)
	{
		compChanged = true;
		_prevCompId = priorityInfo.componentId;
	}

	// copy image & process OR copy ledColors from muxer
	Image<ColorRgb> image = priorityInfo.image;
	if(image.size() > 3)
	{
		emit currentImage(image);
		// disable the black border detector for effects and ledmapping to 0
		if(compChanged)
		{
			_imageProcessor->setBlackbarDetectDisable((_prevCompId == hyperion::COMP_EFFECT));
			_imageProcessor->setHardLedMappingType((_prevCompId == hyperion::COMP_EFFECT) ? 0 : -1);
		}
		_imageProcessor->process(image, _ledBuffer);
	}
	else
	{
		_ledBuffer = priorityInfo.ledColors;
	}
	// copy rawLedColors before adjustments
	_rawLedBuffer = _ledBuffer;

	// apply adjustments
	if(compChanged)
		_raw2ledAdjustment->setBacklightEnabled((_prevCompId != hyperion::COMP_COLOR && _prevCompId != hyperion::COMP_EFFECT));
	_raw2ledAdjustment->applyAdjustment(_ledBuffer);

	// insert cloned leds into buffer
	for (Led& led : _ledStringClone.leds())
	{
		_ledBuffer.insert(_ledBuffer.begin() + led.index, _ledBuffer.at(led.clone));
	}

	int i = 0;
	for (ColorRgb& color : _ledBuffer)
	{
		// correct the color byte order
		switch (_ledStringColorOrder.at(i))
		{
		case ORDER_RGB:
			// leave as it is
			break;
		case ORDER_BGR:
			std::swap(color.red, color.blue);
			break;
		case ORDER_RBG:
			std::swap(color.green, color.blue);
			break;
		case ORDER_GRB:
			std::swap(color.red, color.green);
			break;
		case ORDER_GBR:
			std::swap(color.red, color.green);
			std::swap(color.green, color.blue);
			break;

		case ORDER_BRG:
			std::swap(color.red, color.blue);
			std::swap(color.green, color.blue);
			break;
		}
		i++;
	}
	// fill aditional hw leds with black
	if ( _hwLedCount > _ledBuffer.size() )
	{
		_ledBuffer.resize(_hwLedCount, ColorRgb::BLACK);
	}

	// Write the data to the device
	if (_device->enabled())
	{
		_deviceSmooth->selectConfig(priorityInfo.smooth_cfg);

		// feed smoothing in pause mode to maintain a smooth transistion back to smoth mode
		if (_deviceSmooth->enabled() || _deviceSmooth->pause())
			_deviceSmooth->setLedValues(_ledBuffer);

		if  (! _deviceSmooth->enabled())
			_device->setLedValues(_ledBuffer);
	}
}
