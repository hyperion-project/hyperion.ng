
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
#include <QFile>
#include <QFileInfo>
#include <QHostInfo>

// hyperion include
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>
#include <hyperion/ColorAdjustment.h>

// Leddevice includes
#include <leddevice/LedDevice.h>
#include <leddevice/LedDeviceFactory.h>

#include "MultiColorAdjustment.h"
#include "LinearColorSmoothing.h"

// effect engine includes
#include <effectengine/EffectEngine.h>

#define CORE_LOGGER Logger::getInstance("Core")

Hyperion* Hyperion::_hyperion = nullptr;

Hyperion* Hyperion::initInstance(const QJsonObject& qjsonConfig, const QString configFile, const QString rootPath)
{
	if ( Hyperion::_hyperion != nullptr )
		throw std::runtime_error("Hyperion::initInstance can be called only one time");
	Hyperion::_hyperion = new Hyperion(qjsonConfig, configFile, rootPath);

	return Hyperion::_hyperion;
}

Hyperion* Hyperion::getInstance()
{
	if ( Hyperion::_hyperion == nullptr )
		throw std::runtime_error("Hyperion::getInstance used without call of Hyperion::initInstance before");

	return Hyperion::_hyperion;
}

ColorOrder Hyperion::createColorOrder(const QJsonObject &deviceConfig)
{
	return stringToColorOrder(deviceConfig["colorOrder"].toString("rgb"));
}

ColorAdjustment * Hyperion::createColorAdjustment(const QJsonObject & adjustmentConfig)
{
	const QString id = adjustmentConfig["id"].toString("default");

	RgbChannelAdjustment * blackAdjustment   = createRgbChannelAdjustment(adjustmentConfig, "black"  ,   0,  0,  0);
	RgbChannelAdjustment * whiteAdjustment   = createRgbChannelAdjustment(adjustmentConfig, "white"  , 255,255,255);
	RgbChannelAdjustment * redAdjustment     = createRgbChannelAdjustment(adjustmentConfig, "red"    , 255,  0,  0);
	RgbChannelAdjustment * greenAdjustment   = createRgbChannelAdjustment(adjustmentConfig, "green"  ,   0,255,  0);
	RgbChannelAdjustment * blueAdjustment    = createRgbChannelAdjustment(adjustmentConfig, "blue"   ,   0,  0,255);
	RgbChannelAdjustment * cyanAdjustment    = createRgbChannelAdjustment(adjustmentConfig, "cyan"   ,   0,255,255);
	RgbChannelAdjustment * magentaAdjustment = createRgbChannelAdjustment(adjustmentConfig, "magenta", 255,  0,255);
	RgbChannelAdjustment * yellowAdjustment  = createRgbChannelAdjustment(adjustmentConfig, "yellow" , 255,255,  0);
	RgbTransform         * rgbTransform      = createRgbTransform(adjustmentConfig);

	ColorAdjustment * adjustment = new ColorAdjustment();
	adjustment->_id = id;
	adjustment->_rgbBlackAdjustment   = *blackAdjustment;
	adjustment->_rgbWhiteAdjustment   = *whiteAdjustment;
	adjustment->_rgbRedAdjustment     = *redAdjustment;
	adjustment->_rgbGreenAdjustment   = *greenAdjustment;
	adjustment->_rgbBlueAdjustment    = *blueAdjustment;
	adjustment->_rgbCyanAdjustment    = *cyanAdjustment;
	adjustment->_rgbMagentaAdjustment = *magentaAdjustment;
	adjustment->_rgbYellowAdjustment  = *yellowAdjustment;
	adjustment->_rgbTransform         = *rgbTransform;

	// Cleanup the allocated individual adjustments
	delete blackAdjustment;
	delete whiteAdjustment;
	delete redAdjustment;
	delete greenAdjustment;
	delete blueAdjustment;
	delete cyanAdjustment;
	delete magentaAdjustment;
	delete yellowAdjustment;
	delete rgbTransform;

	return adjustment;
}


