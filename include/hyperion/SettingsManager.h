#pragma once

#include <utils/Logger.h>
#include <utils/settings.h>

// qt includes
#include <QJsonObject>

class Hyperion;
class SettingsTable;

///
/// @brief Manage the settings read write from/to configuration file, on settings changed will emit a signal to update components accordingly
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
	SettingsManager(quint8 instance, QObject* parent = nullptr, bool readonlyMode = false);

	///
	/// @brief Save a complete json configuration
	/// @param config  The entire config object
	/// @param correct If true will correct json against schema before save
	/// @return True on success else false
	///
	bool saveSettings(QJsonObject config, bool correct = false);

	///
	/// @brief get a single setting json from configuration
	/// @param type   The settings::type from enum
	/// @return The requested json data as QJsonDocument
	///
	QJsonDocument getSetting(settings::type type) const;

	///
	/// @brief get the full settings object of this instance (with global settings)
	/// @return The requested json
	///
	const QJsonObject & getSettings() const { return _qconfig; }

signals:
	///
	/// @brief Emits whenever a configuration part changed.
	/// @param type The settings type from enum
	/// @param data The data as QJsonDocument
	///
	void settingsChanged(settings::type type, const QJsonDocument& data);

private:
	///
	/// @brief Add possible migrations steps for configuration here
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

	/// the current configuration of this instance
	QJsonObject _qconfig;

	bool	_readonlyMode;
};
