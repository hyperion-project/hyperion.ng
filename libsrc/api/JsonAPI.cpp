// project includes
#include <api/JsonAPI.h>

// stl includes
#include <iostream>
#include <iterator>

// Qt includes
#include <QResource>
#include <QDateTime>
#include <QCryptographicHash>
#include <QImage>
#include <QBuffer>
#include <QByteArray>
#include <QTimer>

// hyperion includes
#include <leddevice/LedDeviceWrapper.h>
#include <hyperion/GrabberWrapper.h>
#include <utils/jsonschema/QJsonFactory.h>
#include <utils/jsonschema/QJsonSchemaChecker.h>
#include <HyperionConfig.h>
#include <utils/SysInfo.h>
#include <utils/ColorSys.h>
#include <utils/Process.h>
#include <utils/JsonUtils.h>

// bonjour wrapper
#include <bonjour/bonjourbrowserwrapper.h>

// ledmapping int <> string transform methods
#include <hyperion/ImageProcessor.h>

// api includes
#include <api/JsonCB.h>

// auth manager
#include <hyperion/AuthManager.h>

using namespace hyperion;

JsonAPI::JsonAPI(QString peerAddress, Logger* log, const bool& localConnection, QObject* parent, bool noListener)
	: QObject(parent)
	, _authManager(AuthManager::getInstance())
	, _authorized(false)
	, _userAuthorized(false)
	, _apiAuthRequired(_authManager->isAuthRequired())
	, _noListener(noListener)
	, _peerAddress(peerAddress)
	, _log(log)
	, _localConnection(localConnection)
	, _instanceManager(HyperionIManager::getInstance())
	, _hyperion(nullptr)
	, _jsonCB(new JsonCB(this))
	, _streaming_logging_activated(false)
	, _imageStreamTimer(new QTimer(this))
	, _ledStreamTimer(new QTimer(this))
{
	Q_INIT_RESOURCE(JSONRPC_schemas);
}

void JsonAPI::initialize(void)
{
	// For security we block external connections if default PW is set
	if(!_localConnection && _authManager->hasHyperionDefaultPw())
	{
		emit forceClose();
	}
	// if this is localConnection and network allows unauth locals, set authorized flag
	if(_apiAuthRequired && _localConnection)
		_authorized = !_authManager->isLocalAuthRequired();

	// admin access is allowed, when the connection is local and the option for local admin isn't set. Con: All local connections get full access
	if(_localConnection)
	{
		_userAuthorized = !_authManager->isLocalAdminAuthRequired();
		// just in positive direction
		if(_userAuthorized)
			_authorized = true;
	}

	// setup auth interface
	connect(_authManager, &AuthManager::newPendingTokenRequest, this, &JsonAPI::handlePendingTokenRequest);
	connect(_authManager, &AuthManager::tokenResponse, this, &JsonAPI::handleTokenResponse);

	// listen for killed instances
	connect(_instanceManager, &HyperionIManager::instanceStateChanged, this, &JsonAPI::handleInstanceStateChange);

	// pipe callbacks from subscriptions to parent
	connect(_jsonCB, &JsonCB::newCallback, this, &JsonAPI::callbackMessage);

	// init Hyperion pointer
	handleInstanceSwitch(0);

	// notify hyperion about a jsonMessageForward
	connect(this, &JsonAPI::forwardJsonMessage, _hyperion, &Hyperion::forwardJsonMessage);
}

bool JsonAPI::handleInstanceSwitch(const quint8& inst, const bool& forced)
{
	// check if we are already on the requested instance
	if(_hyperion != nullptr && _hyperion->getInstanceIndex() == inst)
		return true;

	if(_instanceManager->IsInstanceRunning(inst))
	{
		Debug(_log,"Client '%s' switch to Hyperion instance %d", QSTRING_CSTR(_peerAddress), inst);
		// cut all connections between hyperion / plugins and this
		if(_hyperion != nullptr)
			disconnect(_hyperion, 0, this, 0);

		// get new Hyperion pointer
		_hyperion = _instanceManager->getHyperionInstance(inst);

		// the JsonCB creates json messages you can subscribe to e.g. data change events
		_jsonCB->setSubscriptionsTo(_hyperion);

		return true;
	}
	return false;
}

void JsonAPI::handleMessage(const QString& messageString, const QString& httpAuthHeader)
{
	const QString ident = "JsonRpc@"+_peerAddress;
	QJsonObject message;
	// parse the message
	if(!JsonUtils::parse(ident, messageString, message, _log))
	{
		sendErrorReply("Errors during message parsing, please consult the Hyperion Log.");
		return;
	}

	// check basic message
	if(!JsonUtils::validate(ident, message, ":schema", _log))
	{
		sendErrorReply("Errors during message validation, please consult the Hyperion Log.");
		return;
	}

	// check specific message
	const QString command = message["command"].toString();
	if(!JsonUtils::validate(ident, message, QString(":schema-%1").arg(command), _log))
	{
		sendErrorReply("Errors during specific message validation, please consult the Hyperion Log");
		return;
	}

	int tan = message["tan"].toInt();

	// client auth before everything else but not for http
	if (!_noListener && command == "authorize")
	{
		handleAuthorizeCommand(message, command, tan);
		return;
	}

	// on the fly auth available for http from http Auth header, on failure we return and auth handler sends a failure
	if(_noListener && _apiAuthRequired && !_authorized)
	{
		// extract token from http header
		QString cToken = httpAuthHeader.mid(5).trimmed();
		if(!handleHTTPAuth(command, tan, cToken))
			return;
	}

	// on strong api auth you need a auth for all cmds
	if(_apiAuthRequired && !_authorized)
	{
		sendErrorReply("No Authorization", command, tan);
		return;
	}

	// switch over all possible commands and handle them
	if      (command == "color")          handleColorCommand         (message, command, tan);
	else if (command == "image")          handleImageCommand         (message, command, tan);
	else if (command == "effect")         handleEffectCommand        (message, command, tan);
	else if (command == "create-effect")  handleCreateEffectCommand  (message, command, tan);
	else if (command == "delete-effect")  handleDeleteEffectCommand  (message, command, tan);
	else if (command == "sysinfo")        handleSysInfoCommand       (message, command, tan);
	else if (command == "serverinfo")     handleServerInfoCommand    (message, command, tan);
	else if (command == "clear")          handleClearCommand         (message, command, tan);
	else if (command == "adjustment")     handleAdjustmentCommand    (message, command, tan);
	else if (command == "sourceselect")   handleSourceSelectCommand  (message, command, tan);
	else if (command == "config")         handleConfigCommand        (message, command, tan);
	else if (command == "componentstate") handleComponentStateCommand(message, command, tan);
	else if (command == "ledcolors")      handleLedColorsCommand     (message, command, tan);
	else if (command == "logging")        handleLoggingCommand       (message, command, tan);
	else if (command == "processing")     handleProcessingCommand    (message, command, tan);
	else if (command == "videomode")      handleVideoModeCommand     (message, command, tan);
	else if (command == "instance")       handleInstanceCommand      (message, command, tan);

	// BEGIN | The following commands are derecated but used to ensure backward compatibility with hyperion Classic remote control
	else if (command == "clearall")
		handleClearallCommand(message, command, tan);
	else if (command == "transform" || command == "correction" || command == "temperature")
		sendErrorReply("The command " + command + "is deprecated, please use the Hyperion Web Interface to configure");
	// END

	// handle not implemented commands
	else handleNotImplemented();
}

