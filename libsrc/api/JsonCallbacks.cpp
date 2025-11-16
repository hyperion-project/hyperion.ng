#include <api/JsonCallbacks.h>
#include <api/JsonInfo.h>
#include <api/JsonApiSubscription.h>

#include <hyperion/Hyperion.h>
#include <hyperion/HyperionIManager.h>
#include <events/EventHandler.h>
#include <hyperion/ComponentRegister.h>
#include <hyperion/PriorityMuxer.h>
#include <utils/ColorSys.h>
#include <hyperion/ImageProcessor.h>
#include <effectengine/EffectFileHandler.h>
#include <db/SettingsTable.h>

#include <QDateTime>
#include <QVariant>
#include <QImage>
#include <QBuffer>
 
Q_LOGGING_CATEGORY(api_callback_msg, "api.callback.msg");
Q_LOGGING_CATEGORY(api_callback_image, "api.callback.image");
Q_LOGGING_CATEGORY(api_callback_leds, "api.callback.leds");

// Constants
namespace {
	bool const IS_IMAGE_SIZE_LIMITED{ true };
	int const MAX_CALLBACK_IMAGE_WIDTH{ 1280 };
	constexpr std::chrono::milliseconds MAX_IMAGE_EMISSION_INTERVAL{ 40 }; // 25 Hz
	constexpr std::chrono::milliseconds MAX_LED_DEVICE_DATA_EMISSION_INTERVAL{ 10 }; // 100 Hz
} //End of constants

JsonCallbacks::JsonCallbacks(QSharedPointer<Logger> log, const QString& peerAddress, QObject* parent)
	: QObject(parent)
	, _log (log)
	, _hyperionWeak(nullptr)
	, _instanceManagerWeak(HyperionIManager::getInstanceWeak())
	, _peerAddress (peerAddress)
	, _componentRegisterWeak(nullptr)
	, _prioMuxerWeak(nullptr)
	, _islogMsgStreamingActive(false)
	, _lastLedUpdateTime(0)
	, _lastImageUpdateTime(0)
	, _isImageSizeLimited(IS_IMAGE_SIZE_LIMITED)
{
	TRACK_SCOPE();
	qRegisterMetaType<PriorityMuxer::InputsMap>("InputsMap");

	if (auto mgr = _instanceManagerWeak.toStrongRef())
	{
		connect(mgr.get(), &HyperionIManager::instanceStateChanged, this, &JsonCallbacks::handleInstanceStateChange);
	}
}

JsonCallbacks::~JsonCallbacks()
{
	TRACK_SCOPE();
}

void JsonCallbacks::handleInstanceStateChange(InstanceState state, quint8 instanceID, const QString& /*name */)
{
	switch (state)
	{
	case InstanceState::H_STOPPED:
	{
		if (instanceID == _instanceID)
		{
			setSubscriptionsTo(NO_INSTANCE_ID);
		}
	}
	[[fallthrough]];
	case InstanceState::H_ON_STOP:
	case InstanceState::H_STARTING:
	case InstanceState::H_STARTED:
	case InstanceState::H_CREATED:
	case InstanceState::H_DELETED:
		break;
	}
}

