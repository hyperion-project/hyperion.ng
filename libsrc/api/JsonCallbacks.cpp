// proj incl
#include <api/JsonCallbacks.h>
#include <api/JsonInfo.h>
#include <api/JsonApiSubscription.h>

// hyperion
#include <hyperion/Hyperion.h>

// HyperionIManager
#include <hyperion/HyperionIManager.h>
// components

#include <hyperion/ComponentRegister.h>
// priorityMuxer

#include <hyperion/PriorityMuxer.h>

// utils
#include <utils/ColorSys.h>

// qt
#include <QDateTime>
#include <QVariant>
#include <QImage>
#include <QBuffer>

// Image to led map helper

#include <hyperion/ImageProcessor.h>

using namespace hyperion;

JsonCallbacks::JsonCallbacks(Logger *log, const QString& peerAddress, QObject* parent)
	: QObject(parent)
	, _log (log)
	, _hyperion(nullptr)
	, _peerAddress (peerAddress)
	, _componentRegister(nullptr)
	, _prioMuxer(nullptr)
	, _islogMsgStreamingActive(false)
{
	qRegisterMetaType<PriorityMuxer::InputsMap>("InputsMap");
}

bool JsonCallbacks::subscribe(const Subscription::Type cmd)
{
	switch (cmd) {
	case Subscription::AdjustmentUpdate:
		connect(_hyperion, &Hyperion::adjustmentChanged, this, &JsonCallbacks::handleAdjustmentChange);
	break;
	case Subscription::ComponentsUpdate:
		connect(_componentRegister, &ComponentRegister::updatedComponentState, this, &JsonCallbacks::handleComponentState);
	break;
#if defined(ENABLE_EFFECTENGINE)
	case Subscription::EffectsUpdate:
		connect(_hyperion, &Hyperion::effectListUpdated, this, &JsonCallbacks::handleEffectListChange);
	break;
#endif
	case Subscription::ImageToLedMappingUpdate:
		connect(_hyperion, &Hyperion::imageToLedsMappingChanged, this, &JsonCallbacks::handleImageToLedsMappingChange);
	break;
	case Subscription::ImageUpdate:
		connect(_hyperion,  &Hyperion::currentImage, this, &JsonCallbacks::handleImageUpdate);
	break;
	case Subscription::InstanceUpdate:
		connect(HyperionIManager::getInstance(), &HyperionIManager::change, this, &JsonCallbacks::handleInstanceChange);
	break;
	case Subscription::LedColorsUpdate:
		connect(_hyperion, &Hyperion::rawLedColors, this, &JsonCallbacks::handleLedColorUpdate);
	break;
	case Subscription::LedsUpdate:
		connect(_hyperion, &Hyperion::settingsChanged, this, &JsonCallbacks::handleLedsConfigChange);
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
	case Subscription::PrioritiesUpdate:
		connect(_prioMuxer, &PriorityMuxer::prioritiesChanged, this, &JsonCallbacks::handlePriorityUpdate);
	break;
	case Subscription::SettingsUpdate:
		connect(_hyperion, &Hyperion::settingsChanged, this, &JsonCallbacks::handleSettingsChange);
	break;
	case Subscription::TokenUpdate:
		connect(AuthManager::getInstance(), &AuthManager::tokenChange, this, &JsonCallbacks::handleTokenChange, Qt::AutoConnection);
	break;
	case Subscription::VideomodeUpdate:
		connect(_hyperion, &Hyperion::newVideoMode, this, &JsonCallbacks::handleVideoModeChange);
	break;

	default:
	return false;
	}

	_subscribedCommands.insert(cmd);

	return true;
}

bool JsonCallbacks::subscribe(const QString& cmd)
{
	JsonApiSubscription subscription = ApiSubscriptionRegister::getSubscriptionInfo(cmd);
	if (subscription.cmd == Subscription::Unknown)
	{
		return false;
	}
	return subscribe(subscription.cmd);
}

QStringList JsonCallbacks::subscribe(const QJsonArray& subscriptions)
{
	QJsonArray subsArr;
	if (subscriptions.contains("all"))
	{
		for (const auto& entry : getCommands(false))
		{
			subsArr.append(entry);
		}
	}
	else
	{
		subsArr = subscriptions;
	}

	QStringList invalidSubscriptions;
	for (auto it = subsArr.begin(); it != subsArr.end(); ++it)
	{
		const QJsonValue& entry = *it;
		if (!subscribe(entry.toString()))
		{
			invalidSubscriptions.append(entry.toString());
		}
	}
	return invalidSubscriptions;
}

