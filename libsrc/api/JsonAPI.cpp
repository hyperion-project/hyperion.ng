// project includes
#include <api/JsonAPI.h>

// Qt includes
#include <QResource>
#include <QDateTime>
#include <QImage>
#include <QBuffer>
#include <QByteArray>
#include <QTimer>
#include <QHostInfo>
#include <QMultiMap>

// hyperion includes
#include <leddevice/LedDeviceWrapper.h>
#include <leddevice/LedDevice.h>
#include <leddevice/LedDeviceFactory.h>

#include <HyperionConfig.h> // Required to determine the cmake options

#include <hyperion/GrabberWrapper.h>
#include <grabber/QtGrabber.h>

#if defined(ENABLE_MF)
	#include <grabber/MFGrabber.h>
#elif defined(ENABLE_V4L2)
	#include <grabber/V4L2Grabber.h>
#endif

#if defined(ENABLE_X11)
	#include <grabber/X11Grabber.h>
#endif

#if defined(ENABLE_XCB)
	#include <grabber/XcbGrabber.h>
#endif

#if defined(ENABLE_DX)
	#include <grabber/DirectXGrabber.h>
#endif

#if defined(ENABLE_FB)
	#include <grabber/FramebufferFrameGrabber.h>
#endif

#if defined(ENABLE_DISPMANX)
	#include <grabber/DispmanxFrameGrabber.h>
#endif

#if defined(ENABLE_AMLOGIC)
	#include <grabber/AmlogicGrabber.h>
#endif

#if defined(ENABLE_OSX)
	#include <grabber/OsxFrameGrabber.h>
#endif

#include <utils/jsonschema/QJsonFactory.h>
#include <utils/jsonschema/QJsonSchemaChecker.h>
#include <HyperionConfig.h>
#include <utils/SysInfo.h>
#include <utils/ColorSys.h>
#include <utils/Process.h>
#include <utils/JsonUtils.h>

// ledmapping int <> string transform methods
#include <hyperion/ImageProcessor.h>

// api includes
#include <api/JsonCB.h>

// auth manager
#include <hyperion/AuthManager.h>

#ifdef ENABLE_MDNS
// mDNS discover
#include <mdns/MdnsBrowser.h>
#include <mdns/MdnsServiceRegister.h>
#else
// ssdp discover
#include <ssdp/SSDPDiscover.h>
#endif

using namespace hyperion;

// Constants
namespace { const bool verbose = false; }

JsonAPI::JsonAPI(QString peerAddress, Logger *log, bool localConnection, QObject *parent, bool noListener)
	: API(log, localConnection, parent)
{
	_noListener = noListener;
	_peerAddress = peerAddress;
	_jsonCB = new JsonCB(this);
	_streaming_logging_activated = false;
	_ledStreamTimer = new QTimer(this);

	Q_INIT_RESOURCE(JSONRPC_schemas);
}

void JsonAPI::initialize()
{
	// init API, REQUIRED!
	API::init();

	// setup auth interface
	connect(this, &API::onPendingTokenRequest, this, &JsonAPI::newPendingTokenRequest);
	connect(this, &API::onTokenResponse, this, &JsonAPI::handleTokenResponse);

	// listen for killed instances
	connect(_instanceManager, &HyperionIManager::instanceStateChanged, this, &JsonAPI::handleInstanceStateChange);

	// pipe callbacks from subscriptions to parent
	connect(_jsonCB, &JsonCB::newCallback, this, &JsonAPI::callbackMessage);

	// notify hyperion about a jsonMessageForward
	if (_hyperion != nullptr)
	{
		// Initialise jsonCB with current instance
		_jsonCB->setSubscriptionsTo(_hyperion);
		connect(this, &JsonAPI::forwardJsonMessage, _hyperion, &Hyperion::forwardJsonMessage);
	}
}

bool JsonAPI::handleInstanceSwitch(quint8 inst, bool forced)
{
	if (API::setHyperionInstance(inst))
	{
		Debug(_log, "Client '%s' switch to Hyperion instance %d", QSTRING_CSTR(_peerAddress), inst);
		// the JsonCB creates json messages you can subscribe to e.g. data change events
		_jsonCB->setSubscriptionsTo(_hyperion);
		return true;
	}
	return false;
}

void JsonAPI::handleMessage(const QString &messageString, const QString &httpAuthHeader)
{
	const QString ident = "JsonRpc@" + _peerAddress;
	QJsonObject message;
	//std::cout << "JsonAPI::handleMessage | [" << static_cast<int>(_hyperion->getInstanceIndex()) << "] Received: ["<< messageString.toStdString() << "]" << std::endl;

	// parse the message
	if (!JsonUtils::parse(ident, messageString, message, _log))
	{
		sendErrorReply("Errors during message parsing, please consult the Hyperion Log.");
		return;
	}

	int tan = 0;
	if (message.value("tan") != QJsonValue::Undefined)
		tan = message["tan"].toInt();

	// check basic message
	if (!JsonUtils::validate(ident, message, ":schema", _log))
	{
		sendErrorReply("Errors during message validation, please consult the Hyperion Log.", "" /*command*/, tan);
		return;
	}

	// check specific message
	const QString command = message["command"].toString();
	if (!JsonUtils::validate(ident, message, QString(":schema-%1").arg(command), _log))
	{
		sendErrorReply("Errors during specific message validation, please consult the Hyperion Log", command, tan);
		return;
	}

	// client auth before everything else but not for http
	if (!_noListener && command == "authorize")
	{
		handleAuthorizeCommand(message, command, tan);
		return;
	}

	// check auth state
	if (!API::isAuthorized())
	{
		// on the fly auth available for http from http Auth header
		if (_noListener)
		{
			QString cToken = httpAuthHeader.mid(5).trimmed();
			if (API::isTokenAuthorized(cToken))
				goto proceed;
		}
		sendErrorReply("No Authorization", command, tan);
		return;
	}
proceed:
	if (_hyperion == nullptr)
	{
		sendErrorReply("Service Unavailable", command, tan);
		return;
	}

	// switch over all possible commands and handle them
	if (command == "color")
		handleColorCommand(message, command, tan);
	else if (command == "image")
		handleImageCommand(message, command, tan);
#if defined(ENABLE_EFFECTENGINE)
	else if (command == "effect")
		handleEffectCommand(message, command, tan);
	else if (command == "create-effect")
		handleCreateEffectCommand(message, command, tan);
	else if (command == "delete-effect")
		handleDeleteEffectCommand(message, command, tan);
#endif
	else if (command == "sysinfo")
		handleSysInfoCommand(message, command, tan);
	else if (command == "serverinfo")
		handleServerInfoCommand(message, command, tan);
	else if (command == "clear")
		handleClearCommand(message, command, tan);
	else if (command == "adjustment")
		handleAdjustmentCommand(message, command, tan);
	else if (command == "sourceselect")
		handleSourceSelectCommand(message, command, tan);
	else if (command == "config")
		handleConfigCommand(message, command, tan);
	else if (command == "componentstate")
		handleComponentStateCommand(message, command, tan);
	else if (command == "ledcolors")
		handleLedColorsCommand(message, command, tan);
	else if (command == "logging")
		handleLoggingCommand(message, command, tan);
	else if (command == "processing")
		handleProcessingCommand(message, command, tan);
	else if (command == "videomode")
		handleVideoModeCommand(message, command, tan);
	else if (command == "instance")
		handleInstanceCommand(message, command, tan);
	else if (command == "leddevice")
		handleLedDeviceCommand(message, command, tan);
	else if (command == "inputsource")
		handleInputSourceCommand(message, command, tan);
	else if (command == "service")
		handleServiceCommand(message, command, tan);

	// BEGIN | The following commands are deprecated but used to ensure backward compatibility with hyperion Classic remote control
	else if (command == "clearall")
		handleClearallCommand(message, command, tan);
	else if (command == "transform" || command == "correction" || command == "temperature")
		sendErrorReply("The command " + command + "is deprecated, please use the Hyperion Web Interface to configure", command, tan);
	// END

	// handle not implemented commands
	else
		handleNotImplemented(command, tan);
}

