// project includes
#include <api/JsonAPI.h>
#include <api/JsonInfo.h>

// Qt includes
#include <QResource>
#include <QDateTime>
#include <QImage>
#include <QBuffer>
#include <QByteArray>
#include <QTimer>
#include <QHostInfo>
#include <QMultiMap>
#include <QRegularExpression>
#include <QStringList>

// hyperion includes
#include <leddevice/LedDeviceWrapper.h>
#include <leddevice/LedDevice.h>
#include <leddevice/LedDeviceFactory.h>

#include <HyperionConfig.h> // Required to determine the cmake options

#include <utils/WeakConnect.h>
#include <events/EventEnum.h>

#include <utils/jsonschema/QJsonFactory.h>
#include <utils/jsonschema/QJsonSchemaChecker.h>
#include <utils/ColorSys.h>
#include <utils/Process.h>
#include <utils/JsonUtils.h>

// ledmapping int <> string transform methods
#include <hyperion/ImageProcessor.h>

// api includes
#include <api/JsonCallbacks.h>
#include <events/EventHandler.h>

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

#include <chrono>
#include <utility>

using namespace hyperion;

// Constants
namespace {

constexpr std::chrono::milliseconds NEW_TOKEN_REQUEST_TIMEOUT{ 180000 };

const char TOKEN_TAG[] = "token";
constexpr int TOKEN_TAG_LENGTH = sizeof(TOKEN_TAG) - 1;
const char BEARER_TOKEN_TAG[] = "Bearer";
constexpr int BEARER_TOKEN_TAG_LENGTH = sizeof(BEARER_TOKEN_TAG) - 1;

const int MIN_PASSWORD_LENGTH = 8;
const int APP_TOKEN_LENGTH = 36;

const bool verbose = false;
}

JsonAPI::JsonAPI(QString peerAddress, Logger *log, bool localConnection, QObject *parent, bool noListener)
	: API(log, localConnection, parent)
	,_noListener(noListener)
	,_peerAddress (std::move(peerAddress))
	,_jsonCB (nullptr)
{
	Q_INIT_RESOURCE(JSONRPC_schemas);

	qRegisterMetaType<Event>("Event");
	_jsonCB = QSharedPointer<JsonCallbacks>(new JsonCallbacks( _log, _peerAddress, parent));
}

void JsonAPI::initialize()
{
	// init API, REQUIRED!
	API::init();

	// setup auth interface
	connect(this, &API::onPendingTokenRequest, this, &JsonAPI::issueNewPendingTokenRequest);
	connect(this, &API::onTokenResponse, this, &JsonAPI::handleTokenResponse);

	// listen for killed instances
	connect(_instanceManager, &HyperionIManager::instanceStateChanged, this, &JsonAPI::handleInstanceStateChange);

	// pipe callbacks from subscriptions to parent
	connect(_jsonCB.data(), &JsonCallbacks::newCallback, this, &JsonAPI::callbackMessage);

	// notify hyperion about a jsonMessageForward
	if (_hyperion != nullptr)
	{
		// Initialise jsonCB with current instance
		_jsonCB->setSubscriptionsTo(_hyperion);
		connect(this, &JsonAPI::forwardJsonMessage, _hyperion, &Hyperion::forwardJsonMessage);
	}

	//notify eventhadler on suspend/resume/idle requests
	connect(this, &JsonAPI::signalEvent, EventHandler::getInstance().data(), &EventHandler::handleEvent);
}

bool JsonAPI::handleInstanceSwitch(quint8 inst, bool /*forced*/)
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

	//parse the message
	QPair<bool, QStringList> parsingResult = JsonUtils::parse(ident, messageString, message, _log);
	if (!parsingResult.first)
	{
		//Try to find command and tan, even parsing failed
		QString command = findCommand(messageString);
		int tan = findTan(messageString);

		sendErrorReply("Parse error", parsingResult.second, command, tan);
		return;
	}

	DebugIf(verbose, _log, "message: [%s]", QJsonDocument(message).toJson(QJsonDocument::Compact).constData() );

	// check specific message
	const QString command = message.value("command").toString();
	const QString subCommand = message.value("subcommand").toString();

	int tan {0};
	if (message.value("tan") != QJsonValue::Undefined)
	{
		tan = message["tan"].toInt();
	}

	// check basic message
	QJsonObject schemaJson = QJsonFactory::readSchema(":schema");
	QPair<bool, QStringList> validationResult = JsonUtils::validate(ident, message, schemaJson, _log);
	if (!validationResult.first)
	{
		sendErrorReply("Invalid command", validationResult.second, command, tan);
		return;
	}

	JsonApiCommand cmd = ApiCommandRegister::getCommandInfo(command, subCommand);
	cmd.tan = tan;

	if (cmd.command == Command::Unknown)
	{
		const QStringList errorDetails (subCommand.isEmpty() ? "subcommand is missing" : QString("Invalid subcommand: %1").arg(subCommand));
		sendErrorReply("Invalid command", errorDetails, command, tan);
		return;
	}

	if (_noListener)
	{
		setAuthorization(false);
		if(cmd.isNolistenerCmd == NoListenerCmd::No)
		{
			sendErrorReply("Command not supported via single API calls using HTTP/S", cmd);
			return;
		}

		// Check authorization for HTTP requests
		if (!httpAuthHeader.isEmpty())
		{
			int bearTokenLenght {0};
			if (httpAuthHeader.startsWith(BEARER_TOKEN_TAG, Qt::CaseInsensitive)) {
				bearTokenLenght = BEARER_TOKEN_TAG_LENGTH;
			}
			else if (httpAuthHeader.startsWith(TOKEN_TAG, Qt::CaseInsensitive)) {
				bearTokenLenght = TOKEN_TAG_LENGTH;
			}

			if (bearTokenLenght == 0)
			{
				sendErrorReply("No bearer token found in Authorization header", cmd);
				return;
			}

			QString cToken =httpAuthHeader.mid(bearTokenLenght).trimmed();
			API::isTokenAuthorized(cToken); // _authorized && _adminAuthorized are set
		}

		if (islocalConnection() && !_authManager->isLocalAuthRequired())
		{
			// if the request comes via a local network connection, plus authorization is disabled for local request,
			// no token authorization is required for non-admin requests
			setAuthorization(true);
		}
	}

	if (cmd.authorization != Authorization::No )
	{
		if (!isAuthorized() || (cmd.authorization == Authorization::Admin && !isAdminAuthorized()))
		{
			sendNoAuthorization(cmd);
			return;
		}
	}

	schemaJson = QJsonFactory::readSchema(QString(":schema-%1").arg(command));
	validationResult = JsonUtils::validate(ident, message, schemaJson, _log);
	if (!validationResult.first)
	{
		sendErrorReply("Invalid params", validationResult.second, cmd);
		return;
	}

	if (_hyperion == nullptr)
	{
		sendErrorReply("Service Unavailable", cmd);
		return;
	}

	if (!message.contains("instance") || cmd.isInstanceCmd == InstanceCmd::No)
	{
		handleCommand(cmd, message);
	}
	else
	{
		handleInstanceCommand(cmd, message);
	}
}