void JsonAPI::handleColorCommand(const QJsonObject& message, const QString& command, const int tan)
{
	emit forwardJsonMessage(message);

	// extract parameters
	int priority = message["priority"].toInt();
	int duration = message["duration"].toInt(-1);
	const QString origin = message["origin"].toString("JsonRpc") + "@"+_peerAddress;

	const QJsonArray & jsonColor = message["color"].toArray();
	const ColorRgb color = {uint8_t(jsonColor.at(0).toInt()),uint8_t(jsonColor.at(1).toInt()),uint8_t(jsonColor.at(2).toInt())};

	// set color
	_hyperion->setColor(priority, color, duration, origin);

	// send reply
	sendSuccessReply(command, tan);
}

void JsonAPI::handleImageCommand(const QJsonObject& message, const QString& command, const int tan)
{
	emit forwardJsonMessage(message);

	// extract parameters
	int priority = message["priority"].toInt();
	const QString origin = message["origin"].toString("JsonRpc") + "@"+_peerAddress;
	int duration = message["duration"].toInt(-1);
	int width = message["imagewidth"].toInt();
	int height = message["imageheight"].toInt();
	int scale = message["scale"].toInt(-1);
	QString format = message["format"].toString();
	QString imgName = message["name"].toString("");
	QByteArray data = QByteArray::fromBase64(QByteArray(message["imagedata"].toString().toUtf8()));

	// truncate name length
	imgName.truncate(16);

	if(format == "auto")
	{
		QImage img = QImage::fromData(data);
		if(img.isNull())
		{
			sendErrorReply("Failed to parse picture, the file might be corrupted", command, tan);
			return;
		}

		// check for requested scale
		if(scale > 24)
		{
			if(img.height() > scale)
			{
				img = img.scaledToHeight(scale);
			}
			if(img.width() > scale)
			{
				img = img.scaledToWidth(scale);
			}
		}

		// check if we need to force a scale
		if(img.width() > 2000 || img.height() > 2000)
		{
			scale = 2000;
			if(img.height() > scale)
			{
				img = img.scaledToHeight(scale);
			}
			if(img.width() > scale)
			{
				img = img.scaledToWidth(scale);
			}
		}

		width = img.width();
		height = img.height();

		// extract image
		img = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
		data.clear();
		data.reserve(img.width() * img.height() * 3);
		for (int i = 0; i < img.height(); ++i)
		{
			const QRgb * scanline = reinterpret_cast<const QRgb *>(img.scanLine(i));
			for (int j = 0; j < img.width(); ++j)
			{
				data.append((char) qRed(scanline[j]));
				data.append((char) qGreen(scanline[j]));
				data.append((char) qBlue(scanline[j]));
			}
		}
	}
	else
	{
		// check consistency of the size of the received data
		if (data.size() != width*height*3)
		{
			sendErrorReply("Size of image data does not match with the width and height", command, tan);
			return;
		}
	}

	// copy image
	Image<ColorRgb> image(width, height);
	memcpy(image.memptr(), data.data(), data.size());

	_hyperion->registerInput(priority, hyperion::COMP_IMAGE, origin, imgName);
	_hyperion->setInputImage(priority, image, duration);

	// send reply
	sendSuccessReply(command, tan);
}

void JsonAPI::handleEffectCommand(const QJsonObject &message, const QString &command, const int tan)
{
	emit forwardJsonMessage(message);

	// extract parameters
	int priority = message["priority"].toInt();
	int duration = message["duration"].toInt(-1);
	QString pythonScript = message["pythonScript"].toString();
	QString origin = message["origin"].toString("JsonRpc") + "@"+_peerAddress;
	const QJsonObject & effect = message["effect"].toObject();
	const QString & effectName = effect["name"].toString();
	const QString & data = message["imageData"].toString("").toUtf8();

	// set output
	 (effect.contains("args"))
		? _hyperion->setEffect(effectName, effect["args"].toObject(), priority, duration, pythonScript, origin, data)
		: _hyperion->setEffect(effectName, priority, duration, origin);

	// send reply
	sendSuccessReply(command, tan);
}

void JsonAPI::handleCreateEffectCommand(const QJsonObject& message, const QString &command, const int tan)
{
	QString resultMsg;
	if(_hyperion->saveEffect(message, resultMsg))
		sendSuccessReply(command, tan);
	else
		sendErrorReply(resultMsg, command, tan);
}