bool JsonCallbacks::unsubscribe(const Subscription::Type cmd)
{
	_subscribedCommands.remove(cmd);

	switch (cmd) {
	case Subscription::AdjustmentUpdate:
		disconnect(_hyperion, &Hyperion::adjustmentChanged, this, &JsonCallbacks::handleAdjustmentChange);
	break;
	case Subscription::ComponentsUpdate:
		disconnect(_componentRegister, &ComponentRegister::updatedComponentState, this, &JsonCallbacks::handleComponentState);
	break;
#if defined(ENABLE_EFFECTENGINE)
	case Subscription::EffectsUpdate:
		disconnect(_hyperion, &Hyperion::effectListUpdated, this, &JsonCallbacks::handleEffectListChange);
	break;
#endif
	case Subscription::ImageToLedMappingUpdate:
		disconnect(_hyperion, &Hyperion::imageToLedsMappingChanged, this, &JsonCallbacks::handleImageToLedsMappingChange);
	break;
	case Subscription::ImageUpdate:
		disconnect(_hyperion, &Hyperion::currentImage, this, &JsonCallbacks::handleImageUpdate);
	break;
	case Subscription::InstanceUpdate:
		disconnect(HyperionIManager::getInstance(), &HyperionIManager::change, this, &JsonCallbacks::handleInstanceChange);
	break;
	case Subscription::LedColorsUpdate:
		disconnect(_hyperion, &Hyperion::rawLedColors, this, &JsonCallbacks::handleLedColorUpdate);
	break;
	case Subscription::LedsUpdate:
		disconnect(_hyperion, &Hyperion::settingsChanged, this, &JsonCallbacks::handleLedsConfigChange);
	break;
	case Subscription::LogMsgUpdate:
		disconnect(LoggerManager::getInstance().data(), &LoggerManager::newLogMessage, this, &JsonCallbacks::handleLogMessageUpdate);
		if (_islogMsgStreamingActive)
		{
			_islogMsgStreamingActive = false;
			Debug(_log, "log streaming deactivated for client  %s", _peerAddress.toStdString().c_str());
		}
	break;
	case Subscription::PrioritiesUpdate:
		disconnect(_prioMuxer, &PriorityMuxer::prioritiesChanged, this, &JsonCallbacks::handlePriorityUpdate);
	break;
	case Subscription::SettingsUpdate:
		disconnect(_hyperion, &Hyperion::settingsChanged, this, &JsonCallbacks::handleSettingsChange);
	break;
	case Subscription::TokenUpdate:
		disconnect(AuthManager::getInstance(), &AuthManager::tokenChange, this, &JsonCallbacks::handleTokenChange);
	break;
	case Subscription::VideomodeUpdate:
		disconnect(_hyperion, &Hyperion::newVideoMode, this, &JsonCallbacks::handleVideoModeChange);
	break;

	default:
	return false;
	}
	return true;
}

bool JsonCallbacks::unsubscribe(const QString& cmd)
{
	JsonApiSubscription subscription = ApiSubscriptionRegister::getSubscriptionInfo(cmd);
	if (subscription.cmd == Subscription::Unknown)
	{
		return false;
	}
	return unsubscribe(subscription.cmd);
}

QStringList JsonCallbacks::unsubscribe(const QJsonArray& subscriptions)
{
	QJsonArray subsArr;
	if (subscriptions.contains("all"))
	{
		for (const auto& entry : getCommands(false))
		{
			subsArr.append(entry);
		}
	}
	else
	{
		subsArr = subscriptions;
	}

	QStringList invalidSubscriptions;
	for (auto it = subsArr.begin(); it != subsArr.end(); ++it)
	{
		const QJsonValue& entry = *it;
		if (!unsubscribe(entry.toString()))
		{
			invalidSubscriptions.append(entry.toString());
		}
	}
	return invalidSubscriptions;
}

void JsonCallbacks::resetSubscriptions()
{
	for (QSet<Subscription::Type>::const_iterator it = _subscribedCommands.constBegin(); it != _subscribedCommands.constEnd(); ++it)
	{
		unsubscribe(*it);
	}
}

void JsonCallbacks::setSubscriptionsTo(Hyperion* hyperion)
{
	assert(hyperion);

	// get current subs
	const QSet<Subscription::Type> currSubs(_subscribedCommands);

	// stop subs
	resetSubscriptions();

	// update pointer
	_hyperion = hyperion;
	_componentRegister = _hyperion->getComponentRegister();
	_prioMuxer = _hyperion->getMuxerInstance();

	// re-apply subs
	for(const auto & entry : currSubs)
	{
		subscribe(entry);
	}
}