void JsonAPI::handleColorCommand(const QJsonObject &message, const QString &command, int tan)
{
	emit forwardJsonMessage(message);
	int priority = message["priority"].toInt();
	int duration = message["duration"].toInt(-1);
	const QString origin = message["origin"].toString("JsonRpc") + "@" + _peerAddress;

	const QJsonArray &jsonColor = message["color"].toArray();
	std::vector<uint8_t> colors;
	// TODO faster copy
	for (const auto &entry : jsonColor)
	{
		colors.emplace_back(uint8_t(entry.toInt()));
	}

	API::setColor(priority, colors, duration, origin);
	sendSuccessReply(command, tan);
}

void JsonAPI::handleImageCommand(const QJsonObject &message, const QString &command, int tan)
{
	emit forwardJsonMessage(message);

	API::ImageCmdData idata;
	idata.priority = message["priority"].toInt();
	idata.origin = message["origin"].toString("JsonRpc") + "@" + _peerAddress;
	idata.duration = message["duration"].toInt(-1);
	idata.width = message["imagewidth"].toInt();
	idata.height = message["imageheight"].toInt();
	idata.scale = message["scale"].toInt(-1);
	idata.format = message["format"].toString();
	idata.imgName = message["name"].toString("");
	idata.data = QByteArray::fromBase64(QByteArray(message["imagedata"].toString().toUtf8()));
	QString replyMsg;

	if (!API::setImage(idata, COMP_IMAGE, replyMsg))
	{
		sendErrorReply(replyMsg, command, tan);
		return;
	}
	sendSuccessReply(command, tan);
}

#if defined(ENABLE_EFFECTENGINE)
void JsonAPI::handleEffectCommand(const QJsonObject &message, const QString &command, int tan)
{
	emit forwardJsonMessage(message);

	EffectCmdData dat;
	dat.priority = message["priority"].toInt();
	dat.duration = message["duration"].toInt(-1);
	dat.pythonScript = message["pythonScript"].toString();
	dat.origin = message["origin"].toString("JsonRpc") + "@" + _peerAddress;
	dat.effectName = message["effect"].toObject()["name"].toString();
	dat.data = message["imageData"].toString("").toUtf8();
	dat.args = message["effect"].toObject()["args"].toObject();

	if (API::setEffect(dat))
		sendSuccessReply(command, tan);
	else
		sendErrorReply("Effect '" + dat.effectName + "' not found", command, tan);
}

void JsonAPI::handleCreateEffectCommand(const QJsonObject &message, const QString &command, int tan)
{
	const QString resultMsg = API::saveEffect(message);
	resultMsg.isEmpty() ? sendSuccessReply(command, tan) : sendErrorReply(resultMsg, command, tan);
}

void JsonAPI::handleDeleteEffectCommand(const QJsonObject &message, const QString &command, int tan)
{
	const QString res = API::deleteEffect(message["name"].toString());
	res.isEmpty() ? sendSuccessReply(command, tan) : sendErrorReply(res, command, tan);
}
#endif

void JsonAPI::handleSysInfoCommand(const QJsonObject &, const QString &command, int tan)
{
	// create result
	QJsonObject result;
	QJsonObject info;
	result["success"] = true;
	result["command"] = command;
	result["tan"] = tan;

	SysInfo::HyperionSysInfo data = SysInfo::get();
	QJsonObject system;
	system["kernelType"] = data.kernelType;
	system["kernelVersion"] = data.kernelVersion;
	system["architecture"] = data.architecture;
	system["cpuModelName"] = data.cpuModelName;
	system["cpuModelType"] = data.cpuModelType;
	system["cpuHardware"] = data.cpuHardware;
	system["cpuRevision"] = data.cpuRevision;
	system["wordSize"] = data.wordSize;
	system["productType"] = data.productType;
	system["productVersion"] = data.productVersion;
	system["prettyName"] = data.prettyName;
	system["hostName"] = data.hostName;
	system["domainName"] = data.domainName;
	system["isUserAdmin"] = data.isUserAdmin;
	system["qtVersion"] = data.qtVersion;
#if defined(ENABLE_EFFECTENGINE)
	system["pyVersion"] = data.pyVersion;
#endif
	info["system"] = system;

	QJsonObject hyperion;
	hyperion["version"] = QString(HYPERION_VERSION);
	hyperion["build"] = QString(HYPERION_BUILD_ID);
	hyperion["gitremote"] = QString(HYPERION_GIT_REMOTE);
	hyperion["time"] = QString(__DATE__ " " __TIME__);
	hyperion["id"] = _authManager->getID();
	hyperion["rootPath"] = _instanceManager->getRootPath();
	hyperion["readOnlyMode"] = _hyperion->getReadOnlyMode();

	info["hyperion"] = hyperion;

	// send the result
	result["info"] = info;
	emit callbackMessage(result);
}

