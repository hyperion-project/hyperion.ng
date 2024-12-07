#ifndef METATABLE_H
#define METATABLE_H

// hyperion
#include <db/DBManager.h>

///
/// @brief meta table specific database interface
///
class MetaTable : public DBManager
{

public:
	/// construct wrapper with plugins table and columns
	explicit MetaTable(QObject* parent = nullptr);

	///
	/// @brief Get the uuid, if the uuid is not set it will be created
	/// @return The uuid
	///
	QString getUUID() const;
};

#endif // METATABLE_H