MultiColorAdjustment * Hyperion::createLedColorsAdjustment(const unsigned ledCnt, const QJsonObject & colorConfig)
{
	// Create the result, the transforms are added to this
	MultiColorAdjustment * adjustment = new MultiColorAdjustment(ledCnt);

	const QJsonValue adjustmentConfig = colorConfig["channelAdjustment"];
	const QRegExp overallExp("([0-9]+(\\-[0-9]+)?)(,[ ]*([0-9]+(\\-[0-9]+)?))*");

	const QJsonArray & adjustmentConfigArray = adjustmentConfig.toArray();
	for (signed i = 0; i < adjustmentConfigArray.size(); ++i)
	{
		const QJsonObject & config = adjustmentConfigArray.at(i).toObject();
		ColorAdjustment * colorAdjustment = createColorAdjustment(config);
		adjustment->addAdjustment(colorAdjustment);

		const QString ledIndicesStr = config["leds"].toString("").trimmed();
		if (ledIndicesStr.compare("*") == 0)
		{
			// Special case for indices '*' => all leds
			adjustment->setAdjustmentForLed(colorAdjustment->_id, 0, ledCnt-1);
			Info(CORE_LOGGER, "ColorAdjustment '%s' => [0; %d]", QSTRING_CSTR(colorAdjustment->_id), ledCnt-1);
			continue;
		}

		if (!overallExp.exactMatch(ledIndicesStr))
		{
			Error(CORE_LOGGER, "Given led indices %d not correct format: %s", i, QSTRING_CSTR(ledIndicesStr));
			continue;
		}

		std::stringstream ss;
		const QStringList ledIndexList = ledIndicesStr.split(",");
		for (int i=0; i<ledIndexList.size(); ++i) {
			if (i > 0)
			{
				ss << ", ";
			}
			if (ledIndexList[i].contains("-"))
			{
				QStringList ledIndices = ledIndexList[i].split("-");
				int startInd = ledIndices[0].toInt();
				int endInd   = ledIndices[1].toInt();

				adjustment->setAdjustmentForLed(colorAdjustment->_id, startInd, endInd);
				ss << startInd << "-" << endInd;
			}
			else
			{
				int index = ledIndexList[i].toInt();
				adjustment->setAdjustmentForLed(colorAdjustment->_id, index, index);
				ss << index;
			}
		}
		Info(CORE_LOGGER, "ColorAdjustment '%s' => [%s]", QSTRING_CSTR(colorAdjustment->_id), ss.str().c_str());
	}

	return adjustment;
}

RgbTransform* Hyperion::createRgbTransform(const QJsonObject& colorConfig)
{
	const double backlightThreshold = colorConfig["backlightThreshold"].toDouble(0.0);
	const bool   backlightColored   = colorConfig["backlightColored"].toBool(false);
	const double brightness    = colorConfig["brightness"].toInt(100);
	const double brightnessComp= colorConfig["brightnessCompensation"].toInt(100);
	const double gammaR        = colorConfig["gammaRed"].toDouble(1.0);
	const double gammaG        = colorConfig["gammaGreen"].toDouble(1.0);
	const double gammaB        = colorConfig["gammaBlue"].toDouble(1.0);

	RgbTransform* transform = new RgbTransform(gammaR, gammaG, gammaB, backlightThreshold, backlightColored, brightness, brightnessComp);
	return transform;
}

RgbChannelAdjustment* Hyperion::createRgbChannelAdjustment(const QJsonObject& colorConfig, const QString channelName, const int defaultR, const int defaultG, const int defaultB)
{
	const QJsonArray& channelConfig  = colorConfig[channelName].toArray();
	RgbChannelAdjustment* adjustment =  new RgbChannelAdjustment(
		channelConfig[0].toInt(defaultR),
		channelConfig[1].toInt(defaultG),
		channelConfig[2].toInt(defaultB),
		"ChannelAdjust_"+channelName.toUpper());
	return adjustment;
}

