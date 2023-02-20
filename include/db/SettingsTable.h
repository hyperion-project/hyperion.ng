#pragma once

// hyperion
#include <db/DBManager.h>

// qt
#include <QDateTime>
#include <QJsonDocument>

///
/// @brief settings table db interface
///
class SettingsTable : public DBManager
{

public:
	/// construct wrapper with settings table
	SettingsTable(quint8 instance, QObject* parent = nullptr)
		: DBManager(parent)
		, _hyperion_inst(instance)
	{
		setTable("settings");
		// create table columns
		createTable(QStringList()<<"type TEXT"<<"config TEXT"<<"hyperion_inst INTEGER"<<"updated_at TEXT");
	};

	///
	/// @brief      Create or update a settings record
	/// @param[in]  type           type of setting
	/// @param[in]  config         The configuration data
	/// @return     true on success else false
	///
	inline bool createSettingsRecord(const QString& type, const QString& config) const
	{
		QVariantMap map;
		map["config"] = config;
		map["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

		VectorPair cond;
		cond.append(CPair("type",type));
		// when a setting is not global we are searching also for the instance
		if(!isSettingGlobal(type))
			cond.append(CPair("AND hyperion_inst",_hyperion_inst));
		return createRecord(cond, map);
	}

	///
	/// @brief      Test if record exist, type can be global setting or local (instance)
	/// @param[in]  type           type of setting
	/// @param[in]  hyperion_inst  The instance of hyperion assigned (might be empty)
	/// @return     true on success else false
	///
	inline bool recordExist(const QString& type) const
	{
		VectorPair cond;
		cond.append(CPair("type",type));
		// when a setting is not global we are searching also for the instance
		if(!isSettingGlobal(type))
			cond.append(CPair("AND hyperion_inst",_hyperion_inst));
		return recordExists(cond);
	}

	///
	/// @brief Get 'config' column of settings entry as QJsonDocument
	/// @param[in]  type   The settings type
	/// @return            The QJsonDocument
	///
	inline QJsonDocument getSettingsRecord(const QString& type) const
	{
		QVariantMap results;
		VectorPair cond;
		cond.append(CPair("type",type));
		// when a setting is not global we are searching also for the instance
		if(!isSettingGlobal(type))
			cond.append(CPair("AND hyperion_inst",_hyperion_inst));
		getRecord(cond, results, QStringList("config"));
		return QJsonDocument::fromJson(results["config"].toByteArray());
	}

	///
	/// @brief Get 'config' column of settings entry as QString
	/// @param[in]  type   The settings type
	/// @return            The QString
	///
	inline QString getSettingsRecordString(const QString& type) const
	{
		QVariantMap results;
		VectorPair cond;
		cond.append(CPair("type",type));
		// when a setting is not global we are searching also for the instance
		if(!isSettingGlobal(type))
			cond.append(CPair("AND hyperion_inst",_hyperion_inst));
		getRecord(cond, results, QStringList("config"));
		return results["config"].toString();
	}

	///
	/// @brief Delete all settings entries associated with this instance, called from InstanceTable of HyperionIManager
	///
	inline void deleteInstance() const
	{
		VectorPair cond;
		cond.append(CPair("hyperion_inst",_hyperion_inst));
		deleteRecord(cond);
	}

	inline bool isSettingGlobal(const QString& type) const
	{
		// list of global settings
		QStringList list;
		// server port services
		list << "jsonServer" << "protoServer" << "flatbufServer" << "forwarder" << "webConfig" << "network"
		// capture
		<< "framegrabber" << "grabberV4L2" << "grabberAudio"
		// other
		<< "logger" << "general";

		return list.contains(type);
	}

private:
	const quint8 _hyperion_inst;
};
