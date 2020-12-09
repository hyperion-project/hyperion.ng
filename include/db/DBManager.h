#pragma once

#include <utils/Logger.h>
#include <QMap>
#include <QVariant>
#include <QPair>
#include <QVector>

class QSqlDatabase;
class QSqlQuery;

typedef QPair<QString,QVariant> CPair;
typedef QVector<CPair> VectorPair;

///
/// @brief Database interface for SQLite3.
///        Inherit this class to create component specific methods based on this interface
///        Usage: setTable(name) once before you use read/write actions
///        To use another database use setDb(newDB) (defaults to "hyperion")
///
///        Incompatible functions with SQlite3:
///        QSqlQuery::size() returns always -1
///
class DBManager : public QObject
{
	Q_OBJECT

public:
	DBManager(QObject* parent = nullptr);
	~DBManager() override;

	/// set root path
	void setRootPath(const QString& rootPath);
	/// define the database to work with
	void setDatabaseName(const QString& dbn) { _dbn = dbn; };
	/// set a table to work with
	void setTable(const QString& table);

	/// get current database object set with setDB()
	QSqlDatabase getDB() const;

	///
	/// @brief Create a table (if required) with the given columns. Older tables will be updated accordingly with missing columns
	///        Does not delete or migrate old columns
	/// @param[in]  columns  The columns of the table. Requires at least one entry!
	/// @return              True on success else false
	///
	bool createTable(QStringList& columns) const;

	///
	/// @brief Create a column if the column already exists returns false and logs error
	/// @param[in]  column   The column of the table
	/// @return              True on success else false
	///
	bool createColumn(const QString& column) const;

	///
	/// @brief Check if at least one record exists in table with the conditions
	/// @param[in]  conditions The search conditions (WHERE)
	/// @return                True on success else false
	///
	bool recordExists(const VectorPair& conditions) const;

	///
	/// @brief Create a new record in table when the conditions find no existing entry. Add additional key:value pairs in columns
	///        DO NOT repeat column keys between 'conditions' and 'columns' as they will be merged on creation
	/// @param[in]  conditions conditions to search for, as a record may exist and should be updated instead (WHERE)
	/// @param[in]  columns    columns to create or update (optional)
	/// @return                True on success else false
	///
	bool createRecord(const VectorPair& conditions, const QVariantMap& columns = QVariantMap()) const;

	///
	/// @brief Update a record with conditions and additional key:value pairs in columns
	/// @param[in]  conditions conditions which rows should be updated (WHERE)
	/// @param[in]  columns    columns to update
	/// @return                True on success else false
	///
	bool updateRecord(const VectorPair& conditions, const QVariantMap& columns) const;

	///
	/// @brief Get data of a single record, multiple records are not supported
	/// @param[in]  conditions  condition to search for (WHERE)
	/// @param[out] results     results of query
	/// @param[in]  tColumns    target columns to search in (optional) if not provided returns all columns
	/// @param[in]  tOrder      target order columns with order by ASC/DESC (optional)
	/// @return                 True on success else false
	///
	bool getRecord(const VectorPair& conditions, QVariantMap& results, const QStringList& tColumns = QStringList(), const QStringList& tOrder = QStringList()) const;

	///
	/// @brief Get data of multiple records, you need to specify the columns. This search is without conditions. Good to grab all data from db
	/// @param[in]  conditions  condition to search for (WHERE)
	/// @param[out] results     results of query
	/// @param[in]  tColumns    target columns to search in (optional) if not provided returns all columns
	/// @param[in]  tOrder      target order columns with order by ASC/DESC (optional)
	/// @return                 True on success else false
	///
	bool getRecords(QVector<QVariantMap>& results, const QStringList& tColumns = QStringList(), const QStringList& tOrder = QStringList()) const;

	///
	/// @brief Delete a record determined by conditions
	/// @param[in]  conditions conditions of the row to delete it (WHERE)
	/// @return                True on success; on error or not found false
	///
	bool deleteRecord(const VectorPair& conditions) const;

	///
	/// @brief Check if table exists in current database
	/// @param[in]  table   The name of the table
	/// @return             True on success else false
	///
	bool tableExists(const QString& table) const;

	///
	/// @brief Delete a table, fails silent (return true) if table is not found
	/// @param[in]  table   The name of the table
	/// @return             True on success else false
	///
	bool deleteTable(const QString& table) const;

	///
	/// @brief Sets a table in read-only mode.
	/// Updates will not written to the table
	/// @param[in]  readOnly True read-only, false - read/write
	///
	void setReadonlyMode(bool readOnly) { _readonlyMode = readOnly; };

private:

	Logger* _log;
	/// databse connection & file name, defaults to hyperion
	QString _dbn = "hyperion";
	/// table in database
	QString _table;

	bool _readonlyMode;

	/// addBindValue to query given by QVariantList
	void doAddBindValue(QSqlQuery& query, const QVariantList& variants) const;
};