LedString Hyperion::createLedString(const QJsonValue& ledsConfig, const ColorOrder deviceOrder)
{
	LedString ledString;
	const QString deviceOrderStr = colorOrderToString(deviceOrder);
	const QJsonArray & ledConfigArray = ledsConfig.toArray();
	int maxLedId = ledConfigArray.size();

	for (signed i = 0; i < ledConfigArray.size(); ++i)
	{
		const QJsonObject& index = ledConfigArray[i].toObject();

		Led led;
		led.index = index["index"].toInt();
		led.clone = index["clone"].toInt(-1);
		if ( led.clone < -1 || led.clone >= maxLedId )
		{
			Warning(CORE_LOGGER, "LED %d: clone index of %d is out of range, clone ignored", led.index, led.clone);
			led.clone = -1;
		}

		if ( led.clone < 0 )
		{
			const QJsonObject& hscanConfig = ledConfigArray[i].toObject()["hscan"].toObject();
			const QJsonObject& vscanConfig = ledConfigArray[i].toObject()["vscan"].toObject();
			led.minX_frac = qMax(0.0, qMin(1.0, hscanConfig["minimum"].toDouble()));
			led.maxX_frac = qMax(0.0, qMin(1.0, hscanConfig["maximum"].toDouble()));
			led.minY_frac = qMax(0.0, qMin(1.0, vscanConfig["minimum"].toDouble()));
			led.maxY_frac = qMax(0.0, qMin(1.0, vscanConfig["maximum"].toDouble()));
			// Fix if the user swapped min and max
			if (led.minX_frac > led.maxX_frac)
			{
				std::swap(led.minX_frac, led.maxX_frac);
			}
			if (led.minY_frac > led.maxY_frac)
			{
				std::swap(led.minY_frac, led.maxY_frac);
			}

			// Get the order of the rgb channels for this led (default is device order)
			led.colorOrder = stringToColorOrder(index["colorOrder"].toString(deviceOrderStr));
			ledString.leds().push_back(led);
		}
	}

	// Make sure the leds are sorted (on their indices)
	std::sort(ledString.leds().begin(), ledString.leds().end(), [](const Led& lhs, const Led& rhs){ return lhs.index < rhs.index; });
	return ledString;
}

LedString Hyperion::createLedStringClone(const QJsonValue& ledsConfig, const ColorOrder deviceOrder)
{
	LedString ledString;
	const QString deviceOrderStr = colorOrderToString(deviceOrder);
	const QJsonArray & ledConfigArray = ledsConfig.toArray();
	int maxLedId = ledConfigArray.size();

	for (signed i = 0; i < ledConfigArray.size(); ++i)
	{
		const QJsonObject& index = ledConfigArray[i].toObject();

		Led led;
		led.index = index["index"].toInt();
		led.clone = index["clone"].toInt(-1);
		if ( led.clone < -1 || led.clone >= maxLedId )
		{
			Warning(CORE_LOGGER, "LED %d: clone index of %d is out of range, clone ignored", led.index, led.clone);
			led.clone = -1;
		}

		if ( led.clone >= 0 )
		{
			Debug(CORE_LOGGER, "LED %d: clone from led %d", led.index, led.clone);
			led.minX_frac = 0;
			led.maxX_frac = 0;
			led.minY_frac = 0;
			led.maxY_frac = 0;
			// Get the order of the rgb channels for this led (default is device order)
			led.colorOrder = stringToColorOrder(index["colorOrder"].toString(deviceOrderStr));

			ledString.leds().push_back(led);
		}

	}

	// Make sure the leds are sorted (on their indices)
	std::sort(ledString.leds().begin(), ledString.leds().end(), [](const Led& lhs, const Led& rhs){ return lhs.index < rhs.index; });
	return ledString;
}