void JsonAPI::handleServerInfoCommand(const QJsonObject &message, const QString &command, int tan)
{
	QJsonObject info;

	// collect priority information
	QJsonArray priorities;
	uint64_t now = QDateTime::currentMSecsSinceEpoch();
	QList<int> activePriorities = _hyperion->getActivePriorities();
	activePriorities.removeAll(PriorityMuxer::LOWEST_PRIORITY);
	int currentPriority = _hyperion->getCurrentPriority();

	for(int priority : qAsConst(activePriorities))
	{
		const Hyperion::InputInfo &priorityInfo = _hyperion->getPriorityInfo(priority);

		QJsonObject item;
		item["priority"] = priority;

		if (priorityInfo.timeoutTime_ms > 0 )
		{
			item["duration_ms"] = int(priorityInfo.timeoutTime_ms - now);
		}

		// owner has optional informations to the component
		if (!priorityInfo.owner.isEmpty())
		{
			item["owner"] = priorityInfo.owner;
		}

		item["componentId"] = QString(hyperion::componentToIdString(priorityInfo.componentId));
		item["origin"] = priorityInfo.origin;
		item["active"] = (priorityInfo.timeoutTime_ms >= -1);
		item["visible"] = (priority == currentPriority);

		if (priorityInfo.componentId == hyperion::COMP_COLOR && !priorityInfo.ledColors.empty())
		{
			QJsonObject LEDcolor;

			// add RGB Value to Array
			QJsonArray RGBValue;
			RGBValue.append(priorityInfo.ledColors.begin()->red);
			RGBValue.append(priorityInfo.ledColors.begin()->green);
			RGBValue.append(priorityInfo.ledColors.begin()->blue);
			LEDcolor.insert("RGB", RGBValue);

			uint16_t Hue;
			float Saturation;
			float Luminace;

			// add HSL Value to Array
			QJsonArray HSLValue;
			ColorSys::rgb2hsl(priorityInfo.ledColors.begin()->red,
							  priorityInfo.ledColors.begin()->green,
							  priorityInfo.ledColors.begin()->blue,
							  Hue, Saturation, Luminace);

			HSLValue.append(Hue);
			HSLValue.append(Saturation);
			HSLValue.append(Luminace);
			LEDcolor.insert("HSL", HSLValue);

			item["value"] = LEDcolor;
		}

		(priority == currentPriority)
		? priorities.prepend(item)
		: priorities.append(item);
	}

	info["priorities"] = priorities;
	info["priorities_autoselect"] = _hyperion->sourceAutoSelectEnabled();

	// collect adjustment information
	QJsonArray adjustmentArray;
	for (const QString &adjustmentId : _hyperion->getAdjustmentIds())
	{
		const ColorAdjustment *colorAdjustment = _hyperion->getAdjustment(adjustmentId);
		if (colorAdjustment == nullptr)
		{
			Error(_log, "Incorrect color adjustment id: %s", QSTRING_CSTR(adjustmentId));
			continue;
		}

		QJsonObject adjustment;
		adjustment["id"] = adjustmentId;

		QJsonArray whiteAdjust;
		whiteAdjust.append(colorAdjustment->_rgbWhiteAdjustment.getAdjustmentR());
		whiteAdjust.append(colorAdjustment->_rgbWhiteAdjustment.getAdjustmentG());
		whiteAdjust.append(colorAdjustment->_rgbWhiteAdjustment.getAdjustmentB());
		adjustment.insert("white", whiteAdjust);

		QJsonArray redAdjust;
		redAdjust.append(colorAdjustment->_rgbRedAdjustment.getAdjustmentR());
		redAdjust.append(colorAdjustment->_rgbRedAdjustment.getAdjustmentG());
		redAdjust.append(colorAdjustment->_rgbRedAdjustment.getAdjustmentB());
		adjustment.insert("red", redAdjust);

		QJsonArray greenAdjust;
		greenAdjust.append(colorAdjustment->_rgbGreenAdjustment.getAdjustmentR());
		greenAdjust.append(colorAdjustment->_rgbGreenAdjustment.getAdjustmentG());
		greenAdjust.append(colorAdjustment->_rgbGreenAdjustment.getAdjustmentB());
		adjustment.insert("green", greenAdjust);

		QJsonArray blueAdjust;
		blueAdjust.append(colorAdjustment->_rgbBlueAdjustment.getAdjustmentR());
		blueAdjust.append(colorAdjustment->_rgbBlueAdjustment.getAdjustmentG());
		blueAdjust.append(colorAdjustment->_rgbBlueAdjustment.getAdjustmentB());
		adjustment.insert("blue", blueAdjust);

		QJsonArray cyanAdjust;
		cyanAdjust.append(colorAdjustment->_rgbCyanAdjustment.getAdjustmentR());
		cyanAdjust.append(colorAdjustment->_rgbCyanAdjustment.getAdjustmentG());
		cyanAdjust.append(colorAdjustment->_rgbCyanAdjustment.getAdjustmentB());
		adjustment.insert("cyan", cyanAdjust);

		QJsonArray magentaAdjust;
		magentaAdjust.append(colorAdjustment->_rgbMagentaAdjustment.getAdjustmentR());
		magentaAdjust.append(colorAdjustment->_rgbMagentaAdjustment.getAdjustmentG());
		magentaAdjust.append(colorAdjustment->_rgbMagentaAdjustment.getAdjustmentB());
		adjustment.insert("magenta", magentaAdjust);

		QJsonArray yellowAdjust;
		yellowAdjust.append(colorAdjustment->_rgbYellowAdjustment.getAdjustmentR());
		yellowAdjust.append(colorAdjustment->_rgbYellowAdjustment.getAdjustmentG());
		yellowAdjust.append(colorAdjustment->_rgbYellowAdjustment.getAdjustmentB());
		adjustment.insert("yellow", yellowAdjust);

		adjustment["backlightThreshold"] = colorAdjustment->_rgbTransform.getBacklightThreshold();
		adjustment["backlightColored"] = colorAdjustment->_rgbTransform.getBacklightColored();
		adjustment["brightness"] = colorAdjustment->_rgbTransform.getBrightness();
		adjustment["brightnessCompensation"] = colorAdjustment->_rgbTransform.getBrightnessCompensation();
		adjustment["gammaRed"] = colorAdjustment->_rgbTransform.getGammaR();
		adjustment["gammaGreen"] = colorAdjustment->_rgbTransform.getGammaG();
		adjustment["gammaBlue"] = colorAdjustment->_rgbTransform.getGammaB();

		adjustment["saturationGain"] = colorAdjustment->_okhsvTransform.getSaturationGain();
		adjustment["brightnessGain"] = colorAdjustment->_okhsvTransform.getBrightnessGain();

		adjustmentArray.append(adjustment);
	}

	info["adjustment"] = adjustmentArray;

#if defined(ENABLE_EFFECTENGINE)
	// collect effect info
	QJsonArray effects;
	const std::list<EffectDefinition> &effectsDefinitions = _hyperion->getEffects();
	for (const EffectDefinition &effectDefinition : effectsDefinitions)
	{
		QJsonObject effect;
		effect["name"] = effectDefinition.name;
		effect["file"] = effectDefinition.file;
		effect["script"] = effectDefinition.script;
		effect["args"] = effectDefinition.args;
		effects.append(effect);
	}

	info["effects"] = effects;
#endif

	// get available led devices
	QJsonObject ledDevices;
	QJsonArray availableLedDevices;
	for (auto dev : LedDeviceWrapper::getDeviceMap())
	{
		availableLedDevices.append(dev.first);
	}

	ledDevices["available"] = availableLedDevices;
	info["ledDevices"] = ledDevices;

	QJsonObject grabbers;

	// *** Deprecated ***
	//QJsonArray availableGrabbers;
	//if ( GrabberWrapper::getInstance() != nullptr )
	//{
	//	QStringList activeGrabbers = GrabberWrapper::getInstance()->getActive(_hyperion->getInstanceIndex());
	//	QJsonArray activeGrabberNames;
	//	for (auto grabberName : activeGrabbers)
	//	{
	//		activeGrabberNames.append(grabberName);
	//	}

	//	grabbers["active"] = activeGrabberNames;
	//}
	//for (auto grabber : GrabberWrapper::availableGrabbers(GrabberTypeFilter::ALL))
	//{
	//	availableGrabbers.append(grabber);
	//}

	//grabbers["available"] = availableGrabbers;

	QJsonObject screenGrabbers;
	if (GrabberWrapper::getInstance() != nullptr)
	{
		QStringList activeGrabbers = GrabberWrapper::getInstance()->getActive(_hyperion->getInstanceIndex(), GrabberTypeFilter::SCREEN);
		QJsonArray activeGrabberNames;
		for (auto grabberName : activeGrabbers)
		{
			activeGrabberNames.append(grabberName);
		}

		screenGrabbers["active"] = activeGrabberNames;
	}
	QJsonArray availableScreenGrabbers;
	for (auto grabber : GrabberWrapper::availableGrabbers(GrabberTypeFilter::SCREEN))
	{
		availableScreenGrabbers.append(grabber);
	}
	screenGrabbers["available"] = availableScreenGrabbers;

	QJsonObject videoGrabbers;
	if (GrabberWrapper::getInstance() != nullptr)
	{
		QStringList activeGrabbers = GrabberWrapper::getInstance()->getActive(_hyperion->getInstanceIndex(), GrabberTypeFilter::VIDEO);
		QJsonArray activeGrabberNames;
		for (auto grabberName : activeGrabbers)
		{
			activeGrabberNames.append(grabberName);
		}

		videoGrabbers["active"] = activeGrabberNames;
	}
	QJsonArray availableVideoGrabbers;
	for (auto grabber : GrabberWrapper::availableGrabbers(GrabberTypeFilter::VIDEO))
	{
		availableVideoGrabbers.append(grabber);
	}
	videoGrabbers["available"] = availableVideoGrabbers;

	grabbers.insert("screen", screenGrabbers);
	grabbers.insert("video", videoGrabbers);
	info["grabbers"] = grabbers;

	info["videomode"] = QString(videoMode2String(_hyperion->getCurrentVideoMode()));

	QJsonObject cecInfo;
#if defined(ENABLE_CEC)
	cecInfo["enabled"] = true;
#else
	cecInfo["enabled"] = false;
#endif
	info["cec"] = cecInfo;

	// get available services
	QJsonArray services;

#if defined(ENABLE_BOBLIGHT_SERVER)
	services.append("boblight");
#endif

#if defined(ENABLE_CEC)
	services.append("cec");
#endif

#if defined(ENABLE_EFFECTENGINE)
	services.append("effectengine");
#endif

#if defined(ENABLE_FORWARDER)
	services.append("forwarder");
#endif

#if defined(ENABLE_FLATBUF_SERVER)
	services.append("flatbuffer");
#endif

#if defined(ENABLE_PROTOBUF_SERVER)
	services.append("protobuffer");
#endif

#if defined(ENABLE_MDNS)
	services.append("mDNS");
#endif
	services.append("SSDP");

	if (!availableScreenGrabbers.isEmpty() || !availableVideoGrabbers.isEmpty() || services.contains("flatbuffer") || services.contains("protobuffer"))
	{
		services.append("borderdetection");
	}

	info["services"] = services;

	// get available components
	QJsonArray component;
	std::map<hyperion::Components, bool> components = _hyperion->getComponentRegister()->getRegister();
	for (auto comp : components)
	{
		QJsonObject item;
		item["name"] = QString::fromStdString(hyperion::componentToIdString(comp.first));
		item["enabled"] = comp.second;

		component.append(item);
	}

	info["components"] = component;
	info["imageToLedMappingType"] = ImageProcessor::mappingTypeToStr(_hyperion->getLedMappingType());

	// add instance info
	QJsonArray instanceInfo;
	for (const auto &entry : API::getAllInstanceData())
	{
		QJsonObject obj;
		obj.insert("friendly_name", entry["friendly_name"].toString());
		obj.insert("instance", entry["instance"].toInt());
		//obj.insert("last_use", entry["last_use"].toString());
		obj.insert("running", entry["running"].toBool());
		instanceInfo.append(obj);
	}
	info["instance"] = instanceInfo;

	// add leds configs
	info["leds"] = _hyperion->getSetting(settings::LEDS).array();

	// BEGIN | The following entries are derecated but used to ensure backward compatibility with hyperion Classic remote control
	// TODO Output the real transformation information instead of default

	// HOST NAME
	info["hostname"] = QHostInfo::localHostName();

	// TRANSFORM INFORMATION (DEFAULT VALUES)
	QJsonArray transformArray;
	for (const QString &transformId : _hyperion->getAdjustmentIds())
	{
		QJsonObject transform;
		QJsonArray blacklevel, whitelevel, gamma, threshold;

		transform["id"] = transformId;
		transform["saturationGain"] = 1.0;
		transform["brightnessGain"] = 1.0;
		transform["saturationLGain"] = 1.0;
		transform["luminanceGain"] = 1.0;
		transform["luminanceMinimum"] = 0.0;

		for (int i = 0; i < 3; i++)
		{
			blacklevel.append(0.0);
			whitelevel.append(1.0);
			gamma.append(2.50);
			threshold.append(0.0);
		}

		transform.insert("blacklevel", blacklevel);
		transform.insert("whitelevel", whitelevel);
		transform.insert("gamma", gamma);
		transform.insert("threshold", threshold);

		transformArray.append(transform);
	}
	info["transform"] = transformArray;

#if defined(ENABLE_EFFECTENGINE)
	// ACTIVE EFFECT INFO
	QJsonArray activeEffects;
	for (const ActiveEffectDefinition &activeEffectDefinition : _hyperion->getActiveEffects())
	{
		if (activeEffectDefinition.priority != PriorityMuxer::LOWEST_PRIORITY - 1)
		{
			QJsonObject activeEffect;
			activeEffect["script"] = activeEffectDefinition.script;
			activeEffect["name"] = activeEffectDefinition.name;
			activeEffect["priority"] = activeEffectDefinition.priority;
			activeEffect["timeout"] = activeEffectDefinition.timeout;
			activeEffect["args"] = activeEffectDefinition.args;
			activeEffects.append(activeEffect);
		}
	}
	info["activeEffects"] = activeEffects;
#endif

	// ACTIVE STATIC LED COLOR
	QJsonArray activeLedColors;
	const Hyperion::InputInfo &priorityInfo = _hyperion->getPriorityInfo(_hyperion->getCurrentPriority());
	if (priorityInfo.componentId == hyperion::COMP_COLOR && !priorityInfo.ledColors.empty())
	{
		QJsonObject LEDcolor;
		// check if LED Color not Black (0,0,0)
		if ((priorityInfo.ledColors.begin()->red +
				 priorityInfo.ledColors.begin()->green +
				 priorityInfo.ledColors.begin()->blue !=
			 0))
		{
			QJsonObject LEDcolor;

			// add RGB Value to Array
			QJsonArray RGBValue;
			RGBValue.append(priorityInfo.ledColors.begin()->red);
			RGBValue.append(priorityInfo.ledColors.begin()->green);
			RGBValue.append(priorityInfo.ledColors.begin()->blue);
			LEDcolor.insert("RGB Value", RGBValue);

			uint16_t Hue;
			float Saturation, Luminace;

			// add HSL Value to Array
			QJsonArray HSLValue;
			ColorSys::rgb2hsl(priorityInfo.ledColors.begin()->red,
							  priorityInfo.ledColors.begin()->green,
							  priorityInfo.ledColors.begin()->blue,
							  Hue, Saturation, Luminace);

			HSLValue.append(Hue);
			HSLValue.append(Saturation);
			HSLValue.append(Luminace);
			LEDcolor.insert("HSL Value", HSLValue);

			activeLedColors.append(LEDcolor);
		}
	}
	info["activeLedColor"] = activeLedColors;

	// END

	sendSuccessDataReply(QJsonDocument(info), command, tan);

	// AFTER we send the info, the client might want to subscribe to future updates
	if (message.contains("subscribe"))
	{
		// check if listeners are allowed
		if (_noListener)
			return;

		QJsonArray subsArr = message["subscribe"].toArray();
		// catch the all keyword and build a list of all cmds
		if (subsArr.contains("all"))
		{
			subsArr = QJsonArray();
			for (const auto& entry : _jsonCB->getCommands())
			{
				subsArr.append(entry);
			}
		}

		for (const QJsonValueRef entry : subsArr)
		{
			// config callbacks just if auth is set
			if ((entry == "settings-update" || entry == "token-update") && !API::isAdminAuthorized())
				continue;
			// silent failure if a subscribe type is not found
			_jsonCB->subscribeFor(entry.toString());
		}
	}
}