void JsonAPI::handleInstanceCommand(const JsonApiCommand& cmd, const QJsonObject &message)
{
	const QJsonValue instanceElement = message.value("instance");
	QJsonArray instances;
	if (instanceElement.isDouble())
	{
		instances.append(instanceElement);
	} else if (instanceElement.isArray())
	{
		instances = instanceElement.toArray();
	}

	QList<quint8> runningInstanceIdxs = _instanceManager->getRunningInstanceIdx();

	QList<quint8> instanceIdxList;
	QStringList errorDetails;
	if (instances.contains("all"))
	{
		for (const auto& instanceIdx : runningInstanceIdxs)
		{
			instanceIdxList.append(instanceIdx);
		}
	}
	else
	{
		for (const auto &instance : std::as_const(instances)) {

			quint8 instanceIdx = static_cast<quint8>(instance.toInt());
			if (instance.isDouble() && runningInstanceIdxs.contains(instanceIdx))
			{
				instanceIdxList.append(instanceIdx);
			}
			else
			{
				errorDetails.append("Not a running or valid instance: " + instance.toVariant().toString());
			}
		}
	}

	if (instanceIdxList.isEmpty() || !errorDetails.isEmpty() )
	{
		sendErrorReply("Invalid instance(s) given", errorDetails, cmd);
		return;
	}

	quint8 currentInstanceIdx = getCurrentInstanceIndex();
	if (instanceIdxList.size() > 1)
	{
		if (cmd.isInstanceCmd != InstanceCmd::Multi)
		{
			sendErrorReply("Command does not support multiple instances", cmd);
			return;
		}
	}

	for (const auto &instanceIdx : instanceIdxList)
	{
		if (setHyperionInstance(instanceIdx))
		{
			handleCommand(cmd, message);
		}
	}

	setHyperionInstance(currentInstanceIdx);
}

void JsonAPI::handleCommand(const JsonApiCommand& cmd, const QJsonObject &message)
{
	switch (cmd.command) {
	case Command::Authorize:
		handleAuthorizeCommand(message, cmd);
	break;
	case Command::Color:
		handleColorCommand(message, cmd);
	break;
	case Command::Image:
		handleImageCommand(message, cmd);
	break;
#if defined(ENABLE_EFFECTENGINE)
	case Command::Effect:
		handleEffectCommand(message, cmd);
	break;
	case Command::CreateEffect:
		handleCreateEffectCommand(message, cmd);
	break;
	case Command::DeleteEffect:
		handleDeleteEffectCommand(message, cmd);
	break;
#endif
	case Command::SysInfo:
		handleSysInfoCommand(message, cmd);
	break;
	case Command::ServerInfo:
		handleServerInfoCommand(message, cmd);
	break;
	case Command::Clear:
		handleClearCommand(message, cmd);
	break;
	case Command::Adjustment:
		handleAdjustmentCommand(message, cmd);
	break;
	case Command::SourceSelect:
		handleSourceSelectCommand(message, cmd);
	break;
	case Command::Config:
		handleConfigCommand(message, cmd);
	break;
	case Command::ComponentState:
		handleComponentStateCommand(message, cmd);
	break;
	case Command::LedColors:
		handleLedColorsCommand(message, cmd);
	break;
	case Command::Logging:
		handleLoggingCommand(message, cmd);
	break;
	case Command::Processing:
		handleProcessingCommand(message, cmd);
	break;
	case Command::VideoMode:
		handleVideoModeCommand(message, cmd);
	break;
	case Command::Instance:
		handleInstanceCommand(message, cmd);
	break;
	case Command::LedDevice:
		handleLedDeviceCommand(message, cmd);
	break;
	case Command::InputSource:
		handleInputSourceCommand(message, cmd);
	break;
	case Command::Service:
		handleServiceCommand(message, cmd);
	break;
	case Command::System:
		handleSystemCommand(message, cmd);
	break;
	case Command::ClearAll:
		handleClearallCommand(message, cmd);
	break;
		// BEGIN | The following commands are deprecated but used to ensure backward compatibility with Hyperion Classic remote control
	case Command::Transform:
	case Command::Correction:
	case Command::Temperature:
		sendErrorReply("The command is deprecated, please use the Hyperion Web Interface to configure", cmd);
	break;
		// END
	default:
	break;
	}
}

void JsonAPI::handleColorCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	emit forwardJsonMessage(message);
	int priority = message["priority"].toInt();
	int duration = message["duration"].toInt(-1);
	const QString origin = message["origin"].toString("JsonRpc") + "@" + _peerAddress;

	const QJsonArray &jsonColor = message["color"].toArray();
	std::vector<uint8_t> colors;
	colors.reserve(static_cast<std::vector<uint8_t>::size_type>(jsonColor.size()));
	// Transform each entry in jsonColor to uint8_t and append to colors
	std::transform(jsonColor.begin(), jsonColor.end(), std::back_inserter(colors),
				   [](const QJsonValue &value) { return static_cast<uint8_t>(value.toInt()); });

	API::setColor(priority, colors, duration, origin);
	sendSuccessReply(cmd);
}