bool JsonCallbacks::subscribe(const Subscription::Type cmd)
{
	if (_subscribedCommands.contains(cmd))
	{
		return true;
	}

	QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();

	switch (cmd) {

	// Global subscriptions
	case Subscription::EffectsUpdate:
#if defined(ENABLE_EFFECTENGINE)
		if (auto fh = EffectFileHandler::getInstance())
		{
			connect(fh.data(), &EffectFileHandler::effectListChanged, this, &JsonCallbacks::handleEffectListChange);
		}
#endif
	break;

	case Subscription::EventUpdate:
		connect(EventHandler::getInstance().data(), &EventHandler::signalEvent, this, &JsonCallbacks::handleEventUpdate);
	break;
	case Subscription::InstanceUpdate:
		if (auto mgr = _instanceManagerWeak.toStrongRef())
		{
			connect(mgr.get(), &HyperionIManager::change, this, &JsonCallbacks::handleInstanceChange);
		}
	break;
	case Subscription::LogMsgUpdate:
		if (!_islogMsgStreamingActive)
		{
			handleLogMessageUpdate (Logger::T_LOG_MESSAGE{}); // needed to trigger log sending
			_islogMsgStreamingActive = true;
			Debug(_log, "log streaming activated for client %s", _peerAddress.toStdString().c_str());
		}
		connect(LoggerManager::getInstance().data(), &LoggerManager::newLogMessage, this, &JsonCallbacks::handleLogMessageUpdate);
	break;
	case Subscription::SettingsUpdate:
		if (auto mgr = _instanceManagerWeak.toStrongRef())
		{
			connect(mgr.get(), &HyperionIManager::settingsChanged, this, &JsonCallbacks::handleSettingsChange);
		}
	break;
	case Subscription::TokenUpdate:
		if (auto auth = AuthManager::getInstance())
		{
			connect(auth.data(), &AuthManager::tokenChange, this, &JsonCallbacks::handleTokenChange, Qt::AutoConnection);
		}
	break;

		// Instance specific subscriptions
	case Subscription::AdjustmentUpdate:
		if (!hyperion.isNull()) {
			connect(hyperion.get(), &Hyperion::adjustmentChanged, this, &JsonCallbacks::handleAdjustmentChange);
		}
	break;
	case Subscription::ComponentsUpdate:
	{
		QSharedPointer<ComponentRegister> const componentRegisterStrong = _componentRegisterWeak.toStrongRef();
		if (!componentRegisterStrong.isNull()) {
			connect(componentRegisterStrong.get(), &ComponentRegister::updatedComponentState, this, &JsonCallbacks::handleComponentState);
		}
	}
	break;
	case Subscription::ImageToLedMappingUpdate:
		if (!hyperion.isNull()) {
			connect(hyperion.get(), &Hyperion::imageToLedsMappingChanged, this, &JsonCallbacks::handleImageToLedsMappingChange);
		}
	break;
	case Subscription::ImageUpdate:
		if (!hyperion.isNull()) {
			_imageUpdateTimer.start();
			connect(hyperion.get(),  &Hyperion::currentImage, this, &JsonCallbacks::handleImageUpdate);
		}
	break;
	case Subscription::LedColorsUpdate:
		if (!hyperion.isNull()) {
			_ledUpdateTimer.start();
			connect(hyperion.get(), &Hyperion::rawLedColors, this, &JsonCallbacks::handleLedColorUpdate);
		}
	break;
	case Subscription::LedsUpdate:
		if (!hyperion.isNull()) {
			connect(hyperion.get(), &Hyperion::settingsChanged, this, &JsonCallbacks::handleLedsConfigChange);
		}
	break;
	case Subscription::PrioritiesUpdate:
	{
		QSharedPointer<PriorityMuxer> const prioMuxerStrong = _prioMuxerWeak.toStrongRef();
		if (!prioMuxerStrong.isNull()) {
			connect(prioMuxerStrong.get(), &PriorityMuxer::prioritiesChanged, this, &JsonCallbacks::handlePriorityUpdate);
		}
	}
	break;
	case Subscription::VideomodeUpdate:
		if (!hyperion.isNull()) {
			connect(hyperion.get(), &Hyperion::newVideoMode, this, &JsonCallbacks::handleVideoModeChange);
		}
	break;

	default:
	return false;
	}

	_subscribedCommands.insert(cmd);

	return true;
}

bool JsonCallbacks::subscribe(const QString& cmd)
{
	JsonApiSubscription const subscription = ApiSubscriptionRegister::getSubscriptionInfo(cmd);
	if (subscription.cmd == Subscription::Unknown)
	{
		return false;
	}
	return subscribe(subscription.cmd);
}

QStringList JsonCallbacks::subscribe(const QJsonArray& subscriptions)
{
	QJsonArray subsArr;
	if (subscriptions.isEmpty())
	{
		const QStringList commands = getCommands(false);
		for (const QString& entry : commands)
		{
			subsArr.append(entry);
		}
	}
	else
	{
		subsArr = subscriptions;
	}

	QStringList invalidSubscriptions;
	for (const QJsonValue& entry : subsArr)
	{
		if (!subscribe(entry.toString()))
		{
			invalidSubscriptions.append(entry.toString());
		}
	}
	return invalidSubscriptions;
}