QSize Hyperion::getLedLayoutGridSize(const QJsonValue& ledsConfig)
{
	std::vector<int> midPointsX;
	std::vector<int> midPointsY;
	const QJsonArray & ledConfigArray = ledsConfig.toArray();

	for (signed i = 0; i < ledConfigArray.size(); ++i)
	{
		const QJsonObject& index = ledConfigArray[i].toObject();

		if (index["clone"].toInt(-1) < 0 )
		{
			const QJsonObject& hscanConfig = ledConfigArray[i].toObject()["hscan"].toObject();
			const QJsonObject& vscanConfig = ledConfigArray[i].toObject()["vscan"].toObject();
			double minX_frac = qMax(0.0, qMin(1.0, hscanConfig["minimum"].toDouble()));
			double maxX_frac = qMax(0.0, qMin(1.0, hscanConfig["maximum"].toDouble()));
			double minY_frac = qMax(0.0, qMin(1.0, vscanConfig["minimum"].toDouble()));
			double maxY_frac = qMax(0.0, qMin(1.0, vscanConfig["maximum"].toDouble()));
			// Fix if the user swapped min and max
			if (minX_frac > maxX_frac)
			{
				std::swap(minX_frac, maxX_frac);
			}
			if (minY_frac > maxY_frac)
			{
				std::swap(minY_frac, maxY_frac);
			}

			// calculate mid point and make grid calculation
			midPointsX.push_back( int(1000.0*(minX_frac + maxX_frac) / 2.0) );
			midPointsY.push_back( int(1000.0*(minY_frac + maxY_frac) / 2.0) );
		}
	}

	// remove duplicates
	std::sort(midPointsX.begin(), midPointsX.end());
	midPointsX.erase(std::unique(midPointsX.begin(), midPointsX.end()), midPointsX.end());
	std::sort(midPointsY.begin(), midPointsY.end());
	midPointsY.erase(std::unique(midPointsY.begin(), midPointsY.end()), midPointsY.end());

	QSize gridSize( midPointsX.size(), midPointsY.size() );
	Debug(CORE_LOGGER, "led layout grid: %dx%d", gridSize.width(), gridSize.height());

	return gridSize;
}



LinearColorSmoothing * Hyperion::createColorSmoothing(const QJsonObject & smoothingConfig, LedDevice* leddevice){
	QString type = smoothingConfig["type"].toString("linear").toLower();
	LinearColorSmoothing * device = nullptr;
	type = "linear"; // TODO currently hardcoded type, delete it if we have more types

	if (type == "linear")
	{
		Info( CORE_LOGGER, "Creating linear smoothing");
		device = new LinearColorSmoothing(
		            leddevice,
		            smoothingConfig["updateFrequency"].toDouble(25.0),
		            smoothingConfig["time_ms"].toInt(200),
		            smoothingConfig["updateDelay"].toInt(0),
		            smoothingConfig["continuousOutput"].toBool(true)
		            );
	}
	else
	{
		Error(CORE_LOGGER, "Smoothing disabled, because of unknown type '%s'.", QSTRING_CSTR(type));
	}

	device->setEnable(smoothingConfig["enable"].toBool(true));
	InfoIf(!device->enabled(), CORE_LOGGER,"Smoothing disabled");

	Q_ASSERT(device != nullptr);
	return device;
}

MessageForwarder * Hyperion::createMessageForwarder(const QJsonObject & forwarderConfig)
{
		MessageForwarder * forwarder = new MessageForwarder();
		if ( !forwarderConfig.isEmpty() && forwarderConfig["enable"].toBool(true) )
		{
			if ( !forwarderConfig["json"].isNull() && forwarderConfig["json"].isArray() )
			{
				const QJsonArray & addr = forwarderConfig["json"].toArray();
				for (signed i = 0; i < addr.size(); ++i)
				{
					Info(CORE_LOGGER, "Json forward to %s", addr.at(i).toString().toStdString().c_str());
					forwarder->addJsonSlave(addr[i].toString());
				}
			}

			if ( !forwarderConfig["proto"].isNull() && forwarderConfig["proto"].isArray() )
			{
				const QJsonArray & addr = forwarderConfig["proto"].toArray();
				for (signed i = 0; i < addr.size(); ++i)
				{
					Info(CORE_LOGGER, "Proto forward to %s", addr.at(i).toString().toStdString().c_str());
					forwarder->addProtoSlave(addr[i].toString());
				}
			}
		}

	return forwarder;
}

MessageForwarder * Hyperion::getForwarder()
{
	return _messageForwarder;
}

