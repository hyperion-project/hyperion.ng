#pragma once

#include <utils/Logger.h>
#include <utils/settings.h>

// qt incl
#include <QJsonObject>

class Hyperion;
class SettingsTable;

///
/// @brief Manage the settings read write from/to config file, on settings changed will emit a signal to update components accordingly
///
class SettingsManager : public QObject
{
	Q_OBJECT
public:
	///
	/// @brief Construct a settings manager and assign a hyperion instance
	/// @params  instance   Instance index of HyperionInstanceManager
	/// @params  parent    The parent hyperion instance
	///
	SettingsManager(quint8 instance, QObject* parent = nullptr);

	///
	/// @brief Save a complete json config
	/// @param config  The entire config object
	/// @param correct If true will correct json against schema before save
	/// @return True on success else false
	///
	bool saveSettings(QJsonObject config, bool correct = false);

	///
	/// @brief get a single setting json from config
	/// @param type   The settings::type from enum
	/// @return The requested json data as QJsonDocument
	///
	QJsonDocument getSetting(settings::type type) const;

	///
	/// @brief get the full settings object of this instance (with global settings)
	/// @return The requested json
	///
	const QJsonObject & getSettings() const { return _qconfig; };

signals:
	///
	/// @brief Emits whenever a config part changed.
	/// @param type The settings type from enum
	/// @param data The data as QJsonDocument
	///
	void settingsChanged(settings::type type, const QJsonDocument& data);

private:
	///
	/// @brief Add possile migrations steps for config here
	/// @param config The configuration object
	/// @return True when a migration has been triggered
	///
	bool handleConfigUpgrade(QJsonObject& config);


	/// Hyperion instance
	Hyperion* _hyperion;

	/// Logger instance
	Logger* _log;

	/// instance of database table interface
	SettingsTable* _sTable;

	/// the schema
	static QJsonObject schemaJson;

	/// the current config of this instance
	QJsonObject _qconfig;
};
