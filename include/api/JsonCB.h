#pragma once

// qt incl
#include <QObject>
#include <QJsonObject>

// components def
#include <utils/Components.h>
// bonjour
#ifdef ENABLE_AVAHI
#include <bonjour/bonjourrecord.h>
#endif
// videModes
#include <utils/VideoMode.h>
// settings
#include <utils/settings.h>
// AuthManager
#include <hyperion/AuthManager.h>

class Hyperion;
class ComponentRegister;
class BonjourBrowserWrapper;
class PriorityMuxer;

class JsonCB : public QObject
{
	Q_OBJECT

public:
	JsonCB(QObject* parent);

	///
	/// @brief Subscribe to future data updates given by cmd
	/// @param cmd          The cmd which will be subscribed for
	/// @param unsubscribe  Revert subscription
	/// @return      True on success, false if not found
	///
	bool subscribeFor(const QString& cmd, bool unsubscribe = false);

	///
	/// @brief Get all possible commands to subscribe for
	/// @return  The list of commands
	///
	QStringList getCommands() { return _availableCommands; };

	///
	/// @brief Get all subscribed commands
	/// @return  The list of commands
	///
	QStringList getSubscribedCommands() { return _subscribedCommands; };

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
#ifdef ENABLE_AVAHI
	///
	/// @brief handle emits from bonjour wrapper
	/// @param  bRegisters   The full register map
	///
	void handleBonjourChange(const QMap<QString,BonjourRecord>& bRegisters);
#endif
	///
	/// @brief handle emits from PriorityMuxer
	///
	void handlePriorityUpdate();

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

	///
	/// @brief Handle effect list change
	///
	void handleEffectListChange();

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

private:
	/// pointer of Hyperion instance
	Hyperion* _hyperion;
	/// pointer of comp register
	ComponentRegister* _componentRegister;
#ifdef ENABLE_AVAHI
	/// Bonjour instance
	BonjourBrowserWrapper* _bonjour;
#endif
	/// priority muxer instance
	PriorityMuxer* _prioMuxer;
	/// contains all available commands
	QStringList _availableCommands;
	/// contains active subscriptions
	QStringList _subscribedCommands;
	/// construct callback msg
	void doCallback(const QString& cmd, const QVariant& data);
};
