#ifndef SETTINGSTABLE_H
#define SETTINGSTABLE_H

#include <db/DBManager.h>
#include <utils/version.hpp>

#include <QJsonDocument>

const int GLOABL_INSTANCE_ID = std::numeric_limits<quint8>::max();;
const char DEFAULT_CONFIG_VERSION[] = "2.0.0-alpha.8";

///
/// @brief settings table db interface
///
class SettingsTable : public DBManager
{

public:
	/// construct wrapper with settings table
	SettingsTable(quint8 instance = GLOABL_INSTANCE_ID, QObject* parent = nullptr);

	///
	/// @brief      Create or update a settings record
	/// @param[in]  type           type of setting
	/// @param[in]  config         The configuration data
	/// @return     true on success else false
	///
	bool createSettingsRecord(const QString& type, const QString& config) const;

	///
	/// @brief      Test if record exist, type can be global setting or local (instance)
	/// @param[in]  type           type of setting
	/// @param[in]  hyperion_inst  The instance of hyperion assigned (might be empty)
	/// @return     true on success else false
	///
	bool recordExist(const QString& type) const;

	///
	/// @brief Get 'config' column of settings entry as QJsonDocument
	/// @param[in]  type   The settings type
	/// @return            The QJsonDocument
	///
	QJsonDocument getSettingsRecord(const QString& type) const;

	///
	/// @brief Get 'config' column of settings entry as QString
	/// @param[in]  type   The settings type
	/// @return            The QString
	///
	QString getSettingsRecordString(const QString& type) const;

	QJsonObject getSettings(const QStringList& filteredTypes = {} ) const;
	QJsonObject getSettings(const QVariant& instance, const QStringList& filteredTypes = {} ) const;

	QStringList nonExtingTypes() const;
	QPair<bool, QStringList> addMissingDefaults();

	///
	/// @brief Delete all settings entries associated with this instance, called from InstanceTable of HyperionIManager
	///
	void deleteInstance() const;

	const QVector<QString>& getGlobalSettingTypes() const;
	bool isGlobalSettingType(const QString& type) const;

	const QVector<QString>& getInstanceSettingTypes() const;
	bool isInstanceSettingType(const QString& type) const;

	const QJsonObject& getDefaultSettings() const;

	semver::version getConfigVersion();
	QString getConfigVersionString();

	bool resolveConfigVersion();
	bool resolveConfigVersion(QJsonObject generalConfig);

private:
	QString fixVersion(const QString& version);

	QVector<QString> initializeGlobalSettingTypes() const;
	static QVector<QString> globalSettingTypes;
	static bool areGlobalSettingTypesInitialised;

	QVector<QString> initializeInstanceSettingTypes() const;
	static QVector<QString> instanceSettingTypes;
	static bool areInstanceSettingTypesInitialised;

	QJsonObject initializeDefaultSettings() const;
	static QJsonObject defaultSettings;
	static bool areDefaultSettingsInitialised;

	const quint8 _instance;
	semver::version _configVersion;
};

#endif // SETTINGSTABLE_H