bool JsonCallbacks::unsubscribe(const Subscription::Type cmd)
{
	if (!_subscribedCommands.contains(cmd))
	{
		return true;
	}

	_subscribedCommands.remove(cmd);

	QSharedPointer<Hyperion> const hyperion = _hyperionWeak.toStrongRef();

	switch (cmd) {
	// Global subscriptions
	case Subscription::EffectsUpdate:
#if defined(ENABLE_EFFECTENGINE)
		if (auto fh = EffectFileHandler::getInstance())
		{
			disconnect(fh.data(), &EffectFileHandler::effectListChanged, this, &JsonCallbacks::handleEffectListChange);
		}
#endif
	break;

	case Subscription::EventUpdate:
		disconnect(EventHandler::getInstance().data(), &EventHandler::signalEvent, this, &JsonCallbacks::handleEventUpdate);
	break;
	case Subscription::InstanceUpdate:
		if (auto mgr = _instanceManagerWeak.toStrongRef())
		{
			disconnect(mgr.get(), &HyperionIManager::change, this, &JsonCallbacks::handleInstanceChange);
		}
	break;
	case Subscription::LogMsgUpdate:
		disconnect(LoggerManager::getInstance().data(), &LoggerManager::newLogMessage, this, &JsonCallbacks::handleLogMessageUpdate);
		if (_islogMsgStreamingActive)
		{
			_islogMsgStreamingActive = false;
			Debug(_log, "log streaming deactivated for client  %s", _peerAddress.toStdString().c_str());
		}
	break;
	case Subscription::SettingsUpdate:
		if (auto mgr = _instanceManagerWeak.toStrongRef())
		{
			disconnect(mgr.get(), &HyperionIManager::settingsChanged, this, &JsonCallbacks::handleSettingsChange);
		}
	break;
	case Subscription::TokenUpdate:
		if (auto auth = AuthManager::getInstance())
		{
			disconnect(auth.data(), &AuthManager::tokenChange, this, &JsonCallbacks::handleTokenChange);
		}
	break;

		// Instance specific subscriptions
	case Subscription::AdjustmentUpdate:
		if (!hyperion.isNull()) {
			disconnect(hyperion.get(), &Hyperion::adjustmentChanged, this, &JsonCallbacks::handleAdjustmentChange);
		}
	break;
	case Subscription::ComponentsUpdate:
	{
		QSharedPointer<ComponentRegister> const componentRegisterStrong = _componentRegisterWeak.toStrongRef();
		if (!componentRegisterStrong.isNull()) {
			disconnect(componentRegisterStrong.get(), &ComponentRegister::updatedComponentState, this, &JsonCallbacks::handleComponentState);
		}
	}
	break;

	case Subscription::ImageToLedMappingUpdate:
		if (!hyperion.isNull()) {
			disconnect(hyperion.get(), &Hyperion::imageToLedsMappingChanged, this, &JsonCallbacks::handleImageToLedsMappingChange);
		}
	break;
	case Subscription::ImageUpdate:
		_imageUpdateTimer.invalidate();
		_lastImageUpdateTime = 0;
		if (!hyperion.isNull()) {
			disconnect(hyperion.get(), &Hyperion::currentImage, this, &JsonCallbacks::handleImageUpdate);
		}
	break;
	case Subscription::LedColorsUpdate:
		if (!hyperion.isNull()) {
			_ledUpdateTimer.invalidate();
			_lastLedUpdateTime = 0;
			disconnect(hyperion.get(), &Hyperion::rawLedColors, this, &JsonCallbacks::handleLedColorUpdate);
		}
	break;
	case Subscription::LedsUpdate:
		if (!hyperion.isNull()) {
			disconnect(hyperion.get(), &Hyperion::settingsChanged, this, &JsonCallbacks::handleLedsConfigChange);
		}
	break;
	case Subscription::PrioritiesUpdate:
	{
		QSharedPointer<PriorityMuxer> const prioMuxerStrong = _prioMuxerWeak.toStrongRef();
		if (!prioMuxerStrong.isNull()) {
			disconnect(prioMuxerStrong.get(), &PriorityMuxer::prioritiesChanged, this, &JsonCallbacks::handlePriorityUpdate);
		}
	}
	break;
	case Subscription::VideomodeUpdate:
		if (!hyperion.isNull()) {
			disconnect(hyperion.get(), &Hyperion::newVideoMode, this, &JsonCallbacks::handleVideoModeChange);
		}
	break;

	default:
	return false;
	}
	return true;
}