Hyperion::Hyperion(const QJsonObject &qjsonConfig, const QString configFile, const QString rootPath)
	: _ledString(createLedString(qjsonConfig["leds"], createColorOrder(qjsonConfig["device"].toObject())))
	, _ledStringClone(createLedStringClone(qjsonConfig["leds"], createColorOrder(qjsonConfig["device"].toObject())))
	, _muxer(_ledString.leds().size())
	, _raw2ledAdjustment(createLedColorsAdjustment(_ledString.leds().size(), qjsonConfig["color"].toObject()))
	, _effectEngine(nullptr)
	, _messageForwarder(createMessageForwarder(qjsonConfig["forwarder"].toObject()))
	, _qjsonConfig(qjsonConfig)
	, _configFile(configFile)
	, _rootPath(rootPath)
	, _timer()
	, _timerBonjourResolver()
	, _log(CORE_LOGGER)
	, _hwLedCount(_ledString.leds().size())
	, _sourceAutoSelectEnabled(true)
	, _configHash()
	, _ledGridSize(getLedLayoutGridSize(qjsonConfig["leds"]))
	, _prevCompId(hyperion::COMP_INVALID)
	, _bonjourBrowser(this)
	, _bonjourResolver(this)
	, _videoMode(VIDEO_2D)
	, _grabbingMode(GRABBINGMODE_INVALID)
{

	if (!_raw2ledAdjustment->verifyAdjustments())
	{
		throw std::runtime_error("Color adjustment incorrectly set");
	}
	// set color correction activity state
	const QJsonObject& color = qjsonConfig["color"].toObject();

	_bonjourBrowser.browseForServiceType(QLatin1String("_hyperiond-http._tcp"));
	connect(&_bonjourBrowser, SIGNAL(currentBonjourRecordsChanged(const QList<BonjourRecord>&)),this, SLOT(currentBonjourRecordsChanged(const QList<BonjourRecord> &)));
	connect(&_bonjourResolver, SIGNAL(bonjourRecordResolved(const QHostInfo &, int)), this, SLOT(bonjourRecordResolved(const QHostInfo &, int)));

	// initialize the image processor factory
	_ledMAppingType = ImageProcessor::mappingTypeToInt(color["imageToLedMappingType"].toString());
	ImageProcessorFactory::getInstance().init(_ledString, qjsonConfig["blackborderdetector"].toObject(),_ledMAppingType );

	getComponentRegister().componentStateChanged(hyperion::COMP_FORWARDER, _messageForwarder->forwardingEnabled());

	// initialize leddevices
	_device       = LedDeviceFactory::construct(qjsonConfig["device"].toObject(),_hwLedCount);
	_deviceSmooth = createColorSmoothing(qjsonConfig["smoothing"].toObject(), _device);
	getComponentRegister().componentStateChanged(hyperion::COMP_SMOOTHING, _deviceSmooth->componentState());
	getComponentRegister().componentStateChanged(hyperion::COMP_LEDDEVICE, _device->componentState());

	_deviceSmooth->addConfig(true); // add pause to config 1

	// setup the timer
	_timer.setSingleShot(true);
	QObject::connect(&_timer, SIGNAL(timeout()), this, SLOT(update()));

	_timerBonjourResolver.setSingleShot(false);
	_timerBonjourResolver.setInterval(1000);
	QObject::connect(&_timerBonjourResolver, SIGNAL(timeout()), this, SLOT(bonjourResolve()));
	_timerBonjourResolver.start();

	// create the effect engine, must be initialized after smoothing!
	_effectEngine = new EffectEngine(this,qjsonConfig["effects"].toObject());

	const QJsonObject& device = qjsonConfig["device"].toObject();
	unsigned int hwLedCount = device["ledCount"].toInt(getLedCount());
	_hwLedCount = qMax(hwLedCount, getLedCount());
	Debug(_log,"configured leds: %d hw leds: %d", getLedCount(), _hwLedCount);
	WarningIf(hwLedCount < getLedCount(), _log, "more leds configured than available. check 'ledCount' in 'device' section");

	// setup config state checks and initial shot
	checkConfigState();
	if(_fsWatcher.addPath(_configFile))
	{
		QObject::connect(&_fsWatcher, &QFileSystemWatcher::fileChanged, this, &Hyperion::checkConfigState);
	}
	else
	{
		Warning(_log,"Filesystem Observer failed for file: %s, use fallback timer", _configFile.toStdString().c_str());
		QObject::connect(&_cTimer, SIGNAL(timeout()), this, SLOT(checkConfigState()));
		_cTimer.start(2000);
	}

	// pipe muxer signal for effect/color timerunner to hyperionStateChanged slot
	QObject::connect(&_muxer, &PriorityMuxer::timerunner, this, &Hyperion::hyperionStateChanged);

	// prepare processing of hyperionStateChanged for forced serverinfo
	connect(&_fsi_timer, SIGNAL(timeout()), this, SLOT(hyperionStateChanged()));
	_fsi_timer.setSingleShot(true);
	_fsi_blockTimer.setSingleShot(true);

	// initialize the leds
	update();
}