void JsonAPI::handleClearCommand(const QJsonObject &message, const QString &command, int tan)
{
	emit forwardJsonMessage(message);
	int priority = message["priority"].toInt();
	QString replyMsg;

	if (!API::clearPriority(priority, replyMsg))
	{
		sendErrorReply(replyMsg, command, tan);
		return;
	}
	sendSuccessReply(command, tan);
}

void JsonAPI::handleClearallCommand(const QJsonObject &message, const QString &command, int tan)
{
	emit forwardJsonMessage(message);
	QString replyMsg;
	API::clearPriority(-1, replyMsg);
	sendSuccessReply(command, tan);
}

void JsonAPI::handleAdjustmentCommand(const QJsonObject &message, const QString &command, int tan)
{
	const QJsonObject &adjustment = message["adjustment"].toObject();

	const QString adjustmentId = adjustment["id"].toString(_hyperion->getAdjustmentIds().first());
	ColorAdjustment *colorAdjustment = _hyperion->getAdjustment(adjustmentId);
	if (colorAdjustment == nullptr)
	{
		Warning(_log, "Incorrect adjustment identifier: %s", adjustmentId.toStdString().c_str());
		return;
	}

	if (adjustment.contains("red"))
	{
		const QJsonArray &values = adjustment["red"].toArray();
		colorAdjustment->_rgbRedAdjustment.setAdjustment(values[0u].toInt(), values[1u].toInt(), values[2u].toInt());
	}

	if (adjustment.contains("green"))
	{
		const QJsonArray &values = adjustment["green"].toArray();
		colorAdjustment->_rgbGreenAdjustment.setAdjustment(values[0u].toInt(), values[1u].toInt(), values[2u].toInt());
	}

	if (adjustment.contains("blue"))
	{
		const QJsonArray &values = adjustment["blue"].toArray();
		colorAdjustment->_rgbBlueAdjustment.setAdjustment(values[0u].toInt(), values[1u].toInt(), values[2u].toInt());
	}
	if (adjustment.contains("cyan"))
	{
		const QJsonArray &values = adjustment["cyan"].toArray();
		colorAdjustment->_rgbCyanAdjustment.setAdjustment(values[0u].toInt(), values[1u].toInt(), values[2u].toInt());
	}
	if (adjustment.contains("magenta"))
	{
		const QJsonArray &values = adjustment["magenta"].toArray();
		colorAdjustment->_rgbMagentaAdjustment.setAdjustment(values[0u].toInt(), values[1u].toInt(), values[2u].toInt());
	}
	if (adjustment.contains("yellow"))
	{
		const QJsonArray &values = adjustment["yellow"].toArray();
		colorAdjustment->_rgbYellowAdjustment.setAdjustment(values[0u].toInt(), values[1u].toInt(), values[2u].toInt());
	}
	if (adjustment.contains("white"))
	{
		const QJsonArray &values = adjustment["white"].toArray();
		colorAdjustment->_rgbWhiteAdjustment.setAdjustment(values[0u].toInt(), values[1u].toInt(), values[2u].toInt());
	}

	if (adjustment.contains("gammaRed"))
	{
		colorAdjustment->_rgbTransform.setGamma(adjustment["gammaRed"].toDouble(), colorAdjustment->_rgbTransform.getGammaG(), colorAdjustment->_rgbTransform.getGammaB());
	}
	if (adjustment.contains("gammaGreen"))
	{
		colorAdjustment->_rgbTransform.setGamma(colorAdjustment->_rgbTransform.getGammaR(), adjustment["gammaGreen"].toDouble(), colorAdjustment->_rgbTransform.getGammaB());
	}
	if (adjustment.contains("gammaBlue"))
	{
		colorAdjustment->_rgbTransform.setGamma(colorAdjustment->_rgbTransform.getGammaR(), colorAdjustment->_rgbTransform.getGammaG(), adjustment["gammaBlue"].toDouble());
	}

	if (adjustment.contains("backlightThreshold"))
	{
		colorAdjustment->_rgbTransform.setBacklightThreshold(adjustment["backlightThreshold"].toDouble());
	}
	if (adjustment.contains("backlightColored"))
	{
		colorAdjustment->_rgbTransform.setBacklightColored(adjustment["backlightColored"].toBool());
	}
	if (adjustment.contains("brightness"))
	{
		colorAdjustment->_rgbTransform.setBrightness(adjustment["brightness"].toInt());
	}
	if (adjustment.contains("brightnessCompensation"))
	{
		colorAdjustment->_rgbTransform.setBrightnessCompensation(adjustment["brightnessCompensation"].toInt());
	}

    if (adjustment.contains("saturationGain"))
    {
        colorAdjustment->_okhsvTransform.setSaturationGain(adjustment["saturationGain"].toDouble());
    }

	if (adjustment.contains("brightnessGain"))
    {
		colorAdjustment->_okhsvTransform.setBrightnessGain(adjustment["brightnessGain"].toDouble());
    }

	// commit the changes
	_hyperion->adjustmentsUpdated();

	sendSuccessReply(command, tan);
}