void JsonAPI::handleImageCommand(const QJsonObject &message, const JsonApiCommand& cmd)
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

	if (API::setImage(idata, COMP_IMAGE, replyMsg)) {
		sendSuccessReply(cmd);
	} else {
		sendErrorReply(replyMsg, cmd);
	}
}

#if defined(ENABLE_EFFECTENGINE)
void JsonAPI::handleEffectCommand(const QJsonObject &message, const JsonApiCommand& cmd)
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

	if (API::setEffect(dat)) {
		sendSuccessReply(cmd);
	} else {
		sendErrorReply("Effect '" + dat.effectName + "' not found", cmd);
	}
}

void JsonAPI::handleCreateEffectCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	const QString resultMsg = API::saveEffect(message);
	resultMsg.isEmpty() ? sendSuccessReply(cmd) : sendErrorReply(resultMsg, cmd);
}

void JsonAPI::handleDeleteEffectCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	const QString res = API::deleteEffect(message["name"].toString());
	res.isEmpty() ? sendSuccessReply(cmd) : sendErrorReply(res, cmd);
}
#endif

void JsonAPI::handleSysInfoCommand(const QJsonObject & /*unused*/, const JsonApiCommand& cmd)
{
	sendSuccessDataReply(JsonInfo::getSystemInfo(_hyperion), cmd);
}

void JsonAPI::handleServerInfoCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	QJsonObject info {};
	QStringList errorDetails;

	switch (cmd.getSubCommand()) {
	case SubCommand::Empty:
	case SubCommand::GetInfo:
		info["priorities"] = JsonInfo::getPrioritiestInfo(_hyperion);
		info["priorities_autoselect"] = _hyperion->sourceAutoSelectEnabled();
		info["adjustment"] = JsonInfo::getAdjustmentInfo(_hyperion, _log);
		info["ledDevices"] = JsonInfo::getAvailableLedDevices();
		info["grabbers"] = JsonInfo::getGrabbers(_hyperion);
		info["videomode"] = QString(videoMode2String(_hyperion->getCurrentVideoMode()));
		info["cec"] = JsonInfo::getCecInfo();
		info["services"] = JsonInfo::getServices();
		info["components"] = JsonInfo::getComponents(_hyperion);
		info["imageToLedMappingType"] = ImageProcessor::mappingTypeToStr(_hyperion->getLedMappingType());
		info["instance"] = JsonInfo::getInstanceInfo();
		info["leds"] = _hyperion->getSetting(settings::LEDS).array();
		info["activeLedColor"] =  JsonInfo::getActiveColors(_hyperion);

#if defined(ENABLE_EFFECTENGINE)
		info["effects"] = JsonInfo::getEffects(_hyperion);
		info["activeEffects"] = JsonInfo::getActiveEffects(_hyperion);
#endif

		// BEGIN | The following entries are deprecated but used to ensure backward compatibility with hyperion Classic or up to Hyperion 2.0.16
		info["hostname"] = QHostInfo::localHostName();
		info["transform"] = JsonInfo::getTransformationInfo(_hyperion);

		if (!_noListener && message.contains("subscribe"))
		{
			const QJsonArray &subscriptions = message["subscribe"].toArray();
			QStringList invaliCommands = _jsonCB->subscribe(subscriptions);
			if (!invaliCommands.isEmpty())
			{
				errorDetails.append("subscribe - Invalid commands provided: " +  invaliCommands.join(','));
			}
		}
		// END

	break;

	case SubCommand::Subscribe:
	case SubCommand::Unsubscribe:
	{
		const QJsonObject &params = message["data"].toObject();
		const QJsonArray &subscriptions = params["subscriptions"].toArray();
		if (subscriptions.isEmpty()) {
			sendErrorReply("Invalid params", {"No subscriptions provided"}, cmd);
			return;
		}

		QStringList invaliCommands;
		if (cmd.subCommand == SubCommand::Subscribe)
		{
			invaliCommands = _jsonCB->subscribe(subscriptions);
		}
		else
		{
			invaliCommands = _jsonCB->unsubscribe(subscriptions);
		}

		if (!invaliCommands.isEmpty())
		{
			errorDetails.append("subscriptions - Invalid commands provided: " +  invaliCommands.join(','));
		}
	}
	break;

	case SubCommand::GetSubscriptions:
		info["subscriptions"] = QJsonArray::fromStringList(_jsonCB->getSubscribedCommands());
	break;

	case SubCommand::GetSubscriptionCommands:
		info["commands"] = QJsonArray::fromStringList(_jsonCB->getCommands());
	break;

	default:
	break;
	}

	sendSuccessDataReplyWithError(info, cmd, errorDetails);
}

void JsonAPI::handleClearCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	emit forwardJsonMessage(message);
	int priority = message["priority"].toInt();
	QString replyMsg;

	if (!API::clearPriority(priority, replyMsg))
	{
		sendErrorReply(replyMsg, cmd);
		return;
	}
	sendSuccessReply(cmd);
}

void JsonAPI::handleClearallCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	emit forwardJsonMessage(message);
	QString replyMsg;
	API::clearPriority(-1, replyMsg);
	sendSuccessReply(cmd);
}

void JsonAPI::handleAdjustmentCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	const QJsonObject &adjustment = message["adjustment"].toObject();

	const QList<QString> adjustmentIds = _hyperion->getAdjustmentIds();
	if (adjustmentIds.isEmpty()) {
		sendErrorReply("No adjustment data available", cmd);
		return;
	}

	const QString adjustmentId = adjustment["id"].toString(adjustmentIds.first());
	ColorAdjustment *colorAdjustment = _hyperion->getAdjustment(adjustmentId);
	if (colorAdjustment == nullptr) {
		Warning(_log, "Incorrect adjustment identifier: %s", adjustmentId.toStdString().c_str());
		return;
	}

	applyColorAdjustments(adjustment, colorAdjustment);
	applyTransforms(adjustment, colorAdjustment);
	_hyperion->adjustmentsUpdated();
	sendSuccessReply(cmd);
}