bool JsonCallbacks::unsubscribe(const QString& cmd)
{
	JsonApiSubscription const subscription = ApiSubscriptionRegister::getSubscriptionInfo(cmd);
	if (subscription.cmd == Subscription::Unknown)
	{
		return false;
	}
	return unsubscribe(subscription.cmd);
}

QStringList JsonCallbacks::unsubscribe(const QJsonArray& subscriptions)
{
	QJsonArray subsArr;
	if (subscriptions.isEmpty())
	{
		resetSubscriptions();
		return {};
	}

	subsArr = subscriptions;

	QStringList invalidSubscriptions;
	for (const QJsonValue& entry : subsArr)
	{
		if (!unsubscribe(entry.toString()))
		{
			invalidSubscriptions.append(entry.toString());
		}
	}
	return invalidSubscriptions;
}

void JsonCallbacks::resetSubscriptions()
{
	const QSet<Subscription::Type> currentSubscriptions = _subscribedCommands;
	for (QSet<Subscription::Type>::const_iterator it = currentSubscriptions.constBegin(); it != currentSubscriptions.constEnd(); ++it)
	{
		unsubscribe(*it);
	}
}

void JsonCallbacks::setSubscriptionsTo(quint8 instanceID)
{
	// get current subscriptions
	const QSet<Subscription::Type> currSubs(_subscribedCommands);

	// stop subs
	resetSubscriptions();

	_instanceID = instanceID;

	// update pointer
	QSharedPointer<Hyperion> hyperion;
	if (auto mgr = _instanceManagerWeak.toStrongRef())
	{
		hyperion = mgr->getHyperionInstance(instanceID);
	}
	if (!hyperion.isNull() && hyperion != _hyperionWeak)
	{
		_hyperionWeak = hyperion;
		_componentRegisterWeak = hyperion->getComponentRegister();
		_prioMuxerWeak = hyperion->getMuxerInstance();
	}

	// re-apply subscriptions
	for(const auto & entry : currSubs)
	{
		subscribe(entry);
	}
}

QStringList JsonCallbacks::getCommands(bool fullList) const
{
	QStringList commands;
	for (JsonApiSubscription const subscription : ApiSubscriptionRegister::getSubscriptionLookup())
	{
		if (fullList || subscription.isAll)
		{
			commands << Subscription::toString(subscription.cmd);
		}
	}
	return commands;
}

QStringList JsonCallbacks::getSubscribedCommands() const
{
	QStringList commands;
	for (Subscription::Type const cmd : _subscribedCommands)
	{
		commands << Subscription::toString(cmd);
	}
	return commands;
}

void JsonCallbacks::doCallback(Subscription::Type cmd, const QVariant& data)
{
	if (data.userType() == QMetaType::QJsonArray)
	{
		doCallback(cmd, data.toJsonArray());
	}
	else
	{
		doCallback(cmd, data.toJsonObject());
	}
}

void JsonCallbacks::doCallback(Subscription::Type cmd, const QJsonArray& data)
{
	QJsonObject obj;
	obj["command"] = Subscription::toString(cmd);

	if (Subscription::isInstanceSpecific(cmd) && _instanceID != NO_INSTANCE_ID)
	{
		obj.insert("instance", _instanceID);
	}
	obj.insert("data", data);

	qCDebug(api_callback_msg).noquote() << "Emitting callback msg" << JsonUtils::toCompact(obj);

	emit callbackReady(obj);
}

void JsonCallbacks::doCallback(Subscription::Type cmd, const QJsonObject& data)
{
	QJsonObject obj;
	obj["command"] = Subscription::toString(cmd);

	if (Subscription::isInstanceSpecific(cmd) && _instanceID != NO_INSTANCE_ID)
	{
		obj.insert("instance", _instanceID);
	}
	obj.insert("data", data);

	qCDebug(api_callback_msg).noquote() << "Emitting callback msg" << JsonUtils::toCompact(obj);

	emit callbackReady(obj);
}

