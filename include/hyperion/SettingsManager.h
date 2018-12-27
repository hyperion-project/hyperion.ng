#pragma once

#include <utils/Logger.h>
#include <utils/settings.h>

// qt incl
#include <QJsonObject>

class SettingsTable;
class Hyperion;

///
/// @brief Manage the settings read write from/to database, on settings changed will emit a signal to update components accordingly
///
class SettingsManager : public QObject
{
	Q_OBJECT
public:
	///
	/// @brief Construct a settings manager and assign a hyperion instance
	/// @params  hyperion   The parent hyperion instance
	/// @params  instance   Instance number of Hyperion
	///
	SettingsManager(Hyperion* hyperion, const quint8& instance, const QString& configFile);

	///
	/// @brief Construct a settings manager for HyperionDaemon
	///
	SettingsManager(const quint8& instance, const QString& configFile);
	~SettingsManager();

	///
	/// @brief Save a complete json config
	/// @param config  The entire config object
	/// @param correct If true will correct json against schema before save
	/// @return        True on success else false
	///
	const bool saveSettings(QJsonObject config, const bool& correct = false);

	///
	/// @brief get a single setting json from database
	/// @param  type   The settings::type from enum
	/// @return        The requested json data as QJsonDocument
	///
	const QJsonDocument getSetting(const settings::type& type);

	///
	/// @brief get the full settings object of this instance (with global settings)
	/// @return        The requested json
	///
	const QJsonObject & getSettings() { return _qconfig; };

signals:
	///
	/// @brief Emits whenever a config part changed. Comparison of database and new data to prevent false positive
	/// @param type   The settings type from enum
	/// @param data   The data as QJsonDocument
	///
	void settingsChanged(const settings::type& type, const QJsonDocument& data);

private:
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