void JsonAPI::applyColorAdjustments(const QJsonObject &adjustment, ColorAdjustment *colorAdjustment)
{
	applyColorAdjustment("red", adjustment, colorAdjustment->_rgbRedAdjustment);
	applyColorAdjustment("green", adjustment, colorAdjustment->_rgbGreenAdjustment);
	applyColorAdjustment("blue", adjustment, colorAdjustment->_rgbBlueAdjustment);
	applyColorAdjustment("cyan", adjustment, colorAdjustment->_rgbCyanAdjustment);
	applyColorAdjustment("magenta", adjustment, colorAdjustment->_rgbMagentaAdjustment);
	applyColorAdjustment("yellow", adjustment, colorAdjustment->_rgbYellowAdjustment);
	applyColorAdjustment("white", adjustment, colorAdjustment->_rgbWhiteAdjustment);
}

void JsonAPI::applyColorAdjustment(const QString &colorName, const QJsonObject &adjustment, RgbChannelAdjustment &rgbAdjustment)
{
	if (adjustment.contains(colorName)) {
		const QJsonArray &values = adjustment[colorName].toArray();
		if (values.size() >= 3) {
			rgbAdjustment.setAdjustment(static_cast<uint8_t>(values[0U].toInt()),
					static_cast<uint8_t>(values[1U].toInt()),
					static_cast<uint8_t>(values[2U].toInt()));
		}
	}
}

void JsonAPI::applyTransforms(const QJsonObject &adjustment, ColorAdjustment *colorAdjustment)
{
	applyGammaTransform("gammaRed", adjustment, colorAdjustment->_rgbTransform, 'r');
	applyGammaTransform("gammaGreen", adjustment, colorAdjustment->_rgbTransform, 'g');
	applyGammaTransform("gammaBlue", adjustment, colorAdjustment->_rgbTransform, 'b');
	applyTransform("backlightThreshold", adjustment, colorAdjustment->_rgbTransform, &RgbTransform::setBacklightThreshold);
	applyTransform("backlightColored", adjustment, colorAdjustment->_rgbTransform, &RgbTransform::setBacklightColored);
	applyTransform("brightness", adjustment, colorAdjustment->_rgbTransform, &RgbTransform::setBrightness);
	applyTransform("brightnessCompensation", adjustment, colorAdjustment->_rgbTransform, &RgbTransform::setBrightnessCompensation);
	applyTransform("saturationGain", adjustment, colorAdjustment->_okhsvTransform, &OkhsvTransform::setSaturationGain);
	applyTransform("brightnessGain", adjustment, colorAdjustment->_okhsvTransform, &OkhsvTransform::setBrightnessGain);
}

void JsonAPI::applyGammaTransform(const QString &transformName, const QJsonObject &adjustment, RgbTransform &rgbTransform, char channel)
{
	if (adjustment.contains(transformName)) {
		rgbTransform.setGamma(channel == 'r' ? adjustment[transformName].toDouble() : rgbTransform.getGammaR(),
							  channel == 'g' ? adjustment[transformName].toDouble() : rgbTransform.getGammaG(),
							  channel == 'b' ? adjustment[transformName].toDouble() : rgbTransform.getGammaB());
	}
}

template<typename T>
void JsonAPI::applyTransform(const QString &transformName, const QJsonObject &adjustment, T &transform, void (T::*setFunction)(bool))
{
	if (adjustment.contains(transformName)) {
		(transform.*setFunction)(adjustment[transformName].toBool());
	}
}

template<typename T>
void JsonAPI::applyTransform(const QString &transformName, const QJsonObject &adjustment, T &transform, void (T::*setFunction)(double))
{
	if (adjustment.contains(transformName)) {
		(transform.*setFunction)(adjustment[transformName].toDouble());
	}
}

template<typename T>
void JsonAPI::applyTransform(const QString &transformName, const QJsonObject &adjustment, T &transform, void (T::*setFunction)(uint8_t))
{
	if (adjustment.contains(transformName)) {
		(transform.*setFunction)(static_cast<uint8_t>(adjustment[transformName].toInt()));
	}
}

void JsonAPI::handleSourceSelectCommand(const QJsonObject &message, const JsonApiCommand& cmd)
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
		sendErrorReply("Priority request is invalid", cmd);
		return;
	}
	sendSuccessReply(cmd);
}

void JsonAPI::handleConfigCommand(const QJsonObject& message, const JsonApiCommand& cmd)
{
	switch (cmd.subCommand) {
	case SubCommand::GetSchema:
		handleSchemaGetCommand(message, cmd);
	break;

	case SubCommand::GetConfig:
		sendSuccessDataReply(_hyperion->getQJsonConfig(), cmd);
	break;

	case SubCommand::SetConfig:
		handleConfigSetCommand(message, cmd);
	break;

	case SubCommand::RestoreConfig:
		handleConfigRestoreCommand(message, cmd);
	break;

	case SubCommand::Reload:
		Debug(_log, "Restarting due to RPC command");
		emit signalEvent(Event::Reload);
		sendSuccessReply(cmd);
	break;

	default:
	break;
	}
}

void JsonAPI::handleConfigSetCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	if (message.contains("config"))
	{
		QJsonObject config = message["config"].toObject();
		if (API::isHyperionEnabled())
		{
			if ( API::saveSettings(config) ) {
				sendSuccessReply(cmd);
			} else {
				sendErrorReply("Save settings failed", cmd);
			}
		}
		else
		{
			sendErrorReply("Saving configuration while Hyperion is disabled isn't possible", cmd);
		}
	}
}

void JsonAPI::handleConfigRestoreCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	if (message.contains("config"))
	{
		QJsonObject config = message["config"].toObject();
		if (API::isHyperionEnabled())
		{
			if ( API::restoreSettings(config) )
			{
				sendSuccessReply(cmd);
			}
			else
			{
				sendErrorReply("Restore settings failed", cmd);
			}
		}
		else
		{
			sendErrorReply("Restoring configuration while Hyperion is disabled is not possible", cmd);
		}
	}
}

