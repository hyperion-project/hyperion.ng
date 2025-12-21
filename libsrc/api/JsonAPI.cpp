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
#include <QRegularExpression>
#include <QStringList>
#include <QEventLoop>

// project includes
#include "hyperion/SettingsManager.h"
#include <api/JsonInfo.h>

// hyperion includes
#include <leddevice/LedDeviceWrapper.h>
#include <leddevice/LedDevice.h>
#include <leddevice/LedDeviceFactory.h>

#include <HyperionConfig.h> // Required to determine the cmake options

#include <utils/WeakConnect.h>
#include <events/EventEnum.h>

#include <utils/GlobalSignals.h>
#include <utils/jsonschema/QJsonFactory.h>
#include <utils/jsonschema/QJsonSchemaChecker.h>
#include <utils/ColorSys.h>
#include <utils/KelvinToRgb.h>
#include <utils/Process.h>
#include <utils/JsonUtils.h>
#include <effectengine/EffectFileHandler.h>

// ledmapping int <> string transform methods
#include <hyperion/ImageProcessor.h>

// api includes
#include <api/JsonCallbacks.h>
#include <events/EventHandler.h>

// auth manager
#include <hyperion/AuthManager.h>
#include <db/DBConfigManager.h>

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

Q_LOGGING_CATEGORY(api_msg_request, "hyperion.api.msg.request");
Q_LOGGING_CATEGORY(api_msg_reply_success, "hyperion.api.msg.reply.success");
Q_LOGGING_CATEGORY(api_msg_reply_error, "hyperion.api.msg.reply.error");
Q_LOGGING_CATEGORY(api_msg_new, "hyperion.api.msg.new");
Q_LOGGING_CATEGORY(api_inputsource, "hyperion.api.inputsource");
Q_LOGGING_CATEGORY(api_leddevice, "hyperion.api.leddevice");

using namespace hyperion;

// Constants
namespace {

constexpr std::chrono::milliseconds NEW_TOKEN_REQUEST_TIMEOUT{ 180000 };
constexpr std::chrono::milliseconds LED_DATA_TIMEOUT { 1000 };

const char TOKEN_TAG[] = "token";
constexpr int TOKEN_TAG_LENGTH = sizeof(TOKEN_TAG) - 1;
const char BEARER_TOKEN_TAG[] = "Bearer";
constexpr int BEARER_TOKEN_TAG_LENGTH = sizeof(BEARER_TOKEN_TAG) - 1;

const int MIN_PASSWORD_LENGTH = 8;
const int APP_TOKEN_LENGTH = 36;

const char SETTINGS_UI_SCHEMA_FILE[] = ":/schema-settings-ui.json";
}

JsonAPI::JsonAPI(QString peerAddress, QSharedPointer<Logger> log, bool localConnection, bool noListener)
	: JsonAPI(peerAddress, log, localConnection, nullptr, noListener)
{
}

JsonAPI::JsonAPI(QString peerAddress, QSharedPointer<Logger> log, bool localConnection, QObject *parent, bool noListener)
	: API(log, localConnection, parent)
	,_noListener(noListener)
	,_peerAddress (std::move(peerAddress))
	,_jsonCB (nullptr)
	,_isServiceAvailable(false)
{
	TRACK_SCOPE();
	Q_INIT_RESOURCE(JSONRPC_schemas);

	qRegisterMetaType<Event>("Event");

	connect(EventHandler::getInstance().data(), &EventHandler::signalEvent, this, [log, this](const Event &event) {
		if (event == Event::Quit)
		{
			_isServiceAvailable = false;
			Info(log, "JSON-API service stopped");
		}
	});

	_jsonCB = QSharedPointer<JsonCallbacks>(new JsonCallbacks( _log, _peerAddress, parent));
}

JsonAPI::~JsonAPI()
{
	TRACK_SCOPE();
}

QSharedPointer<JsonCallbacks> JsonAPI::getCallBack() const
{
	return _jsonCB;
}

void JsonAPI::initialize()
{
	// init API, REQUIRED!
	API::init();

	// setup auth interface
	connect(this, &API::onPendingTokenRequest, this, &JsonAPI::issueNewPendingTokenRequest);
	connect(this, &API::onTokenResponse, this, &JsonAPI::handleTokenResponse);

	// listen for instance state changes via weak lock
	if (auto im = _instanceManagerWeak.toStrongRef())
	{
		connect(im.get(), &HyperionIManager::instanceStateChanged, this, &JsonAPI::handleInstanceStateChange);
	}

	//notify eventhadler on suspend/resume/idle requests
	connect(this, &JsonAPI::signalEvent, EventHandler::getInstance().data(), &EventHandler::handleEvent);

	_jsonCB->setSubscriptionsTo(_currInstanceIndex);

#if defined(ENABLE_FORWARDER)
	// notify the forwarder about a jsonMessageForward request
	QObject::connect(this, &JsonAPI::forwardJsonMessage, GlobalSignals::getInstance(), &GlobalSignals::forwardJsonMessage, Qt::UniqueConnection);
#endif

	Info(_log, "JSON-API service is ready to process requests");
	_isServiceAvailable = true;
}

bool JsonAPI::handleInstanceSwitch(quint8 instanceID, bool /*forced*/)
{
	if ( instanceID != _currInstanceIndex )
	{
		if (API::setHyperionInstance(instanceID))
		{
			Debug(_log, "Client '%s' switch to Hyperion instance %d", QSTRING_CSTR(_peerAddress), instanceID);
			// the JsonCB creates json messages you can subscribe to e.g. data change events
			_jsonCB->setSubscriptionsTo(instanceID);
			return true;
		}
		return false;
	}
	return true;
}

