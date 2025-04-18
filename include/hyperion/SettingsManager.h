#pragma once

#include <utils/Logger.h>
#include <utils/settings.h>

#include <db/SettingsTable.h>

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
	SettingsManager(quint8 instance = NO_INSTANCE_ID, QObject* parent = nullptr);

	///
	/// @brief Save a complete JSON configuration
	/// @param config  The entire config object
	/// @return True on success else false, plus validation errors
	///
	QPair<bool, QStringList> saveSettings(const QJsonObject& config);

	///
	/// @brief Correct a complete JSON configuration
	/// @param config  The entire config object
	/// @return True on success else false, plus correction details
	///
	QPair<bool, QStringList> correctSettings(QJsonObject& config);

	///
	/// @brief get a single setting json from configuration
	/// @param type   The settings::type from enum
	/// @return The requested json data as QJsonDocument
	///
	QJsonDocument getSetting(settings::type type) const;

	///
	/// @brief get a single setting json from configuration
	/// @param type   The type as string
	/// @return The requested json data as QJsonDocument
	///
	QJsonDocument getSetting(const QString& type) const;
	
	///
	/// @brief get the selected settings objects of this instance (including global settings)
	/// @return The requested json
	///
	QJsonObject getSettings(const QStringList& filteredTypes = {}) const;
	QJsonObject getSettings(const QVariant& instance, const QStringList& filteredTypes = {} ) const;

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
	bool upgradeConfig(QJsonObject& config);


	/// Logger instance
	Logger* _log;

	/// Instance number
	quint8 _instance;

	/// instance of database table interface
	SettingsTable* _sTable;

	/// the current configuration of this instance
	QJsonObject _qconfig;

};