int Hyperion::getLatchTime() const
{
  return _device->getLatchTime();
}

unsigned Hyperion::addSmoothingConfig(int settlingTime_ms, double ledUpdateFrequency_hz, unsigned updateDelay)
{
	return _deviceSmooth->addConfig(settlingTime_ms, ledUpdateFrequency_hz, updateDelay);
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
	delete _effectEngine;
	delete _device;
	delete _raw2ledAdjustment;
	delete _messageForwarder;
}

Hyperion::~Hyperion()
{
	freeObjects(false);
}

unsigned Hyperion::getLedCount() const
{
	return _ledString.leds().size();
}

void Hyperion::currentBonjourRecordsChanged(const QList<BonjourRecord> &list)
{
	_hyperionSessions.clear();
	for ( auto rec : list )
	{
		_hyperionSessions.insert(rec.serviceName, rec);
	}
}

void Hyperion::bonjourRecordResolved(const QHostInfo &hostInfo, int port)
{
	if ( _hyperionSessions.contains(_bonjourCurrentServiceToResolve))
	{
		QString host   = hostInfo.hostName();
		QString domain = _hyperionSessions[_bonjourCurrentServiceToResolve].replyDomain;
		if (host.endsWith("."+domain))
		{
			host.remove(host.length()-domain.length()-1,domain.length()+1);
		}
		_hyperionSessions[_bonjourCurrentServiceToResolve].hostName = host;
		_hyperionSessions[_bonjourCurrentServiceToResolve].port     = port;
		_hyperionSessions[_bonjourCurrentServiceToResolve].address  = hostInfo.addresses().isEmpty() ? "" : hostInfo.addresses().first().toString();
		Debug(_log, "found hyperion session: %s:%d",QSTRING_CSTR(hostInfo.hostName()), port);

		//emit change
		emit hyperionStateChanged();
	}
}

void Hyperion::bonjourResolve()
{
	for(auto key : _hyperionSessions.keys())
	{
		if (_hyperionSessions[key].port < 0)
		{
			_bonjourCurrentServiceToResolve = key;
			_bonjourResolver.resolveBonjourRecord(_hyperionSessions[key]);
			break;
		}
	}
}

Hyperion::BonjourRegister Hyperion::getHyperionSessions()
{
	return _hyperionSessions;
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
		emit hyperionStateChanged();
		_prevConfigMod = _configMod;
	}

	// Check config writeable
	QFile file(_configFile);
	QFileInfo fileInfo(file);
	_configWrite = fileInfo.isWritable() && fileInfo.isReadable() ? true : false;

	if(_prevConfigWrite != _configWrite)
	{
		emit hyperionStateChanged();
		_prevConfigWrite = _configWrite;
	}
}

void Hyperion::registerPriority(const QString &name, const int priority/*, const QString &origin*/)
{
	Info(_log, "Register new input source named '%s' for priority channel '%d'", QSTRING_CSTR(name), priority );

	for(auto key : _priorityRegister.keys())
	{
		WarningIf( ( key != name && _priorityRegister.value(key) == priority), _log,
		           "Input source '%s' uses same priority channel (%d) as '%s'.", QSTRING_CSTR(name), priority, QSTRING_CSTR(key));
	}

	_priorityRegister.insert(name, priority);
	emit hyperionStateChanged();
}