void JsonAPI::handleSchemaGetCommand(const QJsonObject& /*message*/, const JsonApiCommand& cmd)
{
	// create result
	QJsonObject schemaJson;
	QJsonObject alldevices;
	QJsonObject properties;

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
	sendSuccessDataReply(schemaJson, cmd);
}

void JsonAPI::handleComponentStateCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	const QJsonObject &componentState = message["componentstate"].toObject();
	QString comp = componentState["component"].toString("invalid");
	bool compState = componentState["state"].toBool(true);
	QString replyMsg;

	if (API::setComponentState(comp, compState, replyMsg)) {
		sendSuccessReply(cmd);
	} else {
		sendErrorReply(replyMsg, cmd);
	}
}

void JsonAPI::handleLedColorsCommand(const QJsonObject& /*message*/, const JsonApiCommand& cmd)
{
	switch (cmd.subCommand) {
	case SubCommand::LedStreamStart:
		_jsonCB->subscribe( Subscription::LedColorsUpdate);
		// push once
		_hyperion->update();
		sendSuccessReply(cmd);
	break;

	case SubCommand::LedStreamStop:
		_jsonCB->unsubscribe( Subscription::LedColorsUpdate);
		sendSuccessReply(cmd);
	break;

	case SubCommand::ImageStreamStart:
		_jsonCB->subscribe(Subscription::ImageUpdate);
		sendSuccessReply(cmd);
	break;

	case SubCommand::ImageStreamStop:
		_jsonCB->unsubscribe(Subscription::ImageUpdate);
		sendSuccessReply(cmd);
	break;

	default:
	break;
	}
}

void JsonAPI::handleLoggingCommand(const QJsonObject& /*message*/, const JsonApiCommand& cmd)
{
	switch (cmd.subCommand) {
	case SubCommand::Start:
		_jsonCB->subscribe("logmsg-update");
		sendSuccessReply(cmd);
	break;

	case SubCommand::Stop:
		_jsonCB->unsubscribe("logmsg-update");
		sendSuccessReply(cmd);
	break;
	default:
	break;
	}
}

void JsonAPI::handleProcessingCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	API::setLedMappingType(ImageProcessor::mappingTypeToInt(message["mappingType"].toString("multicolor_mean")));
	sendSuccessReply(cmd);
}

void JsonAPI::handleVideoModeCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	API::setVideoMode(parse3DMode(message["videoMode"].toString("2D")));
	sendSuccessReply(cmd);
}

void JsonAPI::handleAuthorizeCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	switch (cmd.subCommand) {
	case SubCommand::TokenRequired:
		handleTokenRequired(cmd);
	break;
	case SubCommand::AdminRequired:
		handleAdminRequired(cmd);
	break;
	case SubCommand::NewPasswordRequired:
		handleNewPasswordRequired(cmd);
	break;
	case SubCommand::Logout:
		handleLogout(cmd);
	break;
	case SubCommand::NewPassword:
		handleNewPassword(message, cmd);
	break;
	case SubCommand::CreateToken:
		handleCreateToken(message, cmd);
	break;
	case SubCommand::RenameToken:
		handleRenameToken(message, cmd);
	break;
	case SubCommand::DeleteToken:
		handleDeleteToken(message, cmd);
	break;
	case SubCommand::RequestToken:
		handleRequestToken(message, cmd);
	break;
	case SubCommand::GetPendingTokenRequests:
		handleGetPendingTokenRequests(cmd);
	break;
	case SubCommand::AnswerRequest:
		handleAnswerRequest(message, cmd);
	break;
	case SubCommand::GetTokenList:
		handleGetTokenList(cmd);
	break;
	case SubCommand::Login:
		handleLogin(message, cmd);
	break;
	default:
	return;
	}
}

void JsonAPI::handleTokenRequired(const JsonApiCommand& cmd)
{
	bool isTokenRequired = !islocalConnection() || _authManager->isLocalAuthRequired();
	QJsonObject response { { "required", isTokenRequired} };
	sendSuccessDataReply(response, cmd);
}

void JsonAPI::handleAdminRequired(const JsonApiCommand& cmd)
{
	bool isAdminAuthRequired = true;
	QJsonObject response { { "adminRequired", isAdminAuthRequired} };
	sendSuccessDataReply(response, cmd);
}

void JsonAPI::handleNewPasswordRequired(const JsonApiCommand& cmd)
{
	QJsonObject response { { "newPasswordRequired", API::hasHyperionDefaultPw() } };
	sendSuccessDataReply(response, cmd);
}

void JsonAPI::handleLogout(const JsonApiCommand& cmd)
{
	API::logout();
	sendSuccessReply(cmd);
}

void JsonAPI::handleNewPassword(const QJsonObject &message, const JsonApiCommand& cmd)
{
	const QString password = message["password"].toString().trimmed();
	const QString newPassword = message["newPassword"].toString().trimmed();
	if (API::updateHyperionPassword(password, newPassword)) {
		sendSuccessReply(cmd);
	} else {
		sendErrorReply("Failed to update user password", cmd);
	}
}

void JsonAPI::handleCreateToken(const QJsonObject &message, const JsonApiCommand& cmd)
{
	const QString &comment = message["comment"].toString().trimmed();
	AuthManager::AuthDefinition def;
	const QString createTokenResult = API::createToken(comment, def);
	if (createTokenResult.isEmpty()) {
		QJsonObject newTok;
		newTok["comment"] = def.comment;
		newTok["id"] = def.id;
		newTok["token"] = def.token;

		sendSuccessDataReply(newTok, cmd);
	} else {
		sendErrorReply("Token creation failed", {createTokenResult}, cmd);
	}
}

void JsonAPI::handleRenameToken(const QJsonObject &message, const JsonApiCommand& cmd)
{
	const QString &identifier = message["id"].toString().trimmed();
	const QString &comment = message["comment"].toString().trimmed();
	const QString renameTokenResult = API::renameToken(identifier, comment);
	if (renameTokenResult.isEmpty()) {
		sendSuccessReply(cmd);
	} else {
		sendErrorReply("Token rename failed", {renameTokenResult}, cmd);
	}
}