QStringList JsonCallbacks::getCommands(bool fullList) const
{
	QStringList commands;
	for (JsonApiSubscription subscription : ApiSubscriptionRegister::getSubscriptionLookup())
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
	for (Subscription::Type cmd : _subscribedCommands)
	{
		commands << Subscription::toString(cmd);
	}
	return commands;
}

void JsonCallbacks::doCallback(Subscription::Type cmd, const QVariant& data)
{
	QJsonObject obj;
	obj["instance"] = _hyperion->getInstanceIndex();
	obj["command"] = Subscription::toString(cmd);

	if (data.userType() == QMetaType::QJsonArray) {
		obj["data"] = data.toJsonArray();
	} else {
		obj["data"] = data.toJsonObject();
	}

	emit newCallback(obj);
}

void JsonCallbacks::handleComponentState(hyperion::Components comp, bool state)
{
	QJsonObject data;
	data["name"] = componentToIdString(comp);
	data["enabled"] = state;

	doCallback(Subscription::ComponentsUpdate, QVariant(data));
}

void JsonCallbacks::handlePriorityUpdate(int currentPriority, const PriorityMuxer::InputsMap& activeInputs)
{
	QJsonObject data;
	data["priorities"] = JsonInfo::getPrioritiestInfo(currentPriority, activeInputs);
	data["priorities_autoselect"] = _hyperion->sourceAutoSelectEnabled();

	doCallback(Subscription::PrioritiesUpdate, QVariant(data));
}

void JsonCallbacks::handleImageToLedsMappingChange(int mappingType)
{
	QJsonObject data;
	data["imageToLedMappingType"] = ImageProcessor::mappingTypeToStr(mappingType);

	doCallback(Subscription::ImageToLedMappingUpdate, QVariant(data));
}

void JsonCallbacks::handleAdjustmentChange()
{
	doCallback(Subscription::AdjustmentUpdate, JsonInfo::getAdjustmentInfo(_hyperion,_log));
}

void JsonCallbacks::handleVideoModeChange(VideoMode mode)
{
	QJsonObject data;
	data["videomode"] = QString(videoMode2String(mode));
	doCallback(Subscription::VideomodeUpdate, QVariant(data));
}

#if defined(ENABLE_EFFECTENGINE)
void JsonCallbacks::handleEffectListChange()
{
	QJsonObject effects;
	effects["effects"] = JsonInfo::getEffects(_hyperion);
	doCallback(Subscription::EffectsUpdate, QVariant(effects));
}
#endif

void JsonCallbacks::handleSettingsChange(settings::type type, const QJsonDocument& data)
{
	QJsonObject dat;
	if(data.isObject()) {
		dat[typeToString(type)] = data.object();
	} else {
		dat[typeToString(type)] = data.array();
	}

	doCallback(Subscription::SettingsUpdate, QVariant(dat));
}

void JsonCallbacks::handleLedsConfigChange(settings::type type, const QJsonDocument& data)
{
	if(type == settings::LEDS)
	{
		QJsonObject dat;
		dat[typeToString(type)] = data.array();
		doCallback(Subscription::LedsUpdate, QVariant(dat));
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
	doCallback(Subscription::TokenUpdate, QVariant(arr));
}

void JsonCallbacks::handleLedColorUpdate(const std::vector<ColorRgb> &ledColors)
{
	QJsonObject result;
	QJsonArray leds;

	for (const auto &color : ledColors)
	{
		leds << QJsonValue(color.red) << QJsonValue(color.green) << QJsonValue(color.blue);
	}
	result["leds"] = leds;

	doCallback(Subscription::LedColorsUpdate, QVariant(result));
}

void JsonCallbacks::handleImageUpdate(const Image<ColorRgb> &image)
{
	QImage jpgImage(reinterpret_cast<const uchar*>(image.memptr()), image.width(), image.height(), qsizetype(3) * image.width(), QImage::Format_RGB888);
	QByteArray byteArray;
	QBuffer buffer(&byteArray);
	buffer.open(QIODevice::WriteOnly);
	jpgImage.save(&buffer, "jpg");

	QJsonObject result;
	result["image"] = "data:image/jpg;base64," + QString(byteArray.toBase64());

	doCallback(Subscription::ImageUpdate, QVariant(result));
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

	doCallback(Subscription::LogMsgUpdate, QVariant(result));
}