void JsonCallbacks::handleComponentState(hyperion::Components comp, bool state)
{
	QJsonObject data;
	data["name"] = componentToIdString(comp);
	data["enabled"] = state;

	doCallback(Subscription::ComponentsUpdate, data);
}

void JsonCallbacks::handlePriorityUpdate(int currentPriority, const PriorityMuxer::InputsMap& activeInputs)
{
	QJsonObject data;
	data["priorities"] = JsonInfo::getPrioritiestInfo(currentPriority, activeInputs);
	data["priorities_autoselect"] = _hyperionWeak.toStrongRef()->sourceAutoSelectEnabled();

	doCallback(Subscription::PrioritiesUpdate, data);
}

void JsonCallbacks::handleImageToLedsMappingChange(int mappingType)
{
	QJsonObject data;
	data["imageToLedMappingType"] = ImageProcessor::mappingTypeToStr(mappingType);

	doCallback(Subscription::ImageToLedMappingUpdate, data);
}

void JsonCallbacks::handleAdjustmentChange()
{
	doCallback(Subscription::AdjustmentUpdate, JsonInfo::getAdjustmentInfo(_hyperionWeak.toStrongRef(), _log));
}

void JsonCallbacks::handleVideoModeChange(VideoMode mode)
{
	QJsonObject data;
	data["videomode"] = QString(videoMode2String(mode));
	doCallback(Subscription::VideomodeUpdate, data);
}

#if defined(ENABLE_EFFECTENGINE)
void JsonCallbacks::handleEffectListChange()
{
	QJsonObject effects;
	effects["effects"] = JsonInfo::getEffects();
	doCallback(Subscription::EffectsUpdate, effects);
}
#endif

void JsonCallbacks::handleSettingsChange(settings::type type, const QJsonDocument& data)
{
	QJsonObject obj;
	if(data.isObject()) {
		obj[typeToString(type)] = data.object();
	} else {
		obj[typeToString(type)] = data.array();
	}

	doCallback(Subscription::SettingsUpdate, obj);
}

void JsonCallbacks::handleLedsConfigChange(settings::type type, const QJsonDocument& data)
{
	if(type == settings::LEDS)
	{
		QJsonObject obj;
		obj[typeToString(type)] = data.array();
		doCallback(Subscription::LedsUpdate, obj);
	}
}

void JsonCallbacks::handleInstanceChange()
{
	doCallback(Subscription::InstanceUpdate, JsonInfo::getInstanceInfo());
}

void JsonCallbacks::handleTokenChange(const QVector<AuthManager::AuthDefinition> &def)
{
	QJsonArray arr;
	for (const auto &entry : def)
	{
		QJsonObject sub;
		sub["comment"] = entry.comment;
		sub["id"] = entry.id;
		sub["last_use"] = entry.lastUse;
		arr.push_back(sub);
	}
	doCallback(Subscription::TokenUpdate, arr);
}

void JsonCallbacks::handleLedColorUpdate(const QVector<ColorRgb> &ledColors)
{
	qCDebug(api_callback_leds) << "Handle LED color update for" << ledColors.size() << "LEDs";
	// Take the LED update into a shared buffer and return quickly
	{
		QMutexLocker locker(&_ledColorsBufferMutex);
		_ledColorsUpdateBuffer = ledColors;
	}

	// If a frame processing is NOT already scheduled, schedule one.
	if (!_ledColorsUpdatePending.exchange(true))
	{
		QTimer::singleShot(0, this, &JsonCallbacks::processLedUpdate);
	}

	return; // Return immediately
}