void Hyperion::unRegisterPriority(const QString &name)
{
	Info(_log, "Unregister input source named '%s' from priority register", QSTRING_CSTR(name));
	_priorityRegister.remove(name);
	emit hyperionStateChanged();
}

void Hyperion::setSourceAutoSelectEnabled(bool enabled)
{
	_sourceAutoSelectEnabled = enabled;
	if (! _sourceAutoSelectEnabled)
	{
		setCurrentSourcePriority(_muxer.getCurrentPriority());
	}
	update();
	DebugIf( !_sourceAutoSelectEnabled, _log, "source auto select is disabled");
	InfoIf(_sourceAutoSelectEnabled, _log, "set current input source to auto select");
}

bool Hyperion::setCurrentSourcePriority(int priority )
{
	bool priorityValid = _muxer.hasPriority(priority);
	if (priorityValid)
	{
		DebugIf(_sourceAutoSelectEnabled, _log, "source auto select is disabled");
		_sourceAutoSelectEnabled = false;
		_currentSourcePriority = priority;
		Info(_log, "set current input source to priority channel %d", _currentSourcePriority);
	}

	return priorityValid;
}

void Hyperion::setComponentState(const hyperion::Components component, const bool state)
{
	switch (component)
	{
		case hyperion::COMP_SMOOTHING:
			_deviceSmooth->setEnable(state);
			getComponentRegister().componentStateChanged(hyperion::COMP_SMOOTHING, _deviceSmooth->componentState());
			break;

		case hyperion::COMP_LEDDEVICE:
			_device->setEnable(state);
			getComponentRegister().componentStateChanged(hyperion::COMP_LEDDEVICE, _device->componentState());
			break;

		default:
			emit componentStateChanged(component, state);
	}
}

void Hyperion::setColor(int priority, const ColorRgb &color, const int timeout_ms, bool clearEffects)
{
	// create led output
	std::vector<ColorRgb> ledColors(_ledString.leds().size(), color);

	// set colors
	setColors(priority, ledColors, timeout_ms, clearEffects, hyperion::COMP_COLOR);
}

void Hyperion::setColors(int priority, const std::vector<ColorRgb>& ledColors, const int timeout_ms, bool clearEffects, hyperion::Components component, const QString origin, unsigned smoothCfg)
{
	// clear effects if this call does not come from an effect
	if (clearEffects)
	{
		_effectEngine->channelCleared(priority);
	}

	if (timeout_ms > 0)
	{
		const uint64_t timeoutTime = QDateTime::currentMSecsSinceEpoch() + timeout_ms;
		_muxer.setInput(priority, ledColors, timeoutTime, component, origin, smoothCfg);
	}
	else
	{
		_muxer.setInput(priority, ledColors, -1, component, origin, smoothCfg);
	}

	if (! _sourceAutoSelectEnabled || priority == _muxer.getCurrentPriority())
	{
		update();
	}
}

void Hyperion::setImage(int priority, const Image<ColorRgb> & image, int duration_ms)
{
	if (priority == getCurrentPriority())
	{
		emit emitImage(priority, image, duration_ms);
	}
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
	update();
}

void Hyperion::clear(int priority)
{
	if (_muxer.hasPriority(priority))
	{
		_muxer.clearInput(priority);
		if (!_sourceAutoSelectEnabled && _currentSourcePriority == priority )
		{
			setSourceAutoSelectEnabled(true);
		}

		// update leds if necessary
		if (priority < _muxer.getCurrentPriority())
		{
			update();
		}
	}

	// send clear signal to the effect engine
	// (outside the check so the effect gets cleared even when the effect is not sending colors)
	_effectEngine->channelCleared(priority);
}

