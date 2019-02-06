#pragma once

// qt incl
#include <QObject>
#include <QJsonObject>

// components def
#include <utils/Components.h>
// bonjour
#include <bonjour/bonjourrecord.h>
// videModes
#include <utils/VideoMode.h>
// settings
#include <utils/settings.h>

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
	/// @param cmd   The cmd which will be subscribed for
	/// @return      True on success, false if not found
	///
	bool subscribeFor(const QString& cmd);

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
	void handleComponentState(const hyperion::Components comp, const bool state);

	///
	/// @brief handle emits from bonjour wrapper
	/// @param  bRegisters   The full register map
	///
	void handleBonjourChange(const QMap<QString,BonjourRecord>& bRegisters);

	///
	/// @brief handle emits from PriorityMuxer
	///
	void handlePriorityUpdate();

	///
	/// @brief Handle imageToLedsMapping updates
	///
	void handleImageToLedsMappingChange(const int& mappingType);

	///
	/// @brief Handle the adjustment update
	///
	void handleAdjustmentChange();

	///
	/// @brief Handle video mode change
	/// @param mode  The new videoMode
	///
	void handleVideoModeChange(const VideoMode& mode);

	///
	/// @brief Handle effect list change
	///
	void handleEffectListChange();

	///
	/// @brief Handle a config part change. This does NOT include (global) changes from other hyperion instances
	/// @param type   The settings type from enum
	/// @param data   The data as QJsonDocument
	///
	void handleSettingsChange(const settings::type& type, const QJsonDocument& data);

private:
	/// pointer of Hyperion instance
	Hyperion* _hyperion;
	/// pointer of comp register
	ComponentRegister* _componentRegister;
	/// Bonjour instance
	BonjourBrowserWrapper* _bonjour;
	/// priority muxer instance
	PriorityMuxer* _prioMuxer;
	/// contains all available commands
	QStringList _availableCommands;
	/// contains active subscriptions
	QStringList _subscribedCommands;
	/// construct callback msg
	void doCallback(const QString& cmd, const QVariant& data);
};