void JsonAPI::handleMessage(const QString &messageString, const QString &httpAuthHeader)
{
	const QString ident = "JsonRpc@" + _peerAddress;
	QJsonObject message;

	//parse the message
	auto const [parsingSuccessful, errorDetails] = JsonUtils::parse(ident, messageString, message, _log);
	if (!parsingSuccessful)
	{
		//Try to find command and tan, even parsing failed
		QString const command = findCommand(messageString);
		int const tan = findTan(messageString);

		sendErrorReply("Parse error", errorDetails, command, tan);
		return;
	}

	qCDebug(api_msg_request).noquote() << "Received message:" << JsonUtils::toCompact(message);

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
		const QStringList unknownCmdErrorDetails (subCommand.isEmpty() ? "subcommand is missing" : QString("Invalid subcommand: %1").arg(subCommand));
		sendErrorReply("Invalid command", unknownCmdErrorDetails, command, tan);
		return;
	}

	// Do not further handle requests, if service is not available
	if (!_isServiceAvailable)
	{
		sendErrorReply("Service Unavailable", cmd);
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

			QString const cToken =httpAuthHeader.mid(bearTokenLenght).trimmed();
			API::isTokenAuthorized(cToken); // _authorized && _adminAuthorized are set
		}

		if (islocalConnection())
		{
			bool localAuthRequired = true;
			if (auto auth = _authManagerWeak.toStrongRef())
			{
				localAuthRequired = auth->isLocalAuthRequired();
			}
			if (!localAuthRequired)
			{
				// if the request comes via a local network connection, plus authorization is disabled for local request,
				// no token authorization is required for non-admin requests
				setAuthorization(true);
			}
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

	if (cmd.command == Command::Config && cmd.subCommand == SubCommand::RestoreConfig)
	{
		qCDebug(api_msg_request) << "Skipping schema validation when restoring a configuration to allow repairs";
	}
	else
	{
		validationResult = JsonUtils::validate(ident, message, schemaJson, _log);

		if (!validationResult.first)
		{
			sendErrorReply("Invalid params", validationResult.second, cmd);
			return;
		}
	}

	qCDebug(api_msg_request) << "Request [" << Command::toString(cmd.command) << ", "
							 << SubCommand::toString(cmd.subCommand) << "] - Type: "
							 << InstanceCmd::toString(cmd.getInstanceCmdType()) << " - "
							 << InstanceCmd::toString(cmd.getInstanceMustRun());

	if (cmd.getInstanceCmdType() == InstanceCmd::No)
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
	QJsonArray instances;
	const QJsonValue instanceElement = message.value("instance");

	// Extract instanceIds(s) from the message
	if (!(instanceElement.isUndefined() || instanceElement.isNull()))
	{
		if (instanceElement.isDouble())
		{
			instances.append(instanceElement);
		} else if (instanceElement.isArray())
		{
			instances = instanceElement.toArray();
		}
	}
	else
	{
		// If no instance element is given use the one that was switched to before
		if (_currInstanceIndex != NO_INSTANCE_ID)
		{
			instances.append(_currInstanceIndex);
		}
		else
		{
			//If no instance was given nor one was switched to before, use first running instance (backward compatability)
			quint8 firstRunningInstanceID = NO_INSTANCE_ID;
			if (auto im = _instanceManagerWeak.toStrongRef())
			{
				firstRunningInstanceID = im->getFirstRunningInstanceIdx();
			}
			if (firstRunningInstanceID == NO_INSTANCE_ID)
			{
				QString errorText {"No instance(s) IDs provided and no running instance available applying the API request to"};
				Error(_log, "%s", QSTRING_CSTR(errorText));
				sendErrorReply(errorText, cmd);
				return;
			}

			instances.append(firstRunningInstanceID);
			Debug(_log,"No instance ID(s) provided; applying API request to first running instance [%u]", firstRunningInstanceID);

			if (!handleInstanceSwitch(firstRunningInstanceID))
			{
				QString errorText = QString("Error switching to first running instance: [%1] to process API request").arg(firstRunningInstanceID);
				Error(_log, "%s", QSTRING_CSTR(errorText));
				sendErrorReply(errorText, cmd);
				return;
			}
		}
	}

	InstanceCmd::MustRun const mustRun = cmd.getInstanceMustRun();
	QSet<quint8> configuredInstanceIds;
	QSet<quint8> runningInstanceIds;
	if (auto im = _instanceManagerWeak.toStrongRef())
	{
		configuredInstanceIds = im->getInstanceIds();
		runningInstanceIds = im->getRunningInstanceIdx();
	}
	QSet<quint8> instanceIds;
	QStringList errorDetails;
	QStringList errorDetailsInvalidInstances;

	// Determine instance IDs, if empty array provided apply command to all instances
	if (instances.isEmpty())
	{
		instanceIds = (mustRun == InstanceCmd::MustRun_Yes && !runningInstanceIds.isEmpty()) ? runningInstanceIds : configuredInstanceIds;
	}
	else
	{
		//Resolve instances provided and test, if they need to be running
		for (const auto &instance : std::as_const(instances))
		{
			auto const instanceId = static_cast<quint8>(instance.toInt());
			if (!configuredInstanceIds.contains(instanceId))
			{
				//Do not store in errorDetails, as command could be one working with and without instance
				errorDetailsInvalidInstances.append(QString("Not a valid instance id: [%1]").arg(instanceId));
				continue;
			}

			if (mustRun == InstanceCmd::MustRun_Yes && !runningInstanceIds.contains(instanceId))
			{
				errorDetails.append(QString("Instance [%1] is not running, but the (sub-) command requires a running instance.")
									.arg(instance.toVariant().toString()));
				continue;
			}

			instanceIds.insert(instanceId);
		}
	}

	// Handle cases where no instances are found
	if (instanceIds.isEmpty())
	{
		if (errorDetails.isEmpty() &&
			(cmd.getInstanceCmdType() == InstanceCmd::No_or_Single || cmd.getInstanceCmdType() == InstanceCmd::No_or_Multi) )
		{
			handleCommand(cmd, message);
			return;
		}

		errorDetails.append(errorDetailsInvalidInstances);
		errorDetails.append("No valid instance(s) provided");

		sendErrorReply("Errors for instance(s) given", errorDetails, cmd);
		return;
	}

	// Check if multiple instances are allowed
	if (instanceIds.size() > 1 && cmd.getInstanceCmdType() != InstanceCmd::Multi)
	{
		errorDetails.append("Command does not support multiple instances");
		sendErrorReply("Errors for instance(s) given", errorDetails, cmd);
		return;
	}

	// Execute the command for each valid instance
	quint8 const previousInstance = _currInstanceIndex;

	for (const auto &instanceId : std::as_const(instanceIds))
	{
		bool isCorrectInstance = true;

		if (mustRun == InstanceCmd::MustRun_Yes || _currInstanceIndex == NO_INSTANCE_ID)
		{
			isCorrectInstance = handleInstanceSwitch(instanceId);
		}

		if (isCorrectInstance)
		{
			handleCommand(cmd, message);
		}
	}

	//Switch back to current instance, if command was executed against multiple instances
	if (previousInstance != _currInstanceIndex &&
		(cmd.getInstanceCmdType() == InstanceCmd::Multi || cmd.getInstanceCmdType() == InstanceCmd::No_or_Multi))
	{
		handleInstanceSwitch(previousInstance);
	}
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
	case Command::Effect:
		handleEffectCommand(message, cmd);
	break;
	case Command::CreateEffect:
		handleCreateEffectCommand(message, cmd);
	break;
	case Command::DeleteEffect:
		handleDeleteEffectCommand(message, cmd);
	break;
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
	case Command::InstanceData:
		handleInstanceDataCommand(message, cmd);
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

void JsonAPI::handleGetImageSnapshotCommand(const QJsonObject& message, const JsonApiCommand &cmd)
{
	QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
	if (hyperion.isNull())
	{
		sendErrorReply("Failed to create image snapshot. No instance running.", cmd);
		return;
	}

	QString imageFormat = message.value("format").toString("jpg").toLower();
	if (imageFormat != "jpg" && imageFormat != "png")
	{
		imageFormat = "jpg";
	}

	Image<ColorRgb> capturedImage;
	QEventLoop loop;
	QTimer timer;
	timer.setSingleShot(true);

	const int IMAGE_TIMEOUT_MS = 500; // half a second timeout
	connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

	QObject tempContext; // context object for single-shot connection
	auto connection = QObject::connect(hyperion.get(), &Hyperion::currentImage, &tempContext,
		[&](const Image<ColorRgb>& img){
			capturedImage = img;
			loop.quit();
		});

	timer.start(IMAGE_TIMEOUT_MS);
	loop.exec();
	QObject::disconnect(connection);

	if (capturedImage.isNull())
	{
		sendErrorReply(QString("No image available within %1 ms").arg(IMAGE_TIMEOUT_MS), cmd);
		return;
	}

	QImage qimg = capturedImage.toQImage();
	// Optional: limit size similar to callbacks logic
	const int MAX_WIDTH = 1280;
	if (qimg.width() > MAX_WIDTH)
	{
		qimg = qimg.scaledToWidth(MAX_WIDTH, Qt::FastTransformation);
	}

	QByteArray byteArray;
	QBuffer buffer(&byteArray);
	buffer.open(QIODevice::WriteOnly);
	const char* fmt = (imageFormat == "png") ? "PNG" : "JPG";
	if (!qimg.save(&buffer, fmt))
	{
		sendErrorReply("Failed to encode image", cmd);
		return;
	}
	buffer.close();

	QJsonObject info;
	info["format"] = imageFormat;
	info["width"] = qimg.width();
	info["height"] = qimg.height();
	info["data"] = QStringLiteral("data:image/") + imageFormat + QStringLiteral(";base64,") + QString::fromLatin1(byteArray.toBase64());
	sendSuccessDataReply(info, cmd);
}

void JsonAPI::handleGetLedSnapshotCommand(const QJsonObject& /*message*/, const JsonApiCommand &cmd)
{
	QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
	if (hyperion.isNull())
	{
		sendErrorReply("Failed to create snapshot. No instance running.", cmd);
		return;
	}

	QVector<ColorRgb> ledColors;
	QEventLoop loop;
	QTimer timer;

	// Timeout handling (ensuring loop quits on timeout)
	timer.setSingleShot(true);
	connect(&timer, &QTimer::timeout, this, [&loop]() {
		loop.quit();  // Stop waiting if timeout occurs
	});

	// Capture LED colors when the LED data signal is emitted (execute only once)
	auto thisContext = std::make_unique<QObject>();
	auto pcontext = thisContext.get();
	auto connection = QObject::connect(hyperion.get(), &Hyperion::ledDeviceData, pcontext,
			[this, &loop, &ledColors, cmd](QVector<ColorRgb> ledColorsUpdate) {
		ledColors = ledColorsUpdate;
		loop.quit();  // Ensure the event loop quits immediately when data is received

		QJsonArray ledRgbColorsArray;
		for (const auto &color : ledColors)
		{
			QJsonArray rgbArray;
			rgbArray.append(color.red);
			rgbArray.append(color.green);
			rgbArray.append(color.blue);
			ledRgbColorsArray.append(rgbArray);
		}
		QJsonObject info;
		info["leds"] = ledRgbColorsArray;

		sendSuccessDataReply(info, cmd);
	}
	);

	// Start the timer and wait for either the signal or timeout
	timer.start(LED_DATA_TIMEOUT);
	loop.exec();
	QObject::disconnect(connection);

	// If no data was received, return an error
	if (ledColors.empty())
	{
		QString const replyMsg = QString("No LED color data available, i.e.no LED update was done within the last %1 ms").arg(LED_DATA_TIMEOUT.count());
		sendErrorReply(replyMsg, cmd);
		return;
	}
}

void JsonAPI::handleInstanceDataCommand(const QJsonObject &message, const JsonApiCommand &cmd)
{

	switch (cmd.subCommand)
	{
	case SubCommand::GetImageSnapshot:
		handleGetImageSnapshotCommand(message, cmd);
		break;
	case SubCommand::GetLedSnapshot:
		handleGetLedSnapshotCommand(message, cmd);
		break;
	default:
	break;
	}
}

void JsonAPI::handleColorCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	emit forwardJsonMessage(message, _currInstanceIndex);
	int const priority = message["priority"].toInt();
	int const duration = message["duration"].toInt(-1);
	const QString origin = message["origin"].toString("JsonRpc") + "@" + _peerAddress;

	const QJsonArray &jsonColor = message["color"].toArray();
	QVector<uint8_t> colors;
	colors.reserve(jsonColor.size());
	// Transform each entry in jsonColor to uint8_t and append to colors
	std::transform(jsonColor.begin(), jsonColor.end(), std::back_inserter(colors),
				   [](const QJsonValue &value) { return static_cast<uint8_t>(value.toInt()); });

	API::setColor(priority, colors, duration, origin);
	sendSuccessReply(cmd);
}