void Hyperion::clearall(bool forceClearAll)
{
	_muxer.clearAll(forceClearAll);
	setSourceAutoSelectEnabled(true);

	// update leds
	update();

	// send clearall signal to the effect engine
	_effectEngine->allChannelsCleared();
}

int Hyperion::getCurrentPriority() const
{

	return _sourceAutoSelectEnabled || !_muxer.hasPriority(_currentSourcePriority) ? _muxer.getCurrentPriority() : _currentSourcePriority;
}

bool Hyperion::isCurrentPriority(const int priority) const
{
	return getCurrentPriority() == priority;
}

QList<int> Hyperion::getActivePriorities() const
{
	return _muxer.getPriorities();
}

const Hyperion::InputInfo &Hyperion::getPriorityInfo(const int priority) const
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

int Hyperion::setEffect(const QString &effectName, int priority, int timeout, const QString & origin)
{
	return _effectEngine->runEffect(effectName, priority, timeout, origin);
}

int Hyperion::setEffect(const QString &effectName, const QJsonObject &args, int priority, int timeout, const QString & pythonScript, const QString & origin)
{
	return _effectEngine->runEffect(effectName, args, priority, timeout, pythonScript, origin);
}

void Hyperion::setLedMappingType(int mappingType)
{
	_ledMAppingType = mappingType;
	emit imageToLedsMappingChanged(mappingType);
}

void Hyperion::setVideoMode(VideoMode mode)
{
	_videoMode = mode;
	emit videoMode(mode);
}

void Hyperion::setGrabbingMode(GrabbingMode mode)
{
	_grabbingMode = mode;
	emit grabbingMode(mode);
}


void Hyperion::hyperionStateChanged()
{
	if(_fsi_blockTimer.isActive())
	{
		_fsi_timer.start(300);
	}
	else
	{
		emit sendServerInfo();
		_fsi_blockTimer.start(250);
	}
}

void Hyperion::update()
{
	// Update the muxer, cleaning obsolete priorities
	_muxer.setCurrentTime(QDateTime::currentMSecsSinceEpoch());

	// Obtain the current priority channel
	int priority = _sourceAutoSelectEnabled || !_muxer.hasPriority(_currentSourcePriority) ? _muxer.getCurrentPriority() : _currentSourcePriority;
	const PriorityMuxer::InputInfo & priorityInfo  =  _muxer.getInputInfo(priority);

	// copy ledcolors to local buffer
	_ledBuffer.reserve(_hwLedCount);
	_ledBuffer = priorityInfo.ledColors;

	if (priorityInfo.componentId != _prevCompId)
	{
		bool backlightEnabled = (priorityInfo.componentId != hyperion::COMP_COLOR && priorityInfo.componentId != hyperion::COMP_EFFECT);
		_raw2ledAdjustment->setBacklightEnabled(backlightEnabled);
		_prevCompId = priorityInfo.componentId;
	}
	_raw2ledAdjustment->applyAdjustment(_ledBuffer);

	// init colororder vector, if empty
	if (_ledStringColorOrder.empty())
	{
		for (Led& led : _ledString.leds())
		{
			_ledStringColorOrder.push_back(led.colorOrder);
		}
		for (Led& led : _ledStringClone.leds())
		{
			_ledStringColorOrder.insert(_ledStringColorOrder.begin() + led.index, led.colorOrder);
		}
	}

	// insert cloned leds into buffer
	for (Led& led : _ledStringClone.leds())
	{
		_ledBuffer.insert(_ledBuffer.begin() + led.index, _ledBuffer.at(led.clone));
	}

	int i = 0;
	for (ColorRgb& color : _ledBuffer)
	{
		//const ColorOrder ledColorOrder = leds.at(i).colorOrder;

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

	// Start the timeout-timer
	if (priorityInfo.timeoutTime_ms <= 0)
	{
		_timer.stop();
	}
	else
	{
		int timeout_ms = qMax(0, int(priorityInfo.timeoutTime_ms - QDateTime::currentMSecsSinceEpoch()));
		// qMin() 200ms forced refresh if color is active to update priorityMuxer properly for forced serverinfo push
		_timer.start(qMin(timeout_ms, 200));
	}

}