void JsonAPI::handleDeleteEffectCommand(const QJsonObject& message, const QString& command, const int tan)
{
	QString resultMsg;
	if(_hyperion->deleteEffect(message["name"].toString(), resultMsg))
		sendSuccessReply(command, tan);
	else
		sendErrorReply(resultMsg, command, tan);
}

void JsonAPI::handleSysInfoCommand(const QJsonObject&, const QString& command, const int tan)
{
	// create result
	QJsonObject result;
	QJsonObject info;
	result["success"] = true;
	result["command"] = command;
	result["tan"] = tan;

	SysInfo::HyperionSysInfo data = SysInfo::get();
	QJsonObject system;
	system["kernelType"    ] = data.kernelType;
	system["kernelVersion" ] = data.kernelVersion;
	system["architecture"  ] = data.architecture;
	system["wordSize"      ] = data.wordSize;
	system["productType"   ] = data.productType;
	system["productVersion"] = data.productVersion;
	system["prettyName"    ] = data.prettyName;
	system["hostName"      ] = data.hostName;
	system["domainName"    ] = data.domainName;
	info["system"] = system;

	QJsonObject hyperion;
	hyperion["jsonrpc_version" ] = QString(HYPERION_JSON_VERSION);
	hyperion["version"         ] = QString(HYPERION_VERSION);
	hyperion["channel"         ] = QString(HYPERION_VERSION_CHANNEL);
	hyperion["build"           ] = QString(HYPERION_BUILD_ID);
	hyperion["time"            ] = QString(__DATE__ " " __TIME__);
	hyperion["id"              ] = _authManager->getID();
	info["hyperion"] = hyperion;

	// send the result
	result["info" ] = info;
	emit callbackMessage(result);
}