void JsonAPI::handleImageCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	emit forwardJsonMessage(message, _currInstanceIndex);

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

void JsonAPI::handleEffectCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
#if !defined(ENABLE_EFFECTENGINE)
	sendErrorReply("Effects are not supported by this installation!", cmd);
#else
	emit forwardJsonMessage(message, _currInstanceIndex);

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
#endif
}

void JsonAPI::handleCreateEffectCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
#if !defined(ENABLE_EFFECTENGINE)
	sendErrorReply("Effects are not supported by this installation!", cmd);
#else
	const QString resultMsg = EffectFileHandler::getInstance()->saveEffect(message);
	resultMsg.isEmpty() ? sendSuccessReply(cmd) : sendErrorReply(resultMsg, cmd);
#endif
}

void JsonAPI::handleDeleteEffectCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
#if !defined(ENABLE_EFFECTENGINE)
	sendErrorReply("Effects are not supported by this installation!", cmd);
#else
	const QString resultMsg = EffectFileHandler::getInstance()->deleteEffect(message["name"].toString());
	resultMsg.isEmpty() ? sendSuccessReply(cmd) : sendErrorReply(resultMsg, cmd);
#endif
}

void JsonAPI::handleSysInfoCommand(const QJsonObject & /*unused*/, const JsonApiCommand& cmd)
{
	sendSuccessDataReply(JsonInfo::getSystemInfo(), cmd);
}