void JsonAPI::handleSourceSelectCommand(const QJsonObject &message, const QString &command, int tan)
{
	if (message.contains("auto"))
	{
		API::setSourceAutoSelect(message["auto"].toBool(false));
	}
	else if (message.contains("priority"))
	{
		API::setVisiblePriority(message["priority"].toInt());
	}
	else
	{
		sendErrorReply("Priority request is invalid", command, tan);
		return;
	}
	sendSuccessReply(command, tan);
}

void JsonAPI::handleConfigCommand(const QJsonObject &message, const QString &command, int tan)
{
	QString subcommand = message["subcommand"].toString("");
	QString full_command = command + "-" + subcommand;

	if (subcommand == "getschema")
	{
		handleSchemaGetCommand(message, full_command, tan);
	}
	else if (subcommand == "getconfig")
	{
		if (_adminAuthorized)
			sendSuccessDataReply(QJsonDocument(_hyperion->getQJsonConfig()), full_command, tan);
		else
			sendErrorReply("No Authorization", command, tan);
	}
	else if (subcommand == "setconfig")
	{
		if (_adminAuthorized)
			handleConfigSetCommand(message, full_command, tan);
		else
			sendErrorReply("No Authorization", command, tan);
	}
	else if (subcommand == "restoreconfig")
	{
		if (_adminAuthorized)
			handleConfigRestoreCommand(message, full_command, tan);
		else
			sendErrorReply("No Authorization", command, tan);
	}
	else if (subcommand == "reload")
	{
		if (_adminAuthorized)
		{
			Debug(_log, "Restarting due to RPC command");

			Process::restartHyperion();

			sendSuccessReply(command + "-" + subcommand, tan);
		}
		else
		{
			sendErrorReply("No Authorization", command, tan);
		}
	}
	else
	{
		sendErrorReply("unknown or missing subcommand", full_command, tan);
	}
}

void JsonAPI::handleConfigSetCommand(const QJsonObject &message, const QString &command, int tan)
{
	if (message.contains("config"))
	{
		QJsonObject config = message["config"].toObject();
		if (API::isHyperionEnabled())
		{
			if ( API::saveSettings(config) )
			{
				sendSuccessReply(command, tan);
			}
			else
			{
				sendErrorReply("Save settings failed", command, tan);
			}
		}
		else
			sendErrorReply("Saving configuration while Hyperion is disabled isn't possible", command, tan);
	}
}

void JsonAPI::handleConfigRestoreCommand(const QJsonObject &message, const QString &command, int tan)
{
	if (message.contains("config"))
	{
		QJsonObject config = message["config"].toObject();
		if (API::isHyperionEnabled())
		{
			if ( API::restoreSettings(config) )
			{
				sendSuccessReply(command, tan);
			}
			else
			{
				sendErrorReply("Restore settings failed", command, tan);
			}
		}
		else
			sendErrorReply("Restoring configuration while Hyperion is disabled isn't possible", command, tan);
	}
}

void JsonAPI::handleSchemaGetCommand(const QJsonObject &message, const QString &command, int tan)
{
	// create result
	QJsonObject schemaJson, alldevices, properties;

	// make sure the resources are loaded (they may be left out after static linking)
	Q_INIT_RESOURCE(resource);

	// read the hyperion json schema from the resource
	QString schemaFile = ":/hyperion-schema";

	try
	{
		schemaJson = QJsonFactory::readSchema(schemaFile);
	}
	catch (const std::runtime_error &error)
	{
		throw std::runtime_error(error.what());
	}

	// collect all LED Devices
	properties = schemaJson["properties"].toObject();
	alldevices = LedDeviceWrapper::getLedDeviceSchemas();
	properties.insert("alldevices", alldevices);

#if defined(ENABLE_EFFECTENGINE)
	// collect all available effect schemas
	QJsonArray schemaList;
	const std::list<EffectSchema>& effectsSchemas = _hyperion->getEffectSchemas();
	for (const EffectSchema& effectSchema : effectsSchemas)
	{
		QJsonObject schema;
		schema.insert("script", effectSchema.pyFile);
		schema.insert("schemaLocation", effectSchema.schemaFile);
		schema.insert("schemaContent", effectSchema.pySchema);
		if (effectSchema.pyFile.startsWith(':'))
		{
			schema.insert("type", "system");
		}
		else
		{
			schema.insert("type", "custom");
		}
		schemaList.append(schema);
	}
	properties.insert("effectSchemas", schemaList);
#endif

	schemaJson.insert("properties", properties);

	// send the result
	sendSuccessDataReply(QJsonDocument(schemaJson), command, tan);
}

void JsonAPI::handleComponentStateCommand(const QJsonObject &message, const QString &command, int tan)
{
	const QJsonObject &componentState = message["componentstate"].toObject();
	QString comp = componentState["component"].toString("invalid");
	bool compState = componentState["state"].toBool(true);
	QString replyMsg;

	if (!API::setComponentState(comp, compState, replyMsg))
	{
		sendErrorReply(replyMsg, command, tan);
		return;
	}
	sendSuccessReply(command, tan);
}

void JsonAPI::handleLedColorsCommand(const QJsonObject &message, const QString &command, int tan)
{
	// create result
	QString subcommand = message["subcommand"].toString("");

	// max 20 Hz (50ms) interval for streaming (default: 10 Hz (100ms))
	qint64 streaming_interval = qMax(message["interval"].toInt(100), 50);

	if (subcommand == "ledstream-start")
	{
		_streaming_leds_reply["success"] = true;
		_streaming_leds_reply["command"] = command + "-ledstream-update";
		_streaming_leds_reply["tan"] = tan;

		connect(_hyperion, &Hyperion::rawLedColors, this, [=](const std::vector<ColorRgb> &ledValues) {
			_currentLedValues = ledValues;

			// necessary because Qt::UniqueConnection for lambdas does not work until 5.9
			// see: https://bugreports.qt.io/browse/QTBUG-52438
			if (!_ledStreamConnection)
				_ledStreamConnection = connect(_ledStreamTimer, &QTimer::timeout, this, [=]() {
					emit streamLedcolorsUpdate(_currentLedValues);
				},
				Qt::UniqueConnection);

			// start the timer
			if (!_ledStreamTimer->isActive() || _ledStreamTimer->interval() != streaming_interval)
				_ledStreamTimer->start(streaming_interval);
		},
		Qt::UniqueConnection);
		// push once
		_hyperion->update();
	}
	else if (subcommand == "ledstream-stop")
	{
		disconnect(_hyperion, &Hyperion::rawLedColors, this, 0);
		_ledStreamTimer->stop();
		disconnect(_ledStreamConnection);
	}
	else if (subcommand == "imagestream-start")
	{
		_streaming_image_reply["success"] = true;
		_streaming_image_reply["command"] = command + "-imagestream-update";
		_streaming_image_reply["tan"] = tan;

		connect(_hyperion, &Hyperion::currentImage, this, &JsonAPI::setImage, Qt::UniqueConnection);
	}
	else if (subcommand == "imagestream-stop")
	{
		disconnect(_hyperion, &Hyperion::currentImage, this, 0);
	}
	else
	{
		return;
	}

	sendSuccessReply(command + "-" + subcommand, tan);
}