void JsonAPI::handleDeleteToken(const QJsonObject &message, const JsonApiCommand& cmd)
{
	const QString &identifier = message["id"].toString().trimmed();
	const QString deleteTokenResult = API::deleteToken(identifier);
	if (deleteTokenResult.isEmpty()) {
		sendSuccessReply(cmd);
	} else {
		sendErrorReply("Token deletion failed", {deleteTokenResult}, cmd);
	}
}

void JsonAPI::handleRequestToken(const QJsonObject &message, const JsonApiCommand& cmd)
{
	const QString &identifier = message["id"].toString().trimmed();
	const QString &comment = message["comment"].toString().trimmed();
	const bool &acc = message["accept"].toBool(true);
	if (acc) {
		API::setNewTokenRequest(comment, identifier, cmd.tan);
	} else {
		API::cancelNewTokenRequest(comment, identifier);
		// client should wait for answer
	}
}

void JsonAPI::handleGetPendingTokenRequests(const JsonApiCommand& cmd)
{
	QVector<AuthManager::AuthDefinition> vec;
	if (API::getPendingTokenRequests(vec)) {
		QJsonArray pendingTokeRequests;
		for (const auto &entry : std::as_const(vec))
		{
			QJsonObject obj;
			obj["comment"] = entry.comment;
			obj["id"] = entry.id;
			obj["timeout"] = int(entry.timeoutTime);
			obj["tan"] = entry.tan;
			pendingTokeRequests.append(obj);
		}
		sendSuccessDataReply(pendingTokeRequests, cmd);
	}
}

void JsonAPI::handleAnswerRequest(const QJsonObject &message, const JsonApiCommand& cmd)
{
	const QString &identifier = message["id"].toString().trimmed();
	const bool &accept = message["accept"].toBool(false);
	if (API::handlePendingTokenRequest(identifier, accept)) {
		sendSuccessReply(cmd);
	} else {
		sendErrorReply("Unable to handle token acceptance or denial", cmd);
	}
}

void JsonAPI::handleGetTokenList(const JsonApiCommand& cmd)
{
	QVector<AuthManager::AuthDefinition> defVect;
	if (API::getTokenList(defVect))
	{
		QJsonArray tokenList;
		for (const auto &entry : std::as_const(defVect))
		{
			QJsonObject token;
			token["comment"] = entry.comment;
			token["id"] = entry.id;
			token["last_use"] = entry.lastUse;

			tokenList.append(token);
		}
		sendSuccessDataReply(tokenList, cmd);
	}
}

void JsonAPI::handleLogin(const QJsonObject &message, const JsonApiCommand& cmd)
{
	const QString &token = message["token"].toString().trimmed();
	if (!token.isEmpty())
	{
		// userToken is longer than app token
		if (token.size() > APP_TOKEN_LENGTH)
		{
			if (API::isUserTokenAuthorized(token)) {
				sendSuccessReply(cmd);
			} else {
				sendNoAuthorization(cmd);
			}

			return;
		}

		if (token.size() == APP_TOKEN_LENGTH)
		{
			if (API::isTokenAuthorized(token)) {
				sendSuccessReply(cmd);
			} else {
				sendNoAuthorization(cmd);
			}
		}
		return;
	}

	// password
	const QString &password = message["password"].toString().trimmed();
	if (password.size() >= MIN_PASSWORD_LENGTH)
	{
		QString userTokenRep;
		if (API::isUserAuthorized(password) && API::getUserToken(userTokenRep))
		{
			// Return the current valid Hyperion user token
			QJsonObject response { { "token", userTokenRep } };
			sendSuccessDataReply(response, cmd);
		}
		else
		{
			sendNoAuthorization(cmd);
		}
	}
	else
	{
		sendErrorReply(QString("Password is too short. Minimum length: %1 characters").arg(MIN_PASSWORD_LENGTH), cmd);
	}
}

void JsonAPI::issueNewPendingTokenRequest(const QString &identifier, const QString &comment)
{
	QJsonObject tokenRequest;
	tokenRequest["comment"] = comment;
	tokenRequest["id"] = identifier;
	tokenRequest["timeout"] = static_cast<int>(NEW_TOKEN_REQUEST_TIMEOUT.count());

	sendNewRequest(tokenRequest, "authorize-tokenRequest");
}

void JsonAPI::handleTokenResponse(bool success, const QString &token, const QString &comment, const QString &identifier, const int &tan)
{
	const QString cmd = "authorize-requestToken";
	QJsonObject result;
	result["token"] = token;
	result["comment"] = comment;
	result["id"] = identifier;

	if (success) {
		sendSuccessDataReply(result, cmd, tan);
	} else {
		sendErrorReply("Token request timeout or denied", {}, cmd, tan);
	}
}

void JsonAPI::handleInstanceCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	QString replyMsg;

	const quint8 &inst = static_cast<quint8>(message["instance"].toInt());
	const QString &name = message["name"].toString();

	switch (cmd.subCommand) {
	case SubCommand::SwitchTo:
		if (handleInstanceSwitch(inst))
		{
			QJsonObject response { { "instance", inst } };
			sendSuccessDataReply(response, cmd);
		}
		else
		{
			sendErrorReply("Selected Hyperion instance is not running", cmd);
		}
	break;

	case SubCommand::StartInstance:
		//Only send update once
		weakConnect(this, &API::onStartInstanceResponse, [this, cmd] ()
		{
			sendSuccessReply(cmd);
		});

		if (!API::startInstance(inst, cmd.tan))
		{
			sendErrorReply("Cannot start Hyperion instance index " + QString::number(inst), cmd);
		}
	break;
	case SubCommand::StopInstance:
		// silent fail
		API::stopInstance(inst);
		sendSuccessReply(cmd);
	break;

	case SubCommand::DeleteInstance:
		handleConfigRestoreCommand(message, cmd);
		if (API::deleteInstance(inst, replyMsg))
		{
			sendSuccessReply(cmd);
		}
		else
		{
			sendErrorReply(replyMsg, cmd);
		}
	break;

	case SubCommand::CreateInstance:
	case SubCommand::SaveName:
		// create and save name requires name
		if (name.isEmpty()) {
			sendErrorReply("Name string required for this command", cmd);
			return;
		}

		if (cmd.subCommand == SubCommand::CreateInstance) {
			replyMsg = API::createInstance(name);
		} else {
			replyMsg = setInstanceName(inst, name);
		}

		if (replyMsg.isEmpty()) {
			sendSuccessReply(cmd);
		} else {
			sendErrorReply(replyMsg, cmd);
		}
	break;
	default:
	break;
	}
}

