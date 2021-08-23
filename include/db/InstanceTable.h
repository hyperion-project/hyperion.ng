#pragma once

// db
#include <db/DBManager.h>
#include <db/SettingsTable.h>

// qt
#include <QDateTime>

///
/// @brief Hyperion instance manager specific database interface. prepares also the Hyperion database for all follow up usage (Init QtSqlConnection) along with db name
///
class InstanceTable : public DBManager
{

public:
	InstanceTable(const QString& rootPath, QObject* parent = nullptr, bool readonlyMode = false)
		: DBManager(parent)
	{

		setReadonlyMode(readonlyMode);
		// Init Hyperion database usage
		setRootPath(rootPath);
		setDatabaseName("hyperion");

		// Init instance table
		setTable("instances");
		createTable(QStringList()<<"instance INTEGER"<<"friendly_name TEXT"<<"enabled INTEGER DEFAULT 0"<<"last_use TEXT");

		// start/create the first Hyperion instance index 0
		createInstance();
	};

	///
	/// @brief Create a new Hyperion instance entry, the name needs to be unique
	/// @param       name  The name of the instance
	/// @param[out]  inst  The id that has been assigned
	/// @return True on success else false
	///
	inline bool createInstance(const QString& name, quint8& inst)
	{
		VectorPair fcond;
		fcond.append(CPair("friendly_name",name));

		// check duplicate
		if(!recordExists(fcond))
		{
			inst = 0;
			VectorPair cond;
			cond.append(CPair("instance",inst));

			// increment to next avail index
			while(recordExists(cond))
			{
				inst++;
				cond.removeFirst();
				cond.append(CPair("instance",inst));
			}
			// create
			QVariantMap data;
			data["friendly_name"] = name;
			data["instance"] = inst;
			VectorPair lcond;
			return createRecord(lcond, data);
		}
		return false;
	}

	///
	/// @brief Delete a Hyperion instance
	/// @param  inst  The id that has been assigned
	/// @return True on success else false
	///
	inline bool deleteInstance(quint8 inst)
	{
		VectorPair cond;
		cond.append(CPair("instance",inst));
		if(deleteRecord(cond))
		{
			// delete settings entries
			SettingsTable settingsTable(inst);
			settingsTable.deleteInstance();
			return true;
		}
		return false;
	}

	///
	/// @brief Assign a new name for the given instance
	/// @param  inst  The instance index
	/// @param  name  The new name of the instance
	/// @return True on success else false (instance not found)
	///
	inline bool saveName(quint8 inst, const QString& name)
	{
		VectorPair fcond;
		fcond.append(CPair("friendly_name",name));

		// check duplicate
		if(!recordExists(fcond))
		{
			if(instanceExist(inst))
			{
				VectorPair cond;
				cond.append(CPair("instance",inst));
				QVariantMap data;
				data["friendly_name"] = name;

				return updateRecord(cond, data);
			}
		}
		return false;
	}



	///
	/// @brief Get all instances with all columns
	/// @param justEnabled  return just enabled instances if true
	/// @return The found instances
	///
	inline QVector<QVariantMap> getAllInstances(bool justEnabled = false)
	{
		QVector<QVariantMap> results;
		getRecords(results, QStringList(), QStringList() << "instance ASC");
		if(justEnabled)
		{
			for (auto it = results.begin(); it != results.end();)
			{
				if( ! (*it)["enabled"].toBool())
				{
					it = results.erase(it);
					continue;
				}
				++it;
			}
		}
		return results;
	}

	///
	/// @brief      Test if  instance record exists
	/// @param[in]  user   The user id
	/// @return     true on success else false
	///
	inline bool instanceExist(quint8 inst)
	{
		VectorPair cond;
		cond.append(CPair("instance",inst));
		return recordExists(cond);
	}

	///
	/// @brief Get instance name by instance index
	/// @param index  The index to search for
	/// @return The name of this index, may return NOT FOUND if not found
	///
	inline const QString getNamebyIndex(quint8 index)
	{
		QVariantMap results;
		VectorPair cond;
		cond.append(CPair("instance", index));
		getRecord(cond, results, QStringList("friendly_name"));

		QString name = results["friendly_name"].toString();
		return name.isEmpty() ? "NOT FOUND" : name;
	}

	///
	/// @brief Update 'last_use' timestamp
	/// @param inst  The instance to update
	///
	inline void setLastUse(quint8 inst)
	{
		VectorPair cond;
		cond.append(CPair("instance", inst));
		QVariantMap map;
		map["last_use"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
		updateRecord(cond, map);
	}

	///
	/// @brief Update 'enabled' column by instance index
	/// @param inst     The instance to update
	/// @param newState True when enabled else false
	///
	inline void setEnable(quint8 inst, bool newState)
	{
		VectorPair cond;
		cond.append(CPair("instance", inst));
		QVariantMap map;
		map["enabled"] = newState;
		updateRecord(cond, map);
	}

	///
	/// @brief Get state of 'enabled' column by instance index
	/// @param inst  The instance to get
	/// @return True when enabled else false
	///
	inline bool isEnabled(quint8 inst)
	{
		VectorPair cond;
		cond.append(CPair("instance", inst));
		QVariantMap results;
		getRecord(cond, results);

		return results["enabled"].toBool();
	}

private:
	///
	/// @brief Create first Hyperion instance entry, if index 0 is not found.
	///
	inline void createInstance()
	{
		if(instanceExist(0))
			setEnable(0, true);
		else
		{
			QVariantMap data;
			data["friendly_name"] = "First LED Hardware instance";
			VectorPair cond;
			cond.append(CPair("instance", 0));
			if(createRecord(cond, data))
				setEnable(0, true);
			else
				throw std::runtime_error("Failed to create Hyperion root instance in db! This should never be the case...");
		}
	}
};