void JsonAPI::handleLoggingCommand(const QJsonObject &message, const QString &command, int tan)
{
	// create result
	QString subcommand = message["subcommand"].toString("");

	if (API::isAdminAuthorized())
	{
		_streaming_logging_reply["success"] = true;
		_streaming_logging_reply["command"] = command;
		_streaming_logging_reply["tan"] = tan;

		if (subcommand == "start")
		{
			if (!_streaming_logging_activated)
			{
				_streaming_logging_reply["command"] = command + "-update";
				connect(LoggerManager::getInstance(), &LoggerManager::newLogMessage, this, &JsonAPI::incommingLogMessage);

				emit incommingLogMessage (Logger::T_LOG_MESSAGE{}); // needed to trigger log sending
				Debug(_log, "log streaming activated for client %s", _peerAddress.toStdString().c_str());
			}
		}
		else if (subcommand == "stop")
		{
			if (_streaming_logging_activated)
			{
				disconnect(LoggerManager::getInstance(), &LoggerManager::newLogMessage, this, &JsonAPI::incommingLogMessage);
				_streaming_logging_activated = false;
				Debug(_log, "log streaming deactivated for client  %s", _peerAddress.toStdString().c_str());
			}
		}
		else
		{
			return;
		}

		sendSuccessReply(command + "-" + subcommand, tan);
	}
	else
	{
		sendErrorReply("No Authorization", command + "-" + subcommand, tan);
	}
}

void JsonAPI::handleProcessingCommand(const QJsonObject &message, const QString &command, int tan)
{
	API::setLedMappingType(ImageProcessor::mappingTypeToInt(message["mappingType"].toString("multicolor_mean")));
	sendSuccessReply(command, tan);
}

void JsonAPI::handleVideoModeCommand(const QJsonObject &message, const QString &command, int tan)
{
	API::setVideoMode(parse3DMode(message["videoMode"].toString("2D")));
	sendSuccessReply(command, tan);
}

void JsonAPI::handleAuthorizeCommand(const QJsonObject &message, const QString &command, int tan)
{
	const QString &subc = message["subcommand"].toString().trimmed();
	const QString &id = message["id"].toString().trimmed();
	const QString &password = message["password"].toString().trimmed();
	const QString &newPassword = message["newPassword"].toString().trimmed();
	const QString &comment = message["comment"].toString().trimmed();

	// catch test if auth is required
	if (subc == "tokenRequired")
	{
		QJsonObject req;
		req["required"] = !API::isAuthorized();

		sendSuccessDataReply(QJsonDocument(req), command + "-" + subc, tan);
		return;
	}

	// catch test if admin auth is required
	if (subc == "adminRequired")
	{
		QJsonObject req;
		req["adminRequired"] = !API::isAdminAuthorized();
		sendSuccessDataReply(QJsonDocument(req), command + "-" + subc, tan);
		return;
	}

	// default hyperion password is a security risk, replace it asap
	if (subc == "newPasswordRequired")
	{
		QJsonObject req;
		req["newPasswordRequired"] = API::hasHyperionDefaultPw();
		sendSuccessDataReply(QJsonDocument(req), command + "-" + subc, tan);
		return;
	}

	// catch logout
	if (subc == "logout")
	{
		// disconnect all kind of data callbacks
		JsonAPI::stopDataConnections(); // TODO move to API
		API::logout();
		sendSuccessReply(command + "-" + subc, tan);
		return;
	}

	// change password
	if (subc == "newPassword")
	{
		// use password, newPassword
		if (API::isAdminAuthorized())
		{
			if (API::updateHyperionPassword(password, newPassword))
			{
				sendSuccessReply(command + "-" + subc, tan);
				return;
			}
			sendErrorReply("Failed to update user password", command + "-" + subc, tan);
			return;
		}
		sendErrorReply("No Authorization", command + "-" + subc, tan);
		return;
	}

	// token created from ui
	if (subc == "createToken")
	{
		// use comment
		// for user authorized sessions
		AuthManager::AuthDefinition def;
		const QString res = API::createToken(comment, def);
		if (res.isEmpty())
		{
			QJsonObject newTok;
			newTok["comment"] = def.comment;
			newTok["id"] = def.id;
			newTok["token"] = def.token;

			sendSuccessDataReply(QJsonDocument(newTok), command + "-" + subc, tan);
			return;
		}
		sendErrorReply(res, command + "-" + subc, tan);
		return;
	}

	// rename Token
	if (subc == "renameToken")
	{
		// use id/comment
		const QString res = API::renameToken(id, comment);
		if (res.isEmpty())
		{
			sendSuccessReply(command + "-" + subc, tan);
			return;
		}
		sendErrorReply(res, command + "-" + subc, tan);
		return;
	}

	// delete token
	if (subc == "deleteToken")
	{
		// use id
		const QString res = API::deleteToken(id);
		if (res.isEmpty())
		{
			sendSuccessReply(command + "-" + subc, tan);
			return;
		}
		sendErrorReply(res, command + "-" + subc, tan);
		return;
	}

	// catch token request
	if (subc == "requestToken")
	{
		// use id/comment
		const QString &comment = message["comment"].toString().trimmed();
		const bool &acc = message["accept"].toBool(true);
		if (acc)
			API::setNewTokenRequest(comment, id, tan);
		else
			API::cancelNewTokenRequest(comment, id);
		// client should wait for answer
		return;
	}

	// get pending token requests
	if (subc == "getPendingTokenRequests")
	{
		QVector<AuthManager::AuthDefinition> vec;
		if (API::getPendingTokenRequests(vec))
		{
			QJsonArray arr;
			for (const auto &entry : vec)
			{
				QJsonObject obj;
				obj["comment"] = entry.comment;
				obj["id"] = entry.id;
				obj["timeout"] = int(entry.timeoutTime);
				arr.append(obj);
			}
			sendSuccessDataReply(QJsonDocument(arr), command + "-" + subc, tan);
		}
		else
		{
			sendErrorReply("No Authorization", command + "-" + subc, tan);
		}

		return;
	}

	// accept/deny token request
	if (subc == "answerRequest")
	{
		// use id
		const bool &accept = message["accept"].toBool(false);
		if (!API::handlePendingTokenRequest(id, accept))
			sendErrorReply("No Authorization", command + "-" + subc, tan);
		return;
	}

	// get token list
	if (subc == "getTokenList")
	{
		QVector<AuthManager::AuthDefinition> defVect;
		if (API::getTokenList(defVect))
		{
			QJsonArray tArr;
			for (const auto &entry : defVect)
			{
				QJsonObject subO;
				subO["comment"] = entry.comment;
				subO["id"] = entry.id;
				subO["last_use"] = entry.lastUse;

				tArr.append(subO);
			}
			sendSuccessDataReply(QJsonDocument(tArr), command + "-" + subc, tan);
			return;
		}
		sendErrorReply("No Authorization", command + "-" + subc, tan);
		return;
	}

	// login
	if (subc == "login")
	{
		const QString &token = message["token"].toString().trimmed();

		// catch token
		if (!token.isEmpty())
		{
			// userToken is longer
			if (token.count() > 36)
			{
				if (API::isUserTokenAuthorized(token))
					sendSuccessReply(command + "-" + subc, tan);
				else
					sendErrorReply("No Authorization", command + "-" + subc, tan);

				return;
			}
			// usual app token is 36
			if (token.count() == 36)
			{
				if (API::isTokenAuthorized(token))
				{
					sendSuccessReply(command + "-" + subc, tan);
				}
				else
					sendErrorReply("No Authorization", command + "-" + subc, tan);
			}
			return;
		}

		// password
		// use password
		if (password.count() >= 8)
		{
			QString userTokenRep;
			if (API::isUserAuthorized(password) && API::getUserToken(userTokenRep))
			{
				// Return the current valid Hyperion user token
				QJsonObject obj;
				obj["token"] = userTokenRep;
				sendSuccessDataReply(QJsonDocument(obj), command + "-" + subc, tan);
			}
			else
				sendErrorReply("No Authorization", command + "-" + subc, tan);
		}
		else
			sendErrorReply("Password too short", command + "-" + subc, tan);
	}
}

