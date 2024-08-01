#ifndef SETTINGSTABLE_H
#define SETTINGSTABLE_H

#include <db/DBManager.h>

#include <QJsonDocument>

const int GLOABL_INSTANCE_ID = 0;

///
/// @brief settings table db interface
///
class SettingsTable : public DBManager
{

public:
	/// construct wrapper with settings table
	SettingsTable(quint8 instance, QObject* parent = nullptr);

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

	///
	/// @brief Delete all settings entries associated with this instance, called from InstanceTable of HyperionIManager
	///
	void deleteInstance() const;

	static const QVector<QString>& getGlobalSettingTypes();

	static bool isSettingGlobal(const QString& type);

private:
	const quint8 _hyperion_inst;
};

#endif // SETTINGSTABLE_H