void JsonCallbacks::processLedUpdate()
{
	qCDebug(api_callback_leds) << "Publish LED color update";
	QVector<ColorRgb> ledColorsToProcess;
	{
		QMutexLocker locker(&_ledColorsBufferMutex);
		ledColorsToProcess = _ledColorsUpdateBuffer;
	}

	qint64 const elapsedLedUpdateTime = _ledUpdateTimer.elapsed();
	qint64 const elapsedTimeMs = elapsedLedUpdateTime - _lastLedUpdateTime;
	if (_lastLedUpdateTime == 0 || elapsedTimeMs >= MAX_LED_DEVICE_DATA_EMISSION_INTERVAL.count())
	{
		QJsonObject result;
		QJsonArray leds;

		// Avoid copying by appending RGB values directly
		for (const auto& color : ledColorsToProcess)
		{
			leds.append(QJsonValue(color.red));
			leds.append(QJsonValue(color.green));
			leds.append(QJsonValue(color.blue));
		}
		result["leds"] = leds;

		doCallback(Subscription::LedColorsUpdate, result);
		_lastLedUpdateTime = elapsedLedUpdateTime;
		qCDebug(api_callback_leds) << "Published LED color update";		
	}
	else
	{
		// It's useful to know when we are skipping, but this can be very noisy.
		qCDebug(api_callback_leds) << "Skipping LED color update as last update was only" << elapsedTimeMs << "ms ago";
	}
	_ledColorsUpdatePending.store(false);
}

void JsonCallbacks::handleImageUpdate(const Image<ColorRgb>& image)
{
	qCDebug(api_callback_image) << "Handle image update for image [" << image.id() << "]";

	// Take the image update into a shared buffer and return quickly
	{
		QMutexLocker locker(&_imageBufferMutex);
		_imageUpdateBuffer = image;
	}

	// If a frame processing is NOT already scheduled, schedule one.
	if (!_imageUpdatePending.exchange(true))
	{
		QTimer::singleShot(0, this, &JsonCallbacks::processImageUpdate);
	}

	return; // Return immediately
}

void JsonCallbacks::processImageUpdate()
{
	Image<ColorRgb> imageToProcess;
	{
		QMutexLocker locker(&_imageBufferMutex);
		imageToProcess = _imageUpdateBuffer;
	}

	qCDebug(api_callback_image) << "Publish image update for image [" << imageToProcess.id() << "]";

	qint64 const elapsedImageUpdateTime = _imageUpdateTimer.elapsed();
	qint64 const elapsedTimeMs = elapsedImageUpdateTime - _lastImageUpdateTime;
	if ( _lastImageUpdateTime == 0 || elapsedTimeMs >= MAX_IMAGE_EMISSION_INTERVAL.count())
	{
		QImage jpgImage = std::as_const(imageToProcess).toQImage();
		if (_isImageSizeLimited && jpgImage.width() > MAX_CALLBACK_IMAGE_WIDTH)
		{
			jpgImage = jpgImage.scaledToWidth(MAX_CALLBACK_IMAGE_WIDTH, Qt::FastTransformation);
		}

		QByteArray byteArray;
		QBuffer buffer(&byteArray);
		buffer.open(QIODevice::WriteOnly);

		if (!jpgImage.save(&buffer, "JPG"))
		{
			qWarning() << "[processImageUpdate] Failed to convert image to JPG format.";
			return;
		}
		buffer.close();

		QJsonObject result;
		result["image"] = QStringLiteral("data:image/jpg;base64,") + QString::fromLatin1(byteArray.toBase64());

		doCallback(Subscription::ImageUpdate, result);
		_lastImageUpdateTime = elapsedImageUpdateTime;
		qCDebug(api_callback_image) << "Published image update for image [" << imageToProcess.id() << "]";
	}
	else
	{
		qCDebug(api_callback_image) << "Skipping image [" << imageToProcess.id() << "] update as last update was only" << elapsedTimeMs << "ms ago";
	}
	_imageUpdatePending.store(false);
}

void JsonCallbacks::handleLogMessageUpdate(const Logger::T_LOG_MESSAGE &msg)
{
	QJsonObject result;
	QJsonObject message;
	QJsonArray messageArray;

	if (!_islogMsgStreamingActive)
	{
		_islogMsgStreamingActive = true;
		QMetaObject::invokeMethod(LoggerManager::getInstance().data(), "getLogMessageBuffer",
								  Qt::DirectConnection,
								  Q_RETURN_ARG(QJsonArray, messageArray),
								  Q_ARG(Logger::LogLevel, _log->getLogLevel()));
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

	doCallback(Subscription::LogMsgUpdate, result);
}

void JsonCallbacks::handleEventUpdate(const Event &event)
{
	QJsonObject result;

	result["event"] = eventToString(event);

	doCallback(Subscription::EventUpdate, result);
}