void JsonAPI::handleServerInfoCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	QJsonObject info {};
	QStringList errorDetails;

	switch (cmd.getSubCommand()) {
	case SubCommand::Empty:
	case SubCommand::GetInfo:
	{
		// Global information
		QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
		info = JsonInfo::getInfo(hyperion, _log);
	
		if (!_noListener && message.contains("subscribe"))
		{
			const QJsonArray& subscriptions = message["subscribe"].toArray();
			QStringList const invaliCommands = _jsonCB->subscribe(subscriptions);
			if (!invaliCommands.isEmpty())
			{
				errorDetails.append("subscribe - Invalid commands provided: " + invaliCommands.join(','));
			}
		}
		// END
	}
	break;

	case SubCommand::Subscribe:
	case SubCommand::Unsubscribe:
	{
		QJsonValue const subscriptionsValue = message["subscribe"];
		if (subscriptionsValue.isUndefined())
		{
			sendErrorReply("Invalid params", {"No subscriptions provided"}, cmd);
		}

		const QJsonArray &subscriptions = subscriptionsValue.toArray();
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
	emit forwardJsonMessage(message, _currInstanceIndex);
	int const priority = message["priority"].toInt();
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
	emit forwardJsonMessage(message, _currInstanceIndex);
	QString replyMsg;
	API::clearPriority(-1, replyMsg);
	sendSuccessReply(cmd);
}

void JsonAPI::handleAdjustmentCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
	if (hyperion.isNull())
	{
		sendErrorReply("Failed to handle adjustment command. No instance running.", cmd);
		return;
	}

	const QJsonObject& adjustment = message["adjustment"].toObject();

	const QList<QString> adjustmentIds = hyperion->getAdjustmentIds();
	if (adjustmentIds.isEmpty()) {
		sendErrorReply("No adjustment data available", cmd);
		return;
	}

	const QString adjustmentId = adjustment["id"].toString(adjustmentIds.first());
	ColorAdjustment* colorAdjustment = hyperion->getAdjustment(adjustmentId);
	if (colorAdjustment == nullptr) {
		Warning(_log, "Incorrect adjustment identifier: %s", adjustmentId.toStdString().c_str());
		return;
	}

	applyColorAdjustments(adjustment, colorAdjustment);
	applyTransforms(adjustment, colorAdjustment);
	hyperion->adjustmentsUpdated();

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
	applyTransform("temperature", adjustment, colorAdjustment->_rgbTransform, &RgbTransform::setTemperature);
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
void JsonAPI::applyTransform(const QString &transformName, const QJsonObject &adjustment, T &transform, void (T::*setFunction)(int))
{
	if (adjustment.contains(transformName)) {
		(transform.*setFunction)(adjustment[transformName].toInt());
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
		handleConfigGetCommand(message, cmd);
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
	if (DBManager::isReadOnly())
	{
		sendErrorReply("Database Error", {"Hyperion is running in read-only mode","Configuration updates are not possible"}, cmd);
		return;
	}

	QJsonObject config = message["config"].toObject();
	if (config.isEmpty())
	{
		sendErrorReply("Update configuration failed", {"No configuration data provided!"}, cmd);
		return;
	}

	QStringList errorDetails;

	QMap<quint8, QJsonObject> instancesNewConfigs;

	const QJsonArray instances = config["instances"].toArray();
	if (!instances.isEmpty())
	{
		QSet<quint8> configuredInstanceIds;
		if (auto im = _instanceManagerWeak.toStrongRef())
		{
			configuredInstanceIds = im->getInstanceIds();
		}
		for (const auto &instance : instances)
		{
			QJsonObject instanceObject = instance.toObject();
			const QJsonValue idx = instanceObject["id"];
			if (idx.isDouble())
			{
				auto const instanceId = static_cast<quint8>(idx.toInt());
				if (configuredInstanceIds.contains(instanceId))
				{
					instancesNewConfigs.insert(instanceId,instanceObject.value("settings").toObject());
				}
				else
				{
					errorDetails.append(QString("Given instance id '%1' does not exist. Configuration item will be ignored").arg(instanceId));
				}

			}
		}
	}

	const QJsonObject globalSettings = config["global"].toObject().value("settings").toObject();
	if (!globalSettings.isEmpty())
	{
		const QJsonObject instanceZeroConfig = instancesNewConfigs.value(NO_INSTANCE_ID);
		instancesNewConfigs.insert(NO_INSTANCE_ID, JsonUtils::mergeJsonObjects(instanceZeroConfig, globalSettings));
	}

	QMapIterator<quint8, QJsonObject> iter (instancesNewConfigs);
	while (iter.hasNext()) {
		iter.next();

		quint8 const idx = iter.key();

		QPair<bool, QStringList> isSaved;
		if (auto mgrStrong = HyperionIManager::getInstanceWeak().toStrongRef())
		{
			if (mgrStrong->isInstanceRunning(idx))
			{
				QSharedPointer<Hyperion> const instance = mgrStrong->getHyperionInstance(idx);
				isSaved = instance->saveSettings(iter.value());
			}
			else
			{
				SettingsManager settingMgr (idx);
				connect(&settingMgr, &SettingsManager::settingsChanged, mgrStrong.get(), &HyperionIManager::settingsChanged);
				isSaved = settingMgr.saveSettings(iter.value());
			}
		}
		else
		{
			// Manager not available; cannot save settings
			isSaved.first = false;
			isSaved.second.append(QString("Instance manager unavailable for instance %1").arg(idx));
		}

		errorDetails.append(isSaved.second);
	}

	if (!errorDetails.isEmpty())
	{
		sendErrorReply("Update configuration failed", errorDetails, cmd);
		return;
	}

	sendSuccessReply(cmd);
}

void JsonAPI::handleConfigGetCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	QJsonObject settings;

	QJsonObject filter = message["configFilter"].toObject();
	if (filter.isEmpty())
	{
		//Get complete configuration
		settings = JsonInfo::getConfiguration();
		sendSuccessDataReply(settings, cmd);
	}

	QStringList errorDetails;
	QStringList globalFilterTypes;

	const QJsonValue globalConfig = filter["global"];
	if (globalConfig.isNull())
	{
		globalFilterTypes.append("__none__");
	}
	else
	{
		const QJsonObject globalConfigObject = globalConfig.toObject();
		if (!globalConfigObject.isEmpty())
		{
			QJsonValue const globalTypes = globalConfig["types"];
			if (globalTypes.isNull())
			{
				globalFilterTypes.append("__none__");
			}
			else
			{
				QJsonArray const globalTypesList = globalTypes.toArray();
				for (const auto &type : globalTypesList) {
					if (type.isString()) {
						globalFilterTypes.append(type.toString());
					}
				}
			}
		}
	}

	QList<quint8> instanceListFilter;
	QStringList instanceFilterTypes;

	const QJsonValue instances = filter["instances"];
	if (instances.isNull())
	{
		instanceListFilter.append(NO_INSTANCE_ID);
	}
	else
	{
		const QJsonObject instancesObject = instances.toObject();
		if (!instancesObject.isEmpty())
		{
			QSet<quint8> configuredInstanceIds;
			if (auto im = _instanceManagerWeak.toStrongRef())
			{
				configuredInstanceIds = im->getInstanceIds();
			}
			QJsonValue const instanceIds = instances["ids"];
			if (instanceIds.isNull())
			{
				instanceListFilter.append(NO_INSTANCE_ID);
			}
			else
			{
				QJsonArray const instaceIdsList = instanceIds.toArray();
				for (const auto &idx : instaceIdsList) {
					if (idx.isDouble()) {
						auto const instanceId = static_cast<quint8>(idx.toInt());
						if (configuredInstanceIds.contains(instanceId))
						{
							instanceListFilter.append(instanceId);
						}
						else
						{
							errorDetails.append(QString("Given instance number '%1' does not exist.").arg(instanceId));
						}
					}
				}

				QJsonValue const instanceTypes = instances["types"];
				if (instanceTypes.isNull())
				{
					instanceFilterTypes.append("__none__");
				}
				else
				{
					QJsonArray const instaceTypesList = instanceTypes.toArray();
					for (const auto &type : instaceTypesList) {
						if (type.isString()) {
							instanceFilterTypes.append(type.toString());
						}
					}
				}
			}
		}
	}

	settings = JsonInfo::getConfiguration(instanceListFilter, instanceFilterTypes, globalFilterTypes);

	sendSuccessDataReplyWithError(settings, cmd, errorDetails);
}

void JsonAPI::handleConfigRestoreCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	QJsonObject config = message["config"].toObject();

	DBConfigManager configManager;
	auto const [success, messages] = configManager.updateConfiguration(config, true);
	if (success)
	{
		QString const infoMsg {"Restarting after importing configuration successfully."};
		sendSuccessDataReply(infoMsg, cmd);
		Info(_log, "%s", QSTRING_CSTR(infoMsg));
		emit signalEvent(Event::Restart);
	}
	else
	{
		sendErrorReply("Restore configuration failed", messages, cmd);
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
	QString const schemaFile = SETTINGS_UI_SCHEMA_FILE;

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

	// Add infor about the type of setting elements
	QJsonObject settingTypes;
	QJsonArray globalSettingTypes;

	SettingsTable const settingsTable;
	for (const QString &type : settingsTable.getGlobalSettingTypes())
	{
		globalSettingTypes.append(type);
	}
	settingTypes.insert("globalProperties", globalSettingTypes);

	QJsonArray instanceSettingTypes;
	for (const QString &type : settingsTable.getInstanceSettingTypes())
	{
		instanceSettingTypes.append(type);
	}
	settingTypes.insert("instanceProperties", instanceSettingTypes);
	properties.insert("propertiesTypes", settingTypes);

	properties.insert("effectSchemas", JsonInfo::getEffectSchemas());

	schemaJson.insert("properties", properties);

	// send the result
	sendSuccessDataReply(schemaJson, cmd);
}

void JsonAPI::handleComponentStateCommand(const QJsonObject &message, const JsonApiCommand& cmd)
{
	const QJsonObject &componentState = message["componentstate"].toObject();
	QString const comp = componentState["component"].toString("invalid");
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
	{
		_jsonCB->subscribe(Subscription::LedColorsUpdate);
		if (auto hyperion = _hyperionWeak.toStrongRef())
		{
			// push once
			hyperion->update();
		}
		sendSuccessReply(cmd);
	}
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
	bool isTokenRequired = true;
	if (islocalConnection())
	{
		// For local connections, token required only if local auth enforced
		if (auto auth = _authManagerWeak.toStrongRef())
		{
			isTokenRequired = auth->isLocalAuthRequired();
		}
	}
	else
	{
		// Non-local connections always require token
		isTokenRequired = true;
	}
	QJsonObject const response { { "required", isTokenRequired} };
	sendSuccessDataReply(response, cmd);
}

void JsonAPI::handleAdminRequired(const JsonApiCommand& cmd)
{
	bool isAdminAuthRequired = true;
	QJsonObject const response { { "adminRequired", isAdminAuthRequired} };
	sendSuccessDataReply(response, cmd);
}

void JsonAPI::handleNewPasswordRequired(const JsonApiCommand& cmd)
{
	QJsonObject const response { { "newPasswordRequired", API::hasHyperionDefaultPw() } };
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
			QJsonObject const response { { "token", userTokenRep } };
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
	QStringList errorDetails;

	QJsonValue const instanceValue = message["instance"];

	auto const instanceID = static_cast<quint8>(instanceValue.toInt());
	if(cmd.subCommand != SubCommand::CreateInstance)
	{
		QString errorText;
		if (instanceValue.isUndefined())
		{
			errorText = "No instance provided, but required";

		} 
		else 
		{
			bool exists = false;
			if (auto im = _instanceManagerWeak.toStrongRef())
			{
				exists = im->doesInstanceExist(instanceID);
			}
			if (!exists)
			{
				errorText = QString("Hyperion instance [%1] does not exist.").arg(instanceID);
			}
		}

		if (!errorText.isEmpty())
		{
			sendErrorReply( errorText, cmd);
			return;
		}
	}

	QString instanceName;
	if (auto im = _instanceManagerWeak.toStrongRef())
	{
		instanceName = im->getInstanceName(instanceID);
	}
	const QString &name = message["name"].toString();

	switch (cmd.subCommand) {
	case SubCommand::SwitchTo:
		if (handleInstanceSwitch(instanceID))
		{
			QJsonObject const response { { "instance", instanceID } };
			sendSuccessDataReply(response, cmd);
		}
		else
		{
			sendErrorReply(QString("Switching to Hyperion instance [%1] - %2 failed.").arg(instanceID).arg(instanceName), cmd);
		}
	break;

	case SubCommand::StartInstance:
	{
		bool isRunning = false;
		if (auto im = _instanceManagerWeak.toStrongRef())
		{
			isRunning = im->isInstanceRunning(instanceID);
		}
		if (isRunning)
		{
			errorDetails.append(QString("Hyperion instance [%1] - '%2' is already running.").arg(instanceID).arg(instanceName));
			sendSuccessDataReplyWithError({}, cmd, errorDetails);
			return;
		}
		//Only send update once
		weakConnect(this, &API::onStartInstanceResponse, [this, cmd] ()
		{
			sendSuccessReply(cmd);
		});

		if (!API::startInstance(instanceID, cmd.tan))
		{
			sendErrorReply(QString("Starting Hyperion instance [%1] - '%2' failed.").arg(instanceID).arg(instanceName), cmd);
		}
	}
	break;
	case SubCommand::StopInstance:
	{
		bool isRunning = false;
		if (auto im = _instanceManagerWeak.toStrongRef())
		{
			isRunning = im->isInstanceRunning(instanceID);
		}

		if (!isRunning)
		{
			errorDetails.append(QString("Hyperion instance [%1] - '%2' is not running.").arg(instanceID).arg(instanceName));
			sendSuccessDataReplyWithError({},cmd, errorDetails);
			return;
		}
		// silent fail
		API::stopInstance(instanceID);
		sendSuccessReply(cmd);
	}
	break;

	case SubCommand::DeleteInstance:
		if (API::deleteInstance(instanceID, replyMsg))
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
			replyMsg = API::setInstanceName(instanceID, name);
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

	QJsonObject const config { { "type", devType } };
	QScopedPointer<LedDevice> const ledDevice(LedDeviceFactory::construct(config));

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
}

void JsonAPI::handleLedDeviceDiscover(LedDevice& ledDevice, const QJsonObject& message, const JsonApiCommand& cmd)
{
	const QJsonObject &params = message["params"].toObject();
	qCDebug(api_leddevice).noquote() << "Discover devices: " << JsonUtils::toCompact(params);

	const QJsonObject devicesDiscovered = ledDevice.discover(params);
	qCDebug(api_leddevice).noquote() << "Discovered devices: " << JsonUtils::toCompact(devicesDiscovered);
	sendSuccessDataReply(devicesDiscovered, cmd);
}

void JsonAPI::handleLedDeviceGetProperties(LedDevice& ledDevice, const QJsonObject& message, const JsonApiCommand& cmd)
{
	const QJsonObject &params = message["params"].toObject();
	qCDebug(api_leddevice).noquote() << "Get device properties:" << JsonUtils::toCompact(params);
	const QJsonObject deviceProperties = ledDevice.getProperties(params);
	qCDebug(api_leddevice).noquote() << "Device properties:" << JsonUtils::toCompact(deviceProperties);
	sendSuccessDataReply(deviceProperties, cmd);
}

void JsonAPI::handleLedDeviceIdentify(LedDevice& ledDevice, const QJsonObject& message, const JsonApiCommand& cmd)
{
	const QJsonObject &params = message["params"].toObject();
	qCDebug(api_leddevice).noquote() << "Identify device:" << JsonUtils::toCompact(params);
	ledDevice.identify(params);
	qCDebug(api_leddevice).noquote() << "Identified device:" << JsonUtils::toCompact(params);
	sendSuccessReply(cmd);
}

void JsonAPI::handleLedDeviceAddAuthorization(LedDevice& ledDevice, const QJsonObject& message, const JsonApiCommand& cmd)
{
	const QJsonObject& params = message["params"].toObject();
	qCDebug(api_leddevice) << "Add authorization to device:" << JsonUtils::toCompact(params);
	const QJsonObject response = ledDevice.addAuthorization(params);
	qCDebug(api_leddevice) << "Add authorization response:" << JsonUtils::toCompact(response);
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
		qCDebug(api_inputsource).noquote() << "Discover input sources:" << JsonUtils::toCompact(params);
		QJsonObject const inputSourcesDiscovered = JsonInfo().discoverSources(sourceType, params);
		qCDebug(api_inputsource).noquote() << "Discovered input sources:" << JsonUtils::toCompact(inputSourcesDiscovered);
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
		QString const discoveryMethod("mDNS");
		serviceType = MdnsServiceRegister::getServiceType(type);
#else
		QString discoveryMethod("ssdp");
#endif
		if (!serviceType.isEmpty())
		{
			QJsonArray serviceList;
#ifdef ENABLE_MDNS
			QMetaObject::invokeMethod(MdnsBrowser::getInstance().get(), "browseForServiceType",
									  Qt::QueuedConnection, Q_ARG(QByteArray, serviceType));

			serviceList = MdnsBrowser::getInstance()->getServicesDiscoveredJson(serviceType, MdnsServiceRegister::getServiceNameFilter(type), DEFAULT_DISCOVER_TIMEOUT);
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

QJsonObject JsonAPI::getBasicCommandReply(bool success, const QString &command, int tan, InstanceCmd::Type /*instanceCmdType*/) const
{
	QJsonObject reply;
	reply["success"] = success;
	reply["command"] = command;
	reply["tan"] = tan;

	return reply;
}

void JsonAPI::sendSuccessReply(const JsonApiCommand& cmd)
{
	sendSuccessReply(cmd.toString(), cmd.tan, cmd.instanceCmdType);
}

void JsonAPI::sendSuccessReply(const QString &command, int tan, InstanceCmd::Type instanceCmdType)
{
	QJsonObject const reply {getBasicCommandReply(true, command, tan , instanceCmdType)};

	qCDebug(api_msg_reply_success).noquote() << "Sending reply:" << JsonUtils::toCompact(reply);
	emit callbackReady(reply);
}

void JsonAPI::sendSuccessDataReply(const QJsonValue &infoData, const JsonApiCommand& cmd)
{
	sendSuccessDataReplyWithError(infoData, cmd.toString(), cmd.tan, {}, cmd.instanceCmdType);
}

void JsonAPI::sendSuccessDataReply(const QJsonValue &infoData, const QString &command, int tan, InstanceCmd::Type instanceCmdType)
{
	sendSuccessDataReplyWithError(infoData, command, tan, {}, instanceCmdType);
}

void JsonAPI::sendSuccessDataReplyWithError(const QJsonValue &infoData, const JsonApiCommand& cmd, const QStringList& errorDetails)
{
	sendSuccessDataReplyWithError(infoData, cmd.toString(), cmd.tan, errorDetails, cmd.instanceCmdType);
}

void JsonAPI::sendSuccessDataReplyWithError(const QJsonValue &infoData, const QString &command, int tan, const QStringList& errorDetails, InstanceCmd::Type instanceCmdType)
{
	QJsonObject reply {getBasicCommandReply(true, command, tan , instanceCmdType)};
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
	
	qCDebug(api_msg_reply_success).noquote() << "Sending reply:" << JsonUtils::toCompact(reply);
	emit callbackReady(reply);
}

void JsonAPI::sendErrorReply(const QString &error, const JsonApiCommand& cmd)
{
	sendErrorReply(error, {}, cmd.toString(), cmd.tan, cmd.instanceCmdType);
}

void JsonAPI::sendErrorReply(const QString &error, const QStringList& errorDetails, const JsonApiCommand& cmd)
{
	sendErrorReply(error, errorDetails, cmd.toString(), cmd.tan, cmd.instanceCmdType);
}

void JsonAPI::sendErrorReply(const QString &error, const QStringList& errorDetails, const QString &command, int tan, InstanceCmd::Type instanceCmdType)
{
	QJsonObject reply {getBasicCommandReply(false, command, tan , instanceCmdType)};
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
	
	qCDebug(api_msg_reply_error).noquote() << "Sending reply:" << JsonUtils::toCompact(reply);
	emit callbackReady(reply);
}

void JsonAPI::sendNewRequest(const QJsonValue &infoData, const JsonApiCommand& cmd)
{
	sendSuccessDataReplyWithError(infoData, cmd.toString(), cmd.instanceCmdType);
}

void JsonAPI::sendNewRequest(const QJsonValue &infoData, const QString &command, InstanceCmd::Type instanceCmdType)
{
	QJsonObject request;
	request["command"] = command;

	if (instanceCmdType != InstanceCmd::No)
	{
		if (auto hyperion = _hyperionWeak.toStrongRef())
		{
			request["instance"] = hyperion->getInstanceIndex();
		}
	}

	request["info"] = infoData;

	qCDebug(api_msg_new).noquote() << "Sending new request:" << JsonUtils::toCompact(request);
	emit callbackReady(request);
}

void JsonAPI::sendNoAuthorization(const JsonApiCommand& cmd)
{
	sendErrorReply(NO_AUTHORIZATION, cmd);
}

void JsonAPI::handleInstanceStateChange(InstanceState state, quint8 instanceID, const QString& /*name */)
{
	switch (state)
	{
	case InstanceState::H_STARTED:
	{
		quint8 const currentInstance = _currInstanceIndex;
		if (instanceID == currentInstance)
		{
			if (auto im = _instanceManagerWeak.toStrongRef())
			{
				_hyperionWeak = im->getHyperionInstance(instanceID);
			}
			_jsonCB->setSubscriptionsTo(instanceID);
		}
	}
	break;

	case InstanceState::H_STOPPED:
	case InstanceState::H_STARTING:
	case InstanceState::H_ON_STOP:
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
	static QRegularExpression regex(R"("command"\s*:\s*"([^"]+)");
	QRegularExpressionMatch match = regex.match(jsonString);

	if (match.hasMatch()) {
		commandValue = match.captured(1);
	}
	return commandValue;
}

int JsonAPI::findTan (const QString& jsonString)
{
	int tanValue {0};
	static QRegularExpression regex(R"("tan"\s*:\s*(\d+))");
	QRegularExpressionMatch match = regex.match(jsonString);

	if (match.hasMatch()) {
		QString valueStr = match.captured(1);
		tanValue = valueStr.toInt();
	}
	return tanValue;
}