void JsonAPI::handleServerInfoCommand(const QJsonObject& message, const QString& command, const int tan)
{
	QJsonObject info;

	// collect priority information
	QJsonArray priorities;
	uint64_t now = QDateTime::currentMSecsSinceEpoch();
	QList<int> activePriorities = _hyperion->getActivePriorities();
	activePriorities.removeAll(255);
	int currentPriority = _hyperion->getCurrentPriority();

	foreach (int priority, activePriorities) {
		const Hyperion::InputInfo & priorityInfo = _hyperion->getPriorityInfo(priority);
		QJsonObject item;
		item["priority"] = priority;
		if (priorityInfo.timeoutTime_ms > 0 )
			item["duration_ms"] = int(priorityInfo.timeoutTime_ms - now);

		// owner has optional informations to the component
		if(!priorityInfo.owner.isEmpty())
			item["owner"] = priorityInfo.owner;

		item["componentId"] = QString(hyperion::componentToIdString(priorityInfo.componentId));
		item["origin"] = priorityInfo.origin;
		item["active"] = (priorityInfo.timeoutTime_ms >= -1);
		item["visible"] = (priority == currentPriority);

		if(priorityInfo.componentId == hyperion::COMP_COLOR && !priorityInfo.ledColors.empty())
		{
			QJsonObject LEDcolor;

			// add RGB Value to Array
			QJsonArray RGBValue;
			RGBValue.append(priorityInfo.ledColors.begin()->red);
			RGBValue.append(priorityInfo.ledColors.begin()->green);
			RGBValue.append(priorityInfo.ledColors.begin()->blue);
			LEDcolor.insert("RGB", RGBValue);

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
			LEDcolor.insert("HSL", HSLValue);

			item["value"] = LEDcolor;
		}
		// priorities[priorities.size()] = item;
		priorities.append(item);
	}

	info["priorities"] = priorities;
	info["priorities_autoselect"] = _hyperion->sourceAutoSelectEnabled();

	// collect adjustment information
	QJsonArray adjustmentArray;
	for (const QString& adjustmentId : _hyperion->getAdjustmentIds())
	{
		const ColorAdjustment * colorAdjustment = _hyperion->getAdjustment(adjustmentId);
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
		adjustment["backlightColored"]   = colorAdjustment->_rgbTransform.getBacklightColored();
		adjustment["brightness"] = colorAdjustment->_rgbTransform.getBrightness();
		adjustment["brightnessCompensation"] = colorAdjustment->_rgbTransform.getBrightnessCompensation();
		adjustment["gammaRed"]   = colorAdjustment->_rgbTransform.getGammaR();
		adjustment["gammaGreen"] = colorAdjustment->_rgbTransform.getGammaG();
		adjustment["gammaBlue"]  = colorAdjustment->_rgbTransform.getGammaB();

		adjustmentArray.append(adjustment);
	}

	info["adjustment"] = adjustmentArray;

	// collect effect info
	QJsonArray effects;
	const std::list<EffectDefinition> & effectsDefinitions = _hyperion->getEffects();
	for (const EffectDefinition & effectDefinition : effectsDefinitions)
	{
		QJsonObject effect;
		effect["name"] = effectDefinition.name;
		effect["file"] = effectDefinition.file;
		effect["script"] = effectDefinition.script;
		effect["args"] = effectDefinition.args;
		effects.append(effect);
	}

	info["effects"] = effects;

	// get available led devices
	QJsonObject ledDevices;
	QJsonArray availableLedDevices;
	for (auto dev: LedDeviceWrapper::getDeviceMap())
	{
		availableLedDevices.append(dev.first);
	}

	ledDevices["available"] = availableLedDevices;
	info["ledDevices"] = ledDevices;

	QJsonObject grabbers;
	QJsonArray availableGrabbers;
#if defined(ENABLE_DISPMANX) || defined(ENABLE_V4L2) || defined(ENABLE_FB) || defined(ENABLE_AMLOGIC) || defined(ENABLE_OSX) || defined(ENABLE_X11)
	// get available grabbers
	//grabbers["active"] = ????;
	for (auto grabber: GrabberWrapper::availableGrabbers())
	{
		availableGrabbers.append(grabber);
	}
#endif
	grabbers["available"] = availableGrabbers;
	info["videomode"] = QString(videoMode2String(_hyperion->getCurrentVideoMode()));
	info["grabbers"]      = grabbers;

	// get available components
	QJsonArray component;
	std::map<hyperion::Components, bool> components = _hyperion->getComponentRegister().getRegister();
	for(auto comp : components)
	{
		QJsonObject item;
		item["name"] = QString::fromStdString(hyperion::componentToIdString(comp.first));
		item["enabled"] = comp.second;

		component.append(item);
	}

	info["components"] = component;
	info["imageToLedMappingType"] = ImageProcessor::mappingTypeToStr(_hyperion->getLedMappingType());

	// add sessions
	QJsonArray sessions;
	for (auto session: BonjourBrowserWrapper::getInstance()->getAllServices())
	{
		if (session.port<0) continue;
		QJsonObject item;
		item["name"]   = session.serviceName;
		item["type"]   = session.registeredType;
		item["domain"] = session.replyDomain;
		item["host"]   = session.hostName;
		item["address"]= session.address;
		item["port"]   = session.port;
		sessions.append(item);
	}
	info["sessions"] = sessions;

	// add instance info
	QJsonArray instanceInfo;
	for(const auto & entry : _instanceManager->getInstanceData())
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
		for (const QString& transformId : _hyperion->getAdjustmentIds())
		{
			QJsonObject transform;
			QJsonArray blacklevel, whitelevel, gamma, threshold;

			transform["id"] = transformId;
			transform["saturationGain"] = 1.0;
			transform["valueGain"]      = 1.0;
			transform["saturationLGain"] = 1.0;
			transform["luminanceGain"]   = 1.0;
			transform["luminanceMinimum"]   = 0.0;

			for (int i = 0; i < 3; i++ )
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

		// ACTIVE EFFECT INFO
		QJsonArray activeEffects;
		const std::list<ActiveEffectDefinition> & activeEffectsDefinitions = _hyperion->getActiveEffects();
		for (const ActiveEffectDefinition & activeEffectDefinition : activeEffectsDefinitions)
		{
			if (activeEffectDefinition.priority != PriorityMuxer::LOWEST_PRIORITY -1)
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

		// ACTIVE STATIC LED COLOR
		QJsonArray activeLedColors;
		const Hyperion::InputInfo & priorityInfo = _hyperion->getPriorityInfo(_hyperion->getCurrentPriority());
		if(priorityInfo.componentId == hyperion::COMP_COLOR && !priorityInfo.ledColors.empty())
		{
			QJsonObject LEDcolor;
			// check if LED Color not Black (0,0,0)
			if ((priorityInfo.ledColors.begin()->red +
			priorityInfo.ledColors.begin()->green +
			priorityInfo.ledColors.begin()->blue != 0))
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
	if(message.contains("subscribe"))
	{
		// check if listeners are allowed
		if(_noListener)
			return;

		QJsonArray subsArr = message["subscribe"].toArray();
		// catch the all keyword and build a list of all cmds
		if(subsArr.contains("all"))
		{
			subsArr = QJsonArray();
			for(const auto & entry : _jsonCB->getCommands())
			{
				subsArr.append(entry);
			}
		}
		for(const auto & entry : subsArr)
		{
			// config callbacks just if auth is set
			if(entry == "settings-update" && !_userAuthorized)
				continue;

			if(!_jsonCB->subscribeFor(entry.toString()))
				sendErrorReply(QString("Subscription for '%1' not found. Possible values: %2").arg(entry.toString(), _jsonCB->getCommands().join(", ")), command, tan);
		}
	}
}

void JsonAPI::handleClearCommand(const QJsonObject& message, const QString& command, const int tan)
{
	emit forwardJsonMessage(message);

	int priority = message["priority"].toInt();

	if(priority > 0)
		_hyperion->clear(priority);
	else if(priority < 0)
		_hyperion->clearall();
	else
	{
		sendErrorReply("Priority 0 is not allowed", command, tan);
		return;
	}

	// send reply
	sendSuccessReply(command, tan);
}

void JsonAPI::handleClearallCommand(const QJsonObject& message, const QString& command, const int tan)
{
	emit forwardJsonMessage(message);

	// clear priority
	_hyperion->clearall();

	// send reply
	sendSuccessReply(command, tan);
}

void JsonAPI::handleAdjustmentCommand(const QJsonObject& message, const QString& command, const int tan)
{
	const QJsonObject & adjustment = message["adjustment"].toObject();

	const QString adjustmentId = adjustment["id"].toString(_hyperion->getAdjustmentIds().first());
	ColorAdjustment * colorAdjustment = _hyperion->getAdjustment(adjustmentId);
	if (colorAdjustment == nullptr)
	{
		Warning(_log, "Incorrect adjustment identifier: %s", adjustmentId.toStdString().c_str());
		return;
	}

	if (adjustment.contains("red"))
	{
		const QJsonArray & values = adjustment["red"].toArray();
		colorAdjustment->_rgbRedAdjustment.setAdjustment(values[0u].toInt(), values[1u].toInt(), values[2u].toInt());
	}

	if (adjustment.contains("green"))
	{
		const QJsonArray & values = adjustment["green"].toArray();
		colorAdjustment->_rgbGreenAdjustment.setAdjustment(values[0u].toInt(), values[1u].toInt(), values[2u].toInt());
	}

	if (adjustment.contains("blue"))
	{
		const QJsonArray & values = adjustment["blue"].toArray();
		colorAdjustment->_rgbBlueAdjustment.setAdjustment(values[0u].toInt(), values[1u].toInt(), values[2u].toInt());
	}
	if (adjustment.contains("cyan"))
	{
		const QJsonArray & values = adjustment["cyan"].toArray();
		colorAdjustment->_rgbCyanAdjustment.setAdjustment(values[0u].toInt(), values[1u].toInt(), values[2u].toInt());
	}
	if (adjustment.contains("magenta"))
	{
		const QJsonArray & values = adjustment["magenta"].toArray();
		colorAdjustment->_rgbMagentaAdjustment.setAdjustment(values[0u].toInt(), values[1u].toInt(), values[2u].toInt());
	}
	if (adjustment.contains("yellow"))
	{
		const QJsonArray & values = adjustment["yellow"].toArray();
		colorAdjustment->_rgbYellowAdjustment.setAdjustment(values[0u].toInt(), values[1u].toInt(), values[2u].toInt());
	}
	if (adjustment.contains("white"))
	{
		const QJsonArray & values = adjustment["white"].toArray();
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

	// commit the changes
	_hyperion->adjustmentsUpdated();

	sendSuccessReply(command, tan);
}

void JsonAPI::handleSourceSelectCommand(const QJsonObject& message, const QString& command, const int tan)
{
	bool success = false;
	if (message["auto"].toBool(false))
	{
		_hyperion->setSourceAutoSelectEnabled(true);
		success = true;
	}
	else if (message.contains("priority"))
	{
		success = _hyperion->setCurrentSourcePriority(message["priority"].toInt());
	}

	if (success)
	{
		sendSuccessReply(command, tan);
	}
	else
	{
		sendErrorReply("setting current priority failed", command, tan);
	}
}

void JsonAPI::handleConfigCommand(const QJsonObject& message, const QString& command, const int tan)
{
	QString subcommand = message["subcommand"].toString("");
	QString full_command = command + "-" + subcommand;

	if (subcommand == "getschema")
	{
		handleSchemaGetCommand(message, full_command, tan);
	}
	else if (subcommand == "setconfig")
	{
		if(_userAuthorized)
			handleConfigSetCommand(message, full_command, tan);
		else
			sendErrorReply("No Authorization",command, tan);
	}
	else if (subcommand == "getconfig")
	{
		if(_userAuthorized)
			sendSuccessDataReply(QJsonDocument(_hyperion->getQJsonConfig()), full_command, tan);
		else
			sendErrorReply("No Authorization",command, tan);
	}
	else if (subcommand == "reload")
	{
		if(_userAuthorized)
		{
			_hyperion->freeObjects(true);
			Process::restartHyperion();
			sendErrorReply("failed to restart hyperion", full_command, tan);
		}
		else
			sendErrorReply("No Authorization",command, tan);
	}
	else
	{
		sendErrorReply("unknown or missing subcommand", full_command, tan);
	}
}

void JsonAPI::handleConfigSetCommand(const QJsonObject& message, const QString &command, const int tan)
{
	if (message.contains("config"))
	{
		QJsonObject config = message["config"].toObject();
		if(_hyperion->getComponentRegister().isComponentEnabled(hyperion::COMP_ALL))
		{
			if(_hyperion->saveSettings(config, true))
				sendSuccessReply(command,tan);
			else
				sendErrorReply("Failed to save configuration, more information at the Hyperion log", command, tan);
		}
		else
			sendErrorReply("Saving configuration while Hyperion is disabled isn't possible", command, tan);
	}
}

void JsonAPI::handleSchemaGetCommand(const QJsonObject& message, const QString& command, const int tan)
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
	catch(const std::runtime_error& error)
	{
		throw std::runtime_error(error.what());
	}

	// collect all LED Devices
	properties = schemaJson["properties"].toObject();
	alldevices = LedDeviceWrapper::getLedDeviceSchemas();
	properties.insert("alldevices", alldevices);

	// collect all available effect schemas
	QJsonObject pyEffectSchemas, pyEffectSchema;
	QJsonArray in, ex;
	const std::list<EffectSchema> & effectsSchemas = _hyperion->getEffectSchemas();
	for (const EffectSchema & effectSchema : effectsSchemas)
	{
		if (effectSchema.pyFile.mid(0, 1)  == ":")
		{
			QJsonObject internal;
			internal.insert("script", effectSchema.pyFile);
			internal.insert("schemaLocation", effectSchema.schemaFile);
			internal.insert("schemaContent", effectSchema.pySchema);
			in.append(internal);
		}
		else
		{
			QJsonObject external;
			external.insert("script", effectSchema.pyFile);
			external.insert("schemaLocation", effectSchema.schemaFile);
			external.insert("schemaContent", effectSchema.pySchema);
			ex.append(external);
		}
	}

	if (!in.empty())
		pyEffectSchema.insert("internal", in);
	if (!ex.empty())
		pyEffectSchema.insert("external", ex);

	pyEffectSchemas = pyEffectSchema;
	properties.insert("effectSchemas", pyEffectSchemas);

	schemaJson.insert("properties", properties);

	// send the result
	sendSuccessDataReply(QJsonDocument(schemaJson), command, tan);
}

void JsonAPI::handleComponentStateCommand(const QJsonObject& message, const QString &command, const int tan)
{
	const QJsonObject & componentState = message["componentstate"].toObject();

	QString compStr   = componentState["component"].toString("invalid");
	bool    compState = componentState["state"].toBool(true);

	Components component = stringToComponent(compStr);

	if (compStr == "ALL" )
	{
		if(_hyperion->getComponentRegister().setHyperionEnable(compState))
			sendSuccessReply(command, tan);

		return;
	}
	else if (component != COMP_INVALID)
	{
		// send result before apply
		sendSuccessReply(command, tan);
		_hyperion->setComponentState(component, compState);
		return;
	}
	sendErrorReply("invalid component name", command, tan);
}

void JsonAPI::handleLedColorsCommand(const QJsonObject& message, const QString &command, const int tan)
{
	// create result
	QString subcommand = message["subcommand"].toString("");

	// max 20 Hz (50ms) interval for streaming (default: 10 Hz (100ms))
	qint64 streaming_interval = qMax(message["interval"].toInt(100), 50);

	if (subcommand == "ledstream-start")
	{
		_streaming_leds_reply["success"] = true;
		_streaming_leds_reply["command"] = command+"-ledstream-update";
		_streaming_leds_reply["tan"]  = tan;

		connect(_hyperion, &Hyperion::rawLedColors, this, [=](const std::vector<ColorRgb>& ledValues)
		{
			_currentLedValues = ledValues;

			// necessary because Qt::UniqueConnection for lambdas does not work until 5.9
			// see: https://bugreports.qt.io/browse/QTBUG-52438
			if (!_ledStreamConnection)
				_ledStreamConnection = connect(_ledStreamTimer, &QTimer::timeout, this, [=]()
				{
					emit streamLedcolorsUpdate(_currentLedValues);
				}, Qt::UniqueConnection);

			// start the timer
			if (!_ledStreamTimer->isActive() || _ledStreamTimer->interval() != streaming_interval)
				_ledStreamTimer->start(streaming_interval);
		}, Qt::UniqueConnection);
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
		_streaming_image_reply["command"] = command+"-imagestream-update";
		_streaming_image_reply["tan"]  = tan;

		connect(_hyperion, &Hyperion::currentImage, this, [=](const Image<ColorRgb>& image)
		{
			_currentImage = image;

			// necessary because Qt::UniqueConnection for lambdas does not work until 5.9
			// see: https://bugreports.qt.io/browse/QTBUG-52438
			if (!_imageStreamConnection)
				_imageStreamConnection = connect(_imageStreamTimer, &QTimer::timeout, this, [=]()
				{
					emit setImage(_currentImage);
				}, Qt::UniqueConnection);

			// start timer
			if (!_imageStreamTimer->isActive() || _imageStreamTimer->interval() != streaming_interval)
				_imageStreamTimer->start(streaming_interval);
		}, Qt::UniqueConnection);

		_hyperion->update();
	}
	else if (subcommand == "imagestream-stop")
	{
		disconnect(_hyperion, &Hyperion::currentImage, this, 0);
		_imageStreamTimer->stop();
		disconnect(_imageStreamConnection);
	}
	else
	{
		return;
	}

	sendSuccessReply(command+"-"+subcommand,tan);
}

void JsonAPI::handleLoggingCommand(const QJsonObject& message, const QString &command, const int tan)
{
	// create result
	QString subcommand = message["subcommand"].toString("");
	_streaming_logging_reply["success"] = true;
	_streaming_logging_reply["command"] = command;
	_streaming_logging_reply["tan"]     = tan;

	if (subcommand == "start")
	{
		if (!_streaming_logging_activated)
		{
			_streaming_logging_reply["command"] = command+"-update";
			connect(LoggerManager::getInstance(),SIGNAL(newLogMessage(Logger::T_LOG_MESSAGE)), this, SLOT(incommingLogMessage(Logger::T_LOG_MESSAGE)));
			Debug(_log, "log streaming activated for client %s",_peerAddress.toStdString().c_str()); // needed to trigger log sending
		}
	}
	else if (subcommand == "stop")
	{
		if (_streaming_logging_activated)
		{
			disconnect(LoggerManager::getInstance(), SIGNAL(newLogMessage(Logger::T_LOG_MESSAGE)), this, 0);
			_streaming_logging_activated = false;
			Debug(_log, "log streaming deactivated for client  %s",_peerAddress.toStdString().c_str());

		}
	}
	else
	{
		return;
	}

	sendSuccessReply(command+"-"+subcommand,tan);
}

void JsonAPI::handleProcessingCommand(const QJsonObject& message, const QString &command, const int tan)
{
	_hyperion->setLedMappingType(ImageProcessor::mappingTypeToInt( message["mappingType"].toString("multicolor_mean")) );

	sendSuccessReply(command, tan);
}

void JsonAPI::handleVideoModeCommand(const QJsonObject& message, const QString &command, const int tan)
{
	_hyperion->setVideoMode(parse3DMode(message["videoMode"].toString("2D")));

	sendSuccessReply(command, tan);
}

void JsonAPI::handleAuthorizeCommand(const QJsonObject & message, const QString &command, const int tan)
{
	const QString& subc = message["subcommand"].toString().trimmed();
	const QString& id = message["id"].toString().trimmed();
	const QString& password = message["password"].toString().trimmed();
	const QString& newPassword = message["newPassword"].toString().trimmed();

	// catch test if auth is required
	if(subc == "required")
	{
		QJsonObject req;
		req["required"] = !_authorized;
		sendSuccessDataReply(QJsonDocument(req), command+"-"+subc, tan);
		return;
	}

	// catch test if admin auth is required
	if(subc == "adminRequired")
	{
		QJsonObject req;
		req["adminRequired"] = !_userAuthorized;
		sendSuccessDataReply(QJsonDocument(req), command+"-"+subc, tan);
		return;
	}

	// default hyperion password is a security risk, replace it asap
	if(subc == "newPasswordRequired")
	{
		QJsonObject req;
		req["newPasswordRequired"] = _authManager->hasHyperionDefaultPw();
		sendSuccessDataReply(QJsonDocument(req), command+"-"+subc, tan);
		return;
	}

	// catch logout
	if(subc == "logout")
	{
		_authorized = false;
		_userAuthorized = false;
		// disconnect all kind of data callbacks
		stopDataConnections();
		sendSuccessReply(command+"-"+subc, tan);
		return;
	}

	// change password
	if(subc == "newPassword")
	{
		// use password, newPassword
		if(_userAuthorized)
		{
			if(_authManager->updateUserPassword("Hyperion", password, newPassword))
			{
				sendSuccessReply(command+"-"+subc, tan);
				return;
			}
			sendErrorReply("Failed to update user password",command+"-"+subc, tan);
			return;
		}
		sendErrorReply("No Authorization",command+"-"+subc, tan);
		return;
	}

	// token created from ui
	if(subc == "createToken")
	{
		const QString& c = message["comment"].toString().trimmed();
		// for user authorized sessions
		if(_userAuthorized)
		{
			AuthManager::AuthDefinition def = _authManager->createToken(c);
			QJsonObject newTok;
			newTok["comment"] = def.comment;
			newTok["id"] = def.id;
			newTok["token"] = def.token;

			sendSuccessDataReply(QJsonDocument(newTok), command+"-"+subc, tan);
			return;
		}
		sendErrorReply("No Authorization",command+"-"+subc, tan);
		return;
	}

	// delete token
	if(subc == "deleteToken")
	{
		// use id
		// for user authorized sessions
		if(_userAuthorized)
		{
			_authManager->deleteToken(id);
			sendSuccessReply(command+"-"+subc, tan);
			return;
		}
		sendErrorReply("No Authorization",command+"-"+subc, tan);
		return;
	}

	// catch token request
	if(subc == "requestToken")
	{
		// use id
		const QString& comment = message["comment"].toString().trimmed();
		_authManager->setNewTokenRequest(this, comment, id);
		// client should wait for answer
		return;
	}

	// get pending token requests
	if(subc == "getPendingRequests")
	{
		if(_userAuthorized)
		{
			QMap<QString, AuthManager::AuthDefinition> map = _authManager->getPendingRequests();
			QJsonArray arr;
			for(const auto& entry : map)
			{
				QJsonObject obj;
				obj["comment"] = entry.comment;
				obj["id"] = entry.id;
				obj["timeout"] = int(entry.timeoutTime - QDateTime::currentMSecsSinceEpoch());
				arr.append(obj);
			}
			sendSuccessDataReply(QJsonDocument(arr),command+"-"+subc, tan);
		}
		else
			sendErrorReply("No Authorization", command+"-"+subc, tan);

		return;
	}

	// accept/deny token request
	if(subc == "answerRequest")
	{
		// use id
		const bool& accept = message["accept"].toBool(false);
		if(_userAuthorized)
		{
			if(accept)
				_authManager->acceptTokenRequest(id);
			else
				_authManager->denyTokenRequest(id);
		}
		else
			sendErrorReply("No Authorization", command+"-"+subc, tan);

		return;
	}
	// deny token request
	if(subc == "acceptRequest")
	{
		// use id
		if(_userAuthorized)
		{
			_authManager->acceptTokenRequest(id);
		}
		else
			sendErrorReply("No Authorization", command+"-"+subc, tan);

		return;
	}

	// catch get token list
	if(subc == "getTokenList")
	{
		if(_userAuthorized)
		{
			QVector<AuthManager::AuthDefinition> defVect = _authManager->getTokenList();
			QJsonArray tArr;
			for(const auto& entry : defVect)
			{
				QJsonObject subO;
				subO["comment"] = entry.comment;
				subO["id"] = entry.id;
				subO["last_use"] = entry.lastUse;

				tArr.append(subO);
			}

			sendSuccessDataReply(QJsonDocument(tArr),command+"-"+subc, tan);
			return;
		}
		sendErrorReply("No Authorization",command+"-"+subc, tan);
		return;
	}

	// login
	if(subc == "login")
	{
		const QString& token = message["token"].toString().trimmed();

		// catch token
		if(!token.isEmpty())
		{
			// userToken is longer
			if(token.count() > 36)
			{
				if(_authManager->isUserTokenAuthorized("Hyperion",token))
				{
					_authorized = true;
					_userAuthorized = true;
					sendSuccessReply(command+"-"+subc, tan);
				}
				else
					sendErrorReply("No Authorization", command+"-"+subc, tan);

				return;
			}
			// usual app token is 36
			if(token.count() == 36)
			{
				if(_authManager->isTokenAuthorized(token))
				{
					_authorized = true;
					sendSuccessReply(command+"-"+subc, tan);
				}
				else
					sendErrorReply("No Authorization", command+"-"+subc, tan);
			}
			else
				sendErrorReply("Token is too short", command+"-"+subc, tan);

			return;
		}

		// password
		// use password

		if(password.count() >= 8)
		{
			if(_authManager->isUserAuthorized("Hyperion", password))
			{
				_authorized = true;
				_userAuthorized = true;
				// Return the current valid Hyperion user token
				QJsonObject obj;
				obj["token"] = _authManager->getUserToken();
				sendSuccessDataReply(QJsonDocument(obj),command+"-"+subc, tan);
			}
			else
				sendErrorReply("No Authorization", command+"-"+subc, tan);
		}
		else
			sendErrorReply("Password string too short", command+"-"+subc, tan);
	}
}

bool JsonAPI::handleHTTPAuth(const QString& command, const int& tan, const QString& token)
{
	if(_authManager->isTokenAuthorized(token))
	{
		_authorized = true;
		return true;
	}
	sendErrorReply("No Authorization", command, tan);
	return false;
}

void JsonAPI::handleInstanceCommand(const QJsonObject & message, const QString &command, const int tan)
{
	const QString & subc = message["subcommand"].toString();
	const quint8 & inst = message["instance"].toInt();
	const QString & name = message["name"].toString();

	if(subc == "switchTo")
	{
		if(handleInstanceSwitch(inst)){
			QJsonObject obj;
			obj["instance"] = inst;
			sendSuccessDataReply(QJsonDocument(obj),command+"-"+subc, tan);
		}
		else
			sendErrorReply("Selected Hyperion instance isn't running",command+"-"+subc, tan);
		return;
	}

	if(subc == "startInstance")
	{
		// silent fail
		_instanceManager->startInstance(inst);
		sendSuccessReply(command+"-"+subc, tan);
		return;
	}

	if(subc == "stopInstance")
	{
		// silent fail
		_instanceManager->stopInstance(inst);
		sendSuccessReply(command+"-"+subc, tan);
		return;
	}

	if(subc == "deleteInstance")
	{
		if(_userAuthorized)
		{
			if(_instanceManager->deleteInstance(inst))
				sendSuccessReply(command+"-"+subc, tan);
			else
				sendErrorReply(QString("Failed to delete instance '%1'").arg(inst), command+"-"+subc, tan);
		}
		else
			sendErrorReply("No Authorization",command+"-"+subc, tan);
		return;
	}

	// create and save name requires name
	if(name.isEmpty())
		sendErrorReply("Name string required for this command",command+"-"+subc, tan);

	if(subc == "createInstance")
	{
		if(_userAuthorized)
		{
			if(_instanceManager->createInstance(name))
				sendSuccessReply(command+"-"+subc, tan);
			else
				sendErrorReply(QString("The instance name '%1' is already in use").arg(name), command+"-"+subc, tan);
		}
		else
			sendErrorReply("No Authorization",command+"-"+subc, tan);
		return;
	}

	if(subc == "saveName")
	{
		if(_userAuthorized)
		{
			// silent fail
			if(_instanceManager->saveName(inst,name))
				sendSuccessReply(command+"-"+subc, tan);
			else
				sendErrorReply(QString("The instance name '%1' is already in use").arg(name), command+"-"+subc, tan);
		}
		else
			sendErrorReply("No Authorization",command+"-"+subc, tan);
		return;
	}
}

void JsonAPI::handleNotImplemented()
{
	sendErrorReply("Command not implemented");
}

void JsonAPI::sendSuccessReply(const QString &command, const int tan)
{
	// create reply
	QJsonObject reply;
	reply["success"] = true;
	reply["command"] = command;
	reply["tan"] = tan;

	// send reply
	emit callbackMessage(reply);
}

void JsonAPI::sendSuccessDataReply(const QJsonDocument &doc, const QString &command, const int &tan)
{
	QJsonObject reply;
	reply["success"] = true;
	reply["command"] = command;
	reply["tan"] = tan;
	if(doc.isArray())
		reply["info"] = doc.array();
	else
		reply["info"] = doc.object();

	emit callbackMessage(reply);
}

void JsonAPI::sendErrorReply(const QString &error, const QString &command, const int tan)
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

void JsonAPI::streamLedcolorsUpdate(const std::vector<ColorRgb>& ledColors)
{
	QJsonObject result;
	QJsonArray leds;

	for(const auto & color : ledColors)
	{
		leds << QJsonValue(color.red) << QJsonValue(color.green) << QJsonValue(color.blue);
	}

	result["leds"] = leds;
	_streaming_leds_reply["result"] = result;

	// send the result
	emit callbackMessage(_streaming_leds_reply);
}

void JsonAPI::setImage(const Image<ColorRgb> & image)
{
	QImage jpgImage((const uint8_t *) image.memptr(), image.width(), image.height(), 3*image.width(), QImage::Format_RGB888);
	QByteArray ba;
	QBuffer buffer(&ba);
	buffer.open(QIODevice::WriteOnly);
	jpgImage.save(&buffer, "jpg");

	QJsonObject result;
	result["image"] = "data:image/jpg;base64,"+QString(ba.toBase64());
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
		QVector<Logger::T_LOG_MESSAGE>* logBuffer = LoggerManager::getInstance()->getLogMessageBuffer();
		for(int i=0; i<logBuffer->length(); i++)
		{
			message["appName"] = logBuffer->at(i).appName;
			message["loggerName"] = logBuffer->at(i).loggerName;
			message["function"] = logBuffer->at(i).function;
			message["line"] = QString::number(logBuffer->at(i).line);
			message["fileName"] = logBuffer->at(i).fileName;
			message["message"] = logBuffer->at(i).message;
			message["levelString"] = logBuffer->at(i).levelString;

			messageArray.append(message);
		}
	}
	else
	{
		message["appName"] = msg.appName;
		message["loggerName"] = msg.loggerName;
		message["function"] = msg.function;
		message["line"] = QString::number(msg.line);
		message["fileName"] = msg.fileName;
		message["message"] = msg.message;
		message["levelString"] = msg.levelString;

		messageArray.append(message);
	}

	result.insert("messages", messageArray);
	_streaming_logging_reply["result"] = result;

	// send the result
	emit callbackMessage(_streaming_logging_reply);
}

void JsonAPI::handlePendingTokenRequest(const QString& id, const QString& comment)
{
	// just user sessions are allowed to react on this, to prevent that token authorized instances authorize new tokens on their own
	if(_userAuthorized)
	{
		QJsonObject obj;
		obj["command"] = "authorize-event";
		obj["comment"] = comment;
		obj["id"] = id;

		emit callbackMessage(obj);
	}
}

void JsonAPI::handleTokenResponse(const bool& success, QObject* caller, const QString& token, const QString& comment, const QString& id)
{
	// if this is the requester, we send the reply
	if(this == caller)
	{
		const QString cmd = "authorize-requestToken";
		QJsonObject result;
		result["token"] = token;
		result["comment"] = comment;
		result["id"] = id;

		if(success)
			sendSuccessDataReply(QJsonDocument(result), cmd);
		else
			sendErrorReply("Token request timeout or denied", cmd);
	}
}

void JsonAPI::handleInstanceStateChange(const instanceState& state, const quint8& instance, const QString& name)
{
	switch(state){
		case H_ON_STOP:
			if(_hyperion->getInstanceIndex() == instance)
			{
				handleInstanceSwitch();
			}
			break;
		default:
			break;
	}
}

void JsonAPI::stopDataConnections(void)
{
	LoggerManager::getInstance()->disconnect();
	_streaming_logging_activated = false;
	_jsonCB->resetSubscriptions();
	_imageStreamTimer->stop();
	_ledStreamTimer->stop();

}