void JsonAPI::handleLedDeviceCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	const QString &devType = message["ledDeviceType"].toString().trimmed();
	const LedDeviceRegistry& ledDevices = LedDeviceWrapper::getDeviceMap();

	if (ledDevices.count(devType) == 0) {
		sendErrorReply(QString("Unknown LED-Device type: %1").arg(devType), cmd);
		return;
	}

	QJsonObject config { { "type", devType } };
	LedDevice* ledDevice = LedDeviceFactory::construct(config);

	switch (cmd.subCommand) {
	case SubCommand::Discover:
		handleLedDeviceDiscover(*ledDevice, message, cmd);
	break;
	case SubCommand::GetProperties:
		handleLedDeviceGetProperties(*ledDevice, message, cmd);
	break;
	case SubCommand::Identify:
		handleLedDeviceIdentify(*ledDevice, message, cmd);
	break;
	case SubCommand::AddAuthorization:
		handleLedDeviceAddAuthorization(*ledDevice, message, cmd);
	break;
	default:
	break;
	}

	delete ledDevice;
}

void JsonAPI::handleLedDeviceDiscover(LedDevice& ledDevice, const QJsonObject& message, const JsonApiCommand& cmd)
{
	const QJsonObject &params = message["params"].toObject();
	const QJsonObject devicesDiscovered = ledDevice.discover(params);
	Debug(_log, "response: [%s]", QJsonDocument(devicesDiscovered).toJson(QJsonDocument::Compact).constData() );
	sendSuccessDataReply(devicesDiscovered, cmd);
}

void JsonAPI::handleLedDeviceGetProperties(LedDevice& ledDevice, const QJsonObject& message, const JsonApiCommand& cmd)
{
	const QJsonObject &params = message["params"].toObject();
	const QJsonObject deviceProperties = ledDevice.getProperties(params);
	Debug(_log, "response: [%s]", QJsonDocument(deviceProperties).toJson(QJsonDocument::Compact).constData() );
	sendSuccessDataReply(deviceProperties, cmd);
}

void JsonAPI::handleLedDeviceIdentify(LedDevice& ledDevice, const QJsonObject& message, const JsonApiCommand& cmd)
{
	const QJsonObject &params = message["params"].toObject();
	ledDevice.identify(params);
	sendSuccessReply(cmd);
}

void JsonAPI::handleLedDeviceAddAuthorization(LedDevice& ledDevice, const QJsonObject& message, const JsonApiCommand& cmd)
{
	const QJsonObject& params = message["params"].toObject();
	const QJsonObject response = ledDevice.addAuthorization(params);
	sendSuccessDataReply(response, cmd);
}

void JsonAPI::handleInputSourceCommand(const QJsonObject& message, const JsonApiCommand& cmd) {
	const QString& sourceType = message["sourceType"].toString().trimmed();
	const QStringList sourceTypes {"screen", "video", "audio"};

	if (!sourceTypes.contains(sourceType)) {
		sendErrorReply(QString("Unknown input source type: %1").arg(sourceType), cmd);
		return;
	}

	if (cmd.subCommand == SubCommand::Discover) {

		const QJsonObject& params = message["params"].toObject();
		QJsonObject inputSourcesDiscovered = JsonInfo().discoverSources(sourceType, params);

		DebugIf(verbose, _log, "response: [%s]", QJsonDocument(inputSourcesDiscovered).toJson(QJsonDocument::Compact).constData());

		sendSuccessDataReply(inputSourcesDiscovered, cmd);
	}
}

void JsonAPI::handleServiceCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	if (cmd.subCommand == SubCommand::Discover)
	{
		QByteArray serviceType;
		const QString type = message["serviceType"].toString().trimmed();
#ifdef ENABLE_MDNS
		QString discoveryMethod("mDNS");
		serviceType = MdnsServiceRegister::getServiceType(type);
#else
		QString discoveryMethod("ssdp");
#endif
		if (!serviceType.isEmpty())
		{
			QJsonArray serviceList;
#ifdef ENABLE_MDNS
			QMetaObject::invokeMethod(MdnsBrowser::getInstance().data(), "browseForServiceType",
									  Qt::QueuedConnection, Q_ARG(QByteArray, serviceType));

			serviceList = MdnsBrowser::getInstance().data()->getServicesDiscoveredJson(serviceType, MdnsServiceRegister::getServiceNameFilter(type), DEFAULT_DISCOVER_TIMEOUT);
#endif
			QJsonObject servicesDiscovered;
			QJsonObject servicesOfType;

			servicesOfType.insert(type, serviceList);

			servicesDiscovered.insert("discoveryMethod", discoveryMethod);
			servicesDiscovered.insert("services", servicesOfType);

			sendSuccessDataReply(servicesDiscovered, cmd);
		}
		else
		{
			sendErrorReply(QString("Discovery of service type [%1] via %2 not supported").arg(type, discoveryMethod), cmd);
		}
	}
}

void JsonAPI::handleSystemCommand(const QJsonObject& /*message*/, const JsonApiCommand& cmd)
{
	switch (cmd.subCommand) {
	case SubCommand::Suspend:
		emit signalEvent(Event::Suspend);
	break;
	case SubCommand::Resume:
		emit signalEvent(Event::Resume);
	break;
	case SubCommand::Restart:
		emit signalEvent(Event::Restart);
	break;
	case SubCommand::ToggleSuspend:
		emit signalEvent(Event::ToggleSuspend);
	break;
	case SubCommand::Idle:
		emit signalEvent(Event::Idle);
	break;
	case SubCommand::ToggleIdle:
		emit signalEvent(Event::ToggleIdle);
	break;
	default:
	return;
	}
	sendSuccessReply(cmd);
}