void JsonAPI::handleInstanceCommand(const QJsonObject &message, const QString &command, int tan)
{
	const QString &subc = message["subcommand"].toString();
	const quint8 &inst = message["instance"].toInt();
	const QString &name = message["name"].toString();

	if (subc == "switchTo")
	{
		if (handleInstanceSwitch(inst))
		{
			QJsonObject obj;
			obj["instance"] = inst;
			sendSuccessDataReply(QJsonDocument(obj), command + "-" + subc, tan);
		}
		else
			sendErrorReply("Selected Hyperion instance isn't running", command + "-" + subc, tan);
		return;
	}

	if (subc == "startInstance")
	{
		connect(this, &API::onStartInstanceResponse, [=] (const int &tan) { sendSuccessReply(command + "-" + subc, tan); });
		if (!API::startInstance(inst, tan))
			sendErrorReply("Can't start Hyperion instance index " + QString::number(inst), command + "-" + subc, tan);

		return;
	}

	if (subc == "stopInstance")
	{
		// silent fail
		API::stopInstance(inst);
		sendSuccessReply(command + "-" + subc, tan);
		return;
	}

	if (subc == "deleteInstance")
	{
		QString replyMsg;
		if (API::deleteInstance(inst, replyMsg))
			sendSuccessReply(command + "-" + subc, tan);
		else
			sendErrorReply(replyMsg, command + "-" + subc, tan);
		return;
	}

	// create and save name requires name
	if (name.isEmpty())
		sendErrorReply("Name string required for this command", command + "-" + subc, tan);

	if (subc == "createInstance")
	{
		QString replyMsg = API::createInstance(name);
		if (replyMsg.isEmpty())
			sendSuccessReply(command + "-" + subc, tan);
		else
			sendErrorReply(replyMsg, command + "-" + subc, tan);
		return;
	}

	if (subc == "saveName")
	{
		QString replyMsg = API::setInstanceName(inst, name);
		if (replyMsg.isEmpty())
			sendSuccessReply(command + "-" + subc, tan);
		else
			sendErrorReply(replyMsg, command + "-" + subc, tan);
		return;
	}
}

