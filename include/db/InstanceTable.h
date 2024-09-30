#ifndef INSTANCETABLE_H
#define INSTANCETABLE_H

#include <db/DBManager.h>

///
/// @brief Hyperion instance manager specific database interface. prepares also the Hyperion database for all follow up usage (Init QtSqlConnection) along with db name
///
class InstanceTable : public DBManager
{

public:
	explicit InstanceTable(QObject* parent = nullptr);

	///
	/// @brief Create a new Hyperion instance entry, the name needs to be unique
	/// @param       name  The name of the instance
	/// @param[out]  inst  The id that has been assigned
	/// @return True on success else false
	///
	bool createInstance(const QString& name, quint8& inst);

	///
	/// @brief Create first Hyperion instance entry, if index 0 is not found.
	///
	void createDefaultInstance();

	///
	/// @brief Delete a Hyperion instance
	/// @param  inst  The id that has been assigned
	/// @return True on success else false
	///
	bool deleteInstance(quint8 inst);

	///
	/// @brief Assign a new name for the given instance
	/// @param  inst  The instance index
	/// @param  name  The new name of the instance
	/// @return True on success else false (instance not found)
	///
	bool saveName(quint8 inst, const QString& name);

	///
	/// @brief Get all instances with all columns
	/// @param onlyEnabled  return only enabled instances if true
	/// @return The found instances
	///
	QVector<QVariantMap> getAllInstances(bool onlyEnabled = false);

	///
	/// @brief Get all instance IDs
	/// @param onlyEnabled  return only enabled instance IDs if true
	/// @return The found instances
	///
	QList<quint8> getAllInstanceIDs (bool onlyEnabled = false);

	///
	/// @brief      Test if  instance record exists
	/// @param[in]  user   The user id
	/// @return     true on success else false
	///
	bool instanceExist(quint8 inst);

	///
	/// @brief Get instance name by instance index
	/// @param index  The index to search for
	/// @return The name of this index, may return NOT FOUND if not found
	///
	QString getNamebyIndex(quint8 index);

	///
	/// @brief Update 'last_use' timestamp
	/// @param inst  The instance to update
	/// @return True on success else false
	///
	bool setLastUse(quint8 inst);

	///
	/// @brief Update 'enabled' column by instance index
	/// @param inst     The instance to update
	/// @param newState True when enabled else false
	/// @return True on success else false
	///
	bool setEnable(quint8 inst, bool newState);

	///
	/// @brief Get state of 'enabled' column by instance index
	/// @param inst  The instance to get
	/// @return True when enabled else false
	///
	bool isEnabled(quint8 inst);

private:
};

#endif // INSTANCETABLE_H