QJsonObject JsonAPI::getBasicCommandReply(bool success, const QString &command, int tan, InstanceCmd::Type isInstanceCmd) const
{
	QJsonObject reply;
	reply["success"] = success;
	reply["command"] = command;
	reply["tan"] = tan;

	if (isInstanceCmd == InstanceCmd::Yes || ( isInstanceCmd == InstanceCmd::Multi && !_noListener))
	{
		reply["instance"] = _hyperion->getInstanceIndex();
	}
	return reply;
}

void JsonAPI::sendSuccessReply(const JsonApiCommand& cmd)
{
	sendSuccessReply(cmd.toString(), cmd.tan, cmd.isInstanceCmd);
}

void JsonAPI::sendSuccessReply(const QString &command, int tan, InstanceCmd::Type isInstanceCmd)
{
	emit callbackMessage(getBasicCommandReply(true, command, tan , isInstanceCmd));
}

void JsonAPI::sendSuccessDataReply(const QJsonValue &infoData, const JsonApiCommand& cmd)
{
	sendSuccessDataReplyWithError(infoData, cmd.toString(), cmd.tan, {}, cmd.isInstanceCmd);
}

void JsonAPI::sendSuccessDataReply(const QJsonValue &infoData, const QString &command, int tan, InstanceCmd::Type isInstanceCmd)
{
	sendSuccessDataReplyWithError(infoData, command, tan, {}, isInstanceCmd);
}

void JsonAPI::sendSuccessDataReplyWithError(const QJsonValue &infoData, const JsonApiCommand& cmd, const QStringList& errorDetails)
{
	sendSuccessDataReplyWithError(infoData, cmd.toString(), cmd.tan, errorDetails, cmd.isInstanceCmd);
}

void JsonAPI::sendSuccessDataReplyWithError(const QJsonValue &infoData, const QString &command, int tan, const QStringList& errorDetails, InstanceCmd::Type isInstanceCmd)
{
	QJsonObject reply {getBasicCommandReply(true, command, tan , isInstanceCmd)};
	reply["info"] = infoData;

	if (!errorDetails.isEmpty())
	{
		QJsonArray errorsArray;
		for (const QString& errorString : errorDetails)
		{
			QJsonObject errorObject;
			errorObject["description"] = errorString;
			errorsArray.append(errorObject);
		}
		reply["errorData"] = errorsArray;
	}

	emit callbackMessage(reply);
}

void JsonAPI::sendErrorReply(const QString &error, const JsonApiCommand& cmd)
{
	sendErrorReply(error, {}, cmd.toString(), cmd.tan, cmd.isInstanceCmd);
}

void JsonAPI::sendErrorReply(const QString &error, const QStringList& errorDetails, const JsonApiCommand& cmd)
{
	sendErrorReply(error, errorDetails, cmd.toString(), cmd.tan, cmd.isInstanceCmd);
}

void JsonAPI::sendErrorReply(const QString &error, const QStringList& errorDetails, const QString &command, int tan, InstanceCmd::Type isInstanceCmd)
{
	QJsonObject reply {getBasicCommandReply(false, command, tan , isInstanceCmd)};
	reply["error"] = error;
	if (!errorDetails.isEmpty())
	{
		QJsonArray errorsArray;
		for (const QString& errorString : errorDetails)
		{
			QJsonObject errorObject;
			errorObject["description"] = errorString;
			errorsArray.append(errorObject);
		}
		reply["errorData"] = errorsArray;
	}

	emit callbackMessage(reply);
}

void JsonAPI::sendNewRequest(const QJsonValue &infoData, const JsonApiCommand& cmd)
{
	sendSuccessDataReplyWithError(infoData, cmd.toString(), cmd.isInstanceCmd);
}

void JsonAPI::sendNewRequest(const QJsonValue &infoData, const QString &command, InstanceCmd::Type isInstanceCmd)
{
	QJsonObject request;
	request["command"] = command;

	if (isInstanceCmd != InstanceCmd::No)
	{
		request["instance"] = _hyperion->getInstanceIndex();
	}

	request["info"] = infoData;

	emit callbackMessage(request);
}

void JsonAPI::sendNoAuthorization(const JsonApiCommand& cmd)
{
	sendErrorReply(NO_AUTHORIZATION, cmd);
}

void JsonAPI::handleInstanceStateChange(InstanceState state, quint8 instance, const QString& /*name */)
{
	switch (state)
	{
	case InstanceState::H_ON_STOP:
		if (_hyperion->getInstanceIndex() == instance)
		{
			handleInstanceSwitch();
		}
	break;

	case InstanceState::H_STARTED:
	case InstanceState::H_STOPPED:
	case InstanceState::H_CREATED:
	case InstanceState::H_DELETED:
	break;
	}
}

void JsonAPI::stopDataConnections()
{
	_jsonCB->resetSubscriptions();
	LoggerManager::getInstance()->disconnect();
}

QString JsonAPI::findCommand (const QString& jsonString)
{
	QString commandValue {"unknown"};

	// Define a regular expression pattern to match the value associated with the key "command"
	static QRegularExpression regex("\"command\"\\s*:\\s*\"([^\"]+)\"");
	QRegularExpressionMatch match = regex.match(jsonString);

	if (match.hasMatch()) {
		commandValue = match.captured(1);
	}
	return commandValue;
}

int JsonAPI::findTan (const QString& jsonString)
{
	int tanValue {0};
	static QRegularExpression regex("\"tan\"\\s*:\\s*(\\d+)");
	QRegularExpressionMatch match = regex.match(jsonString);

	if (match.hasMatch()) {
		QString valueStr = match.captured(1);
		tanValue = valueStr.toInt();
	}
	return tanValue;
}
