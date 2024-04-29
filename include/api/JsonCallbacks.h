#pragma once

#include "api/JsonApiSubscription.h"
#include <api/API.h>
#include <events/EventEnum.h>

// qt incl
#include <QObject>
#include <QJsonObject>
#include <QSet>

#include <utils/Components.h>
#include <utils/VideoMode.h>
#include <utils/settings.h>
#include <hyperion/AuthManager.h>
#include <hyperion/PriorityMuxer.h>

class Hyperion;
class ComponentRegister;
class PriorityMuxer;

class JsonCallbacks : public QObject
{
	Q_OBJECT

public:
	JsonCallbacks(Logger* log, const QString& peerAddress, QObject* parent);

	///
	/// @brief Subscribe to future data updates given by cmd
	/// @param cmd   The cmd which will be subscribed for
	/// @return      True on success, false if not found
	///
	bool subscribe(const QString& cmd);

	///
	/// @brief Subscribe to future data updates given by subscription list
	/// @param type   Array of subscriptionsm
	///
	QStringList subscribe(const QJsonArray& subscriptions);

	///
	/// @brief Subscribe to future data updates given by cmd
	/// @param cmd   The cmd which will be subscribed to
	/// @return      True on success, false if not found
	///
	bool subscribe(Subscription::Type subscription);

	///
	/// @brief Unsubscribe to future data updates given by cmd
	/// @param cmd   The cmd which will be unsubscribed
	/// @return      True on success, false if not found
	///
	bool unsubscribe(const QString& cmd);

	///
	/// @brief Unsubscribe to future data updates given by subscription list
	/// @param type   Array of subscriptions
	///
	QStringList unsubscribe(const QJsonArray& subscriptions);

	///
	/// @brief Unsubscribe to future data updates given by cmd
	/// @param cmd   The cmd which will be subscribed to
	/// @return      True on success, false if not found
	///
	bool unsubscribe(Subscription::Type cmd);

	///
	/// @brief Get all possible commands to subscribe for
	/// @param fullList Return all possible commands or those not triggered by API requests (subscriptions="ALL")
	/// @return  The list of commands
	///
	QStringList getCommands(bool fullList = true) const;
	///
	/// @brief Get all subscribed commands
	/// @return  The list of commands
	///
	QStringList getSubscribedCommands() const;

	///
	/// @brief Reset subscriptions, disconnect all signals
	///
	void resetSubscriptions();

	///
	/// @brief Re-apply all current subs to a new Hyperion instance, the connections to the old instance will be dropped
	///
	void setSubscriptionsTo(Hyperion* hyperion);

signals:
	///
	/// @brief Emits whenever a new json mesage callback is ready to send
	/// @param The JsonObject message
	///
	void newCallback(QJsonObject);

private slots:
	///
	/// @brief handle component state changes
	///
	void handleComponentState(hyperion::Components comp, bool state);

	///
	/// @brief handle emits from PriorityMuxer
	/// @param  currentPriority The current priority at time of emit
	/// @param  activeInputs The current active input map at time of emit
	///
	void handlePriorityUpdate(int currentPriority, const PriorityMuxer::InputsMap& activeInputs);

	///
	/// @brief Handle imageToLedsMapping updates
	///
	void handleImageToLedsMappingChange(int mappingType);

	///
	/// @brief Handle the adjustment update
	///
	void handleAdjustmentChange();

	///
	/// @brief Handle video mode change
	/// @param mode  The new videoMode
	///
	void handleVideoModeChange(VideoMode mode);

#if defined(ENABLE_EFFECTENGINE)
	///
	/// @brief Handle effect list change
	///
	void handleEffectListChange();
#endif

	///
	/// @brief Handle a config part change. This does NOT include (global) changes from other hyperion instances
	/// @param type   The settings type from enum
	/// @param data   The data as QJsonDocument
	///
	void handleSettingsChange(settings::type type, const QJsonDocument& data);

	///
	/// @brief Handle led config specific updates (required for led color streaming with positional display)
	/// @param type   The settings type from enum
	/// @param data   The data as QJsonDocument
	///
	void handleLedsConfigChange(settings::type type, const QJsonDocument& data);

	///
	/// @brief Handle Hyperion instance manager change
	///
	void handleInstanceChange();

	///
	/// @brief Handle AuthManager token changes
	///
	void handleTokenChange(const QVector<AuthManager::AuthDefinition> &def);

	///
	/// @brief Is called whenever the current Hyperion instance pushes new led raw values (if enabled)
	/// @param ledColors  The current led colors
	///
	void handleLedColorUpdate(const std::vector<ColorRgb> &ledColors);

	///
	/// @brief Is called whenever the current Hyperion instance pushes new image update (if enabled)
	/// @param image  The current image
	///
	void handleImageUpdate(const Image<ColorRgb> &image);

	///
	/// @brief Process and push new log messages from logger (if enabled)
	///
	void handleLogMessageUpdate(const Logger::T_LOG_MESSAGE &);

	///
	/// @brief Is called whenever an event is triggert
	/// @param image  The current event
	///
	void handleEventUpdate(const Event &event);

private:

	/// construct callback msg
	void doCallback(Subscription::Type cmd, const QVariant& data);

	Logger *_log;
	Hyperion* _hyperion;

	/// The peer address of the client
	QString _peerAddress;

	/// pointer of comp register
	ComponentRegister* _componentRegister;

	/// priority muxer instance
	PriorityMuxer* _prioMuxer;

	/// contains active subscriptions
	QSet<Subscription::Type> _subscribedCommands;

	/// flag to determine state of log streaming
	bool _islogMsgStreamingActive;
};