void JsonAPI::handleLedDeviceCommand(const QJsonObject &message, const QString &command, int tan)
{
	Debug(_log, "message: [%s]", QString(QJsonDocument(message).toJson(QJsonDocument::Compact)).toUtf8().constData() );

	const QString &subc = message["subcommand"].toString().trimmed();
	const QString &devType = message["ledDeviceType"].toString().trimmed();

	QString full_command = command + "-" + subc;

	// TODO: Validate that device type is a valid one
/*	if ( ! valid type )
	{
		sendErrorReply("Unknown device", full_command, tan);
	}
	else
*/	{
		QJsonObject config;
		config.insert("type", devType);
		LedDevice* ledDevice = nullptr;

		if (subc == "discover")
		{
			ledDevice = LedDeviceFactory::construct(config);
			const QJsonObject &params = message["params"].toObject();
			const QJsonObject devicesDiscovered = ledDevice->discover(params);

			Debug(_log, "response: [%s]", QString(QJsonDocument(devicesDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData() );

			sendSuccessDataReply(QJsonDocument(devicesDiscovered), full_command, tan);
		}
		else if (subc == "getProperties")
		{
			ledDevice = LedDeviceFactory::construct(config);
			const QJsonObject &params = message["params"].toObject();
			const QJsonObject deviceProperties = ledDevice->getProperties(params);

			Debug(_log, "response: [%s]", QString(QJsonDocument(deviceProperties).toJson(QJsonDocument::Compact)).toUtf8().constData() );

			sendSuccessDataReply(QJsonDocument(deviceProperties), full_command, tan);
		}
		else if (subc == "identify")
		{
			ledDevice = LedDeviceFactory::construct(config);
			const QJsonObject &params = message["params"].toObject();
			ledDevice->identify(params);

			sendSuccessReply(full_command, tan);
		}
		else if (subc == "addAuthorization")
		{
			ledDevice = LedDeviceFactory::construct(config);
			const QJsonObject& params = message["params"].toObject();
			const QJsonObject response = ledDevice->addAuthorization(params);

			Debug(_log, "response: [%s]", QString(QJsonDocument(response).toJson(QJsonDocument::Compact)).toUtf8().constData());

			sendSuccessDataReply(QJsonDocument(response), full_command, tan);
		}
		else
		{
			sendErrorReply("Unknown or missing subcommand", full_command, tan);
		}

		delete  ledDevice;
	}
}

void JsonAPI::handleInputSourceCommand(const QJsonObject& message, const QString& command, int tan)
{
	DebugIf(verbose, _log, "message: [%s]", QString(QJsonDocument(message).toJson(QJsonDocument::Compact)).toUtf8().constData());

	const QString& subc = message["subcommand"].toString().trimmed();
	const QString& sourceType = message["sourceType"].toString().trimmed();

	QString full_command = command + "-" + subc;

	// TODO: Validate that source type is a valid one
/*	if ( ! valid type )
	{
		sendErrorReply("Unknown device", full_command, tan);
	}
	else
*/ {
		if (subc == "discover")
		{
			QJsonObject inputSourcesDiscovered;
			inputSourcesDiscovered.insert("sourceType", sourceType);
			QJsonArray videoInputs;

#if defined(ENABLE_V4L2) || defined(ENABLE_MF)

			if (sourceType == "video" )
			{
#if defined(ENABLE_MF)
				MFGrabber* grabber = new MFGrabber();
#elif defined(ENABLE_V4L2)
				V4L2Grabber* grabber = new V4L2Grabber();
#endif
				QJsonObject params;
				videoInputs = grabber->discover(params);
				delete grabber;
			}
			else
#endif
			{
				DebugIf(verbose, _log, "sourceType: [%s]", QSTRING_CSTR(sourceType));

				if (sourceType == "screen")
				{
					QJsonObject params;

					QJsonObject device;
					#ifdef ENABLE_QT
					QtGrabber* qtgrabber = new QtGrabber();
					device = qtgrabber->discover(params);
					if (!device.isEmpty() )
					{
						videoInputs.append(device);
					}
					delete qtgrabber;
					#endif

					#ifdef ENABLE_DX
					DirectXGrabber* dxgrabber = new DirectXGrabber();
					device = dxgrabber->discover(params);
					if (!device.isEmpty() )
					{
						videoInputs.append(device);
					}
					delete dxgrabber;
					#endif

					#ifdef ENABLE_X11
					X11Grabber* x11Grabber = new X11Grabber();
					device = x11Grabber->discover(params);
					if (!device.isEmpty() )
					{
						videoInputs.append(device);
					}
					delete x11Grabber;
					#endif

					#ifdef ENABLE_XCB
					XcbGrabber* xcbGrabber = new XcbGrabber();
					device = xcbGrabber->discover(params);
					if (!device.isEmpty() )
					{
						videoInputs.append(device);
					}
					delete xcbGrabber;
					#endif

					//Ignore FB for Amlogic, as it is embedded in the Amlogic grabber itself
					#if defined(ENABLE_FB) && !defined(ENABLE_AMLOGIC)

					FramebufferFrameGrabber* fbGrabber = new FramebufferFrameGrabber();
					device = fbGrabber->discover(params);
					if (!device.isEmpty() )
					{
						videoInputs.append(device);
					}
					delete fbGrabber;
					#endif

					#if defined(ENABLE_DISPMANX)
					DispmanxFrameGrabber* dispmanx = new DispmanxFrameGrabber();
					if (dispmanx->isAvailable())
					{
						device = dispmanx->discover(params);
						if (!device.isEmpty() )
						{
							videoInputs.append(device);
						}
					}
					delete dispmanx;
					#endif

					#if defined(ENABLE_AMLOGIC)
					AmlogicGrabber* amlGrabber = new AmlogicGrabber();
					device = amlGrabber->discover(params);
					if (!device.isEmpty() )
					{
						videoInputs.append(device);
					}
					delete amlGrabber;
					#endif

					#if defined(ENABLE_OSX)
					OsxFrameGrabber* osxGrabber = new OsxFrameGrabber();
					device = osxGrabber->discover(params);
					if (!device.isEmpty() )
					{
						videoInputs.append(device);
					}
					delete osxGrabber;
					#endif
				}

			}
			inputSourcesDiscovered["video_sources"] = videoInputs;

			DebugIf(verbose, _log, "response: [%s]", QString(QJsonDocument(inputSourcesDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

			sendSuccessDataReply(QJsonDocument(inputSourcesDiscovered), full_command, tan);
		}
		else
		{
			sendErrorReply("Unknown or missing subcommand", full_command, tan);
		}
	}
}

void JsonAPI::handleServiceCommand(const QJsonObject &message, const QString &command, int tan)
{
	DebugIf(verbose, _log, "message: [%s]", QString(QJsonDocument(message).toJson(QJsonDocument::Compact)).toUtf8().constData());

	const QString &subc = message["subcommand"].toString().trimmed();
	const QString type = message["serviceType"].toString().trimmed();

	QString full_command = command + "-" + subc;

	if (subc == "discover")
	{
		QByteArray serviceType;

		QJsonObject servicesDiscovered;
		QJsonObject servicesOfType;
		QJsonArray serviceList;

#ifdef ENABLE_MDNS
		QString discoveryMethod("mDNS");
		serviceType = MdnsServiceRegister::getServiceType(type);
#else
		QString discoveryMethod("ssdp");
#endif
		if (!serviceType.isEmpty())
		{
#ifdef ENABLE_MDNS
			QMetaObject::invokeMethod(&MdnsBrowser::getInstance(), "browseForServiceType",
									   Qt::QueuedConnection, Q_ARG(QByteArray, serviceType));

			serviceList = MdnsBrowser::getInstance().getServicesDiscoveredJson(serviceType, MdnsServiceRegister::getServiceNameFilter(type), DEFAULT_DISCOVER_TIMEOUT);
#endif
			servicesOfType.insert(type, serviceList);

			servicesDiscovered.insert("discoveryMethod", discoveryMethod);
			servicesDiscovered.insert("services", servicesOfType);

			sendSuccessDataReply(QJsonDocument(servicesDiscovered), full_command, tan);
		}
		else
		{
			sendErrorReply(QString("Discovery of service type [%1] via %2 not supported").arg(type, discoveryMethod), full_command, tan);
		}
	}
	else
	{
		sendErrorReply("Unknown or missing subcommand", full_command, tan);
	}
}

void JsonAPI::handleNotImplemented(const QString &command, int tan)
{
	sendErrorReply("Command not implemented", command, tan);
}

void JsonAPI::sendSuccessReply(const QString &command, int tan)
{
	// create reply
	QJsonObject reply;
	reply["success"] = true;
	reply["command"] = command;
	reply["tan"] = tan;

	// send reply
	emit callbackMessage(reply);
}

void JsonAPI::sendSuccessDataReply(const QJsonDocument &doc, const QString &command, int tan)
{
	QJsonObject reply;
	reply["success"] = true;
	reply["command"] = command;
	reply["tan"] = tan;
	if (doc.isArray())
		reply["info"] = doc.array();
	else
		reply["info"] = doc.object();

	emit callbackMessage(reply);
}

void JsonAPI::sendErrorReply(const QString &error, const QString &command, int tan)
{
	// create reply
	QJsonObject reply;
	reply["success"] = false;
	reply["error"] = error;
	reply["command"] = command;
	reply["tan"] = tan;

	// send reply
	emit callbackMessage(reply);
}

void JsonAPI::streamLedcolorsUpdate(const std::vector<ColorRgb> &ledColors)
{
	QJsonObject result;
	QJsonArray leds;

	for (const auto &color : ledColors)
	{
		leds << QJsonValue(color.red) << QJsonValue(color.green) << QJsonValue(color.blue);
	}

	result["leds"] = leds;
	_streaming_leds_reply["result"] = result;

	// send the result
	emit callbackMessage(_streaming_leds_reply);
}

void JsonAPI::setImage(const Image<ColorRgb> &image)
{
	QImage jpgImage((const uint8_t *)image.memptr(), image.width(), image.height(), 3 * image.width(), QImage::Format_RGB888);
	QByteArray ba;
	QBuffer buffer(&ba);
	buffer.open(QIODevice::WriteOnly);
	jpgImage.save(&buffer, "jpg");

	QJsonObject result;
	result["image"] = "data:image/jpg;base64," + QString(ba.toBase64());
	_streaming_image_reply["result"] = result;
	emit callbackMessage(_streaming_image_reply);
}

void JsonAPI::incommingLogMessage(const Logger::T_LOG_MESSAGE &msg)
{
	QJsonObject result, message;
	QJsonArray messageArray;

	if (!_streaming_logging_activated)
	{
		_streaming_logging_activated = true;
		const QList<Logger::T_LOG_MESSAGE> *logBuffer = LoggerManager::getInstance()->getLogMessageBuffer();
		for (int i = 0; i < logBuffer->length(); i++)
		{
			//Only present records of the current log-level
			if ( logBuffer->at(i).level >= _log->getLogLevel())
			{
				message["loggerName"] = logBuffer->at(i).loggerName;
				message["loggerSubName"] = logBuffer->at(i).loggerSubName;
				message["function"] = logBuffer->at(i).function;
				message["line"] = QString::number(logBuffer->at(i).line);
				message["fileName"] = logBuffer->at(i).fileName;
				message["message"] = logBuffer->at(i).message;
				message["levelString"] = logBuffer->at(i).levelString;
				message["utime"] = QString::number(logBuffer->at(i).utime);

				messageArray.append(message);
			}
		}
	}
	else
	{
		message["loggerName"] = msg.loggerName;
		message["loggerSubName"] = msg.loggerSubName;
		message["function"] = msg.function;
		message["line"] = QString::number(msg.line);
		message["fileName"] = msg.fileName;
		message["message"] = msg.message;
		message["levelString"] = msg.levelString;
		message["utime"] = QString::number(msg.utime);

		messageArray.append(message);
	}

	result.insert("messages", messageArray);
	_streaming_logging_reply["result"] = result;

	// send the result
	emit callbackMessage(_streaming_logging_reply);
}

void JsonAPI::newPendingTokenRequest(const QString &id, const QString &comment)
{
	QJsonObject obj;
	obj["comment"] = comment;
	obj["id"] = id;
	obj["timeout"] = 180000;

	sendSuccessDataReply(QJsonDocument(obj), "authorize-tokenRequest", 1);
}

void JsonAPI::handleTokenResponse(bool success, const QString &token, const QString &comment, const QString &id, const int &tan)
{
	const QString cmd = "authorize-requestToken";
	QJsonObject result;
	result["token"] = token;
	result["comment"] = comment;
	result["id"] = id;

	if (success)
		sendSuccessDataReply(QJsonDocument(result), cmd, tan);
	else
		sendErrorReply("Token request timeout or denied", cmd, tan);
}

void JsonAPI::handleInstanceStateChange(InstanceState state, quint8 instance, const QString &name)
{
	switch (state)
	{
	case InstanceState::H_ON_STOP:
		if (_hyperion->getInstanceIndex() == instance)
		{
			handleInstanceSwitch();
		}
		break;
	default:
		break;
	}
}

void JsonAPI::stopDataConnections()
{
	LoggerManager::getInstance()->disconnect();
	_streaming_logging_activated = false;
	_jsonCB->resetSubscriptions();
	// led stream colors
	disconnect(_hyperion, &Hyperion::rawLedColors, this, 0);
	_ledStreamTimer->stop();
	disconnect(_ledStreamConnection);
}
