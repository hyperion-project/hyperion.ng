#include "utils/settings.h"
#include <db/DBManager.h>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>
#include <QDir>
#include <QMetaType>
#include <QJsonObject>

#ifdef _WIN32
	#include <stdexcept>
#endif

#define NO_SQLQUERY_LOGGING

// Constants
namespace {
	const char DATABASE_DIRECTORYNAME[] = "db";
	const char DATABASE_FILENAME[] = "hyperion.db";

} //End of constants


QDir DBManager::_dataDirectory;
QDir DBManager::_databaseDirectory;
QFileInfo DBManager::_databaseFile;
QThreadStorage<QSqlDatabase> DBManager::_databasePool;
bool DBManager::_isReadOnly {false};

DBManager::DBManager(QObject* parent)
	: QObject(parent)
	, _log(Logger::getInstance("DB"))
{
}

void DBManager::initializeDatabase(const QDir& dataDirectory, bool isReadOnly)
{
	_dataDirectory = dataDirectory;
	_databaseDirectory.setPath(_dataDirectory.absoluteFilePath(DATABASE_DIRECTORYNAME));
	QDir().mkpath(_databaseDirectory.absolutePath());
	_databaseFile.setFile(_databaseDirectory,DATABASE_FILENAME);
	_isReadOnly = isReadOnly;
}

void DBManager::setTable(const QString& table)
{
	_table = table;
}

QSqlDatabase DBManager::getDB() const
{
	if(_databasePool.hasLocalData())
	{
		return _databasePool.localData();
	}
		auto database = QSqlDatabase::addDatabase("QSQLITE", QUuid::createUuid().toString());

		if (isReadOnly())
		{
			database.setConnectOptions("QSQLITE_OPEN_READONLY");
		}

#ifdef SQLQUERY_LOGGING
		Debug(Logger::getInstance("DB"), "Database is opened in %s mode", _isReadOnly ? "read-only" : "read/write");
#endif

		_databasePool.setLocalData(database);
		database.setDatabaseName(_databaseFile.absoluteFilePath());
		if(!database.open())
		{
			Error(_log, "%s", QSTRING_CSTR(database.lastError().text()));
			throw std::runtime_error("Failed to open database connection!");
		}

		return database;
}

bool DBManager::createRecord(const VectorPair& conditions, const QVariantMap& columns) const
{
	if(recordExists(conditions))
	{
		// if there is no column data, return
		if(columns.isEmpty())
		{
			return true;
		}

		return updateRecord(conditions, columns);
	}

	QSqlDatabase idb = getDB();
	QSqlQuery query(idb);
	query.setForwardOnly(true);

	QVariantList cValues;
	QStringList prep;
	QStringList placeh;

	// prep merge columns & condition
	QVariantMap::const_iterator columnIter = columns.constBegin();
	while (columnIter != columns.constEnd()) {
		prep.append(columnIter.key());
		cValues += columnIter.value();
		placeh.append("?");

		++columnIter;
	}
	for(const auto& pair : conditions)
	{
		// remove the condition statements
		QString tmp = pair.first;
		prep << tmp.remove("AND");
		cValues << pair.second;
		placeh.append("?");
	}
	query.prepare(QString("INSERT INTO %1 ( %2 ) VALUES ( %3 )").arg(_table,prep.join(", "), placeh.join(", ")));
	// add column & condition values
	addBindValues(query, cValues);

	return executeQuery(query);
}

bool DBManager::recordExists(const VectorPair& conditions) const
{
	if(conditions.isEmpty())
	{
		return false;
	}

	QSqlDatabase idb = getDB();
	QSqlQuery query(idb);
	query.setForwardOnly(true);

	QStringList prepCond;
	QVariantList bindVal;
	prepCond << "WHERE";

	for(const auto& pair : conditions)
	{
		prepCond << pair.first+"= ?";
		bindVal << pair.second;
	}
	query.prepare(QString("SELECT * FROM %1 %2").arg(_table,prepCond.join(" ")));
	addBindValues(query, bindVal);

	if (!executeQuery(query))
	{
		return false;
	}

	int entry = 0;
	while (query.next())
	{
		entry++;
	}

	return entry > 0;
}

bool DBManager::updateRecord(const VectorPair& conditions, const QVariantMap& columns) const
{
	if (isReadOnly())
	{
		return true;
	}

	QSqlDatabase idb = getDB();
	QSqlQuery query(idb);
	query.setForwardOnly(true);

	QVariantList values;
	QStringList prep;

	// prepare columns valus
	QVariantMap::const_iterator columnIter = columns.constBegin();
	while (columnIter != columns.constEnd()) {
		prep += columnIter.key()+"= ?";
		values += columnIter.value();

		++columnIter;
	}

	// prepare condition values
	QStringList prepCond;
	QVariantList prepBindVal;
	if(!conditions.isEmpty()) {
		prepCond << "WHERE";
	}

	for(const auto& pair : conditions)
	{
		prepCond << pair.first+"= ?";
		prepBindVal << pair.second;
	}

	query.prepare(QString("UPDATE %1 SET %2 %3").arg(_table,prep.join(", "), prepCond.join(" ")));
	// add column values
	addBindValues(query, values);
	// add condition values
	addBindValues(query, prepBindVal);

	return executeQuery(query);
}

bool DBManager::getRecord(const VectorPair& conditions, QVariantMap& results, const QStringList& tColumns, const QStringList& tOrder) const
{
	QVector<QVariantMap> resultVector{};
	bool success = getRecords(conditions, resultVector, tColumns, tOrder);
	if (success && !resultVector.isEmpty()) {
		results = resultVector.first();
	}
	return success;
}

bool DBManager::getRecords(QVector<QVariantMap>& results, const QStringList& tColumns, const QStringList& tOrder) const
{
	return getRecords({}, results, tColumns, tOrder);
}

bool DBManager::getRecords(const VectorPair& conditions, QVector<QVariantMap>& results, const QStringList& tColumns, const QStringList& tOrder) const
{
	// prep conditions
	QStringList conditionList;
	QVariantList bindValues;

	for(const auto& pair : conditions)
	{
		conditionList << pair.first;
		if (pair.second.isNull())
		{
			conditionList << "IS NULL";
		}
		else
		{
			conditionList << "= ?";
			bindValues << pair.second;
		}
	}

	return getRecords(conditionList.join((" ")), bindValues, results, tColumns, tOrder);
}

bool DBManager::getRecords(const QString& condition, const QVariantList& bindValues, QVector<QVariantMap>& results, const QStringList& tColumns, const QStringList& tOrder) const
{
	QSqlDatabase idb = getDB();
	QSqlQuery query(idb);
	query.setForwardOnly(true);

	QString sColumns("*");
	if(!tColumns.isEmpty())
	{
		sColumns = tColumns.join(", ");
	}

	QString sOrder("");
	if(!tOrder.isEmpty())
	{
		sOrder = "ORDER BY ";
		sOrder.append(tOrder.join(", "));
	}

	// prep conditions
	QString prepCond;
	if(!condition.isEmpty())
	{
		prepCond = QString("WHERE %1").arg(condition);
	}

	query.prepare(QString("SELECT %1 FROM %2 %3 %4").arg(sColumns,_table, prepCond, sOrder));
	addBindValues(query, bindValues);
	if (!executeQuery(query))
	{
		return false;
	}

	// iterate through all found records
	while(query.next())
	{
		QVariantMap entry;
		QSqlRecord rec = query.record();
		for(int i = 0; i<rec.count(); i++)
		{
			entry[rec.fieldName(i)] = rec.value(i);
		}
		results.append(entry);
	}

	return true;
}


bool DBManager::deleteRecord(const VectorPair& conditions) const
{
	if (_isReadOnly)
	{
		return true;
	}

	if(conditions.isEmpty())
	{
		Error(_log, "Oops, a deleteRecord() call wants to delete the entire table (%s)! Denied it", QSTRING_CSTR(_table));
		return false;
	}

	if(recordExists(conditions))
	{
		QSqlDatabase idb = getDB();
		QSqlQuery query(idb);

		// prep conditions
		QStringList prepCond("WHERE");
		QVariantList bindValues;

		for(const auto& pair : conditions)
		{
			prepCond << pair.first+"= ?";
			bindValues << pair.second;
		}

		query.prepare(QString("DELETE FROM %1 %2").arg(_table,prepCond.join(" ")));
		addBindValues(query, bindValues);

		return executeQuery(query);
	}
	return false;
}

bool DBManager::createTable(QStringList& columns) const
{
	if(columns.isEmpty())
	{
		Error(_log,"Empty tables aren't supported!");
		return false;
	}

	QSqlDatabase idb = getDB();
	// create table if required
	QSqlQuery query(idb);
	if(!tableExists(_table))
	{
		// empty tables aren't supported by sqlite, add one column
		QString tcolumn = columns.takeFirst();
		// default CURRENT_TIMESTAMP is not supported by ALTER TABLE
		query.prepare(QString("CREATE TABLE %1 ( %2 )").arg(_table,tcolumn));
		if (!executeQuery(query))
		{
			return false;
		}
	}
	// create columns if required
	QSqlRecord rec = idb.record(_table);
	int err = 0;
	for(const auto& column : columns)
	{
		QStringList columName = column.split(' ');
		if(rec.indexOf(columName.at(0)) == -1)
		{
			if(!createColumn(column))
			{
				err++;
			}
		}
	}
	return err == 0;
}

bool DBManager::createColumn(const QString& column) const
{
	QSqlDatabase idb = getDB();
	QSqlQuery query(idb);

	query.prepare(QString("ALTER TABLE %1 ADD COLUMN %2").arg(_table,column));
	return executeQuery(query);
}

bool DBManager::tableExists(const QString& table) const
{
	QSqlDatabase idb = getDB();
	QStringList tables = idb.tables();
	return tables.contains(table);
}

bool DBManager::deleteTable(const QString& table) const
{
	if(tableExists(table))
	{
		QSqlDatabase idb = getDB();
		QSqlQuery query(idb);

		query.prepare(QString("DROP TABLE %1").arg(table));
		return executeQuery(query);
	}
	return true;
}

void DBManager::addBindValues(QSqlQuery& query, const QVariantList& bindValues) const
{
	if (!bindValues.isEmpty())
	{
		for(const auto& value : bindValues)
		{
			query.addBindValue(value);
		}
	}
}

QString DBManager::constructExecutedQuery(const QSqlQuery& query) const
{
	QString executedQuery = query.executedQuery();

	// Check if the query uses positional placeholders
	if (executedQuery.contains('?')) {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
		QVariantList boundValues = query.boundValues(); // Get bound values as a list
#else
		QVariantMap boundValues = query.boundValues(); // Get bound values as a list
#endif
		// Iterate through the bound values and replace placeholders
		for (const QVariant &value : boundValues) {
			// Replace the first occurrence of '?' with the actual value
			QString valueStr;
			if (value.canConvert<QString>())
			{
				valueStr = value.toString();
			}
			else
			{
				valueStr = "Unkown";
			}
			executedQuery.replace(executedQuery.indexOf('?'), 1, valueStr);
		}
	}
	return executedQuery;
}

bool DBManager::executeQuery(QSqlQuery& query) const
{
	if( !query.exec())
	{
		QString finalQuery = constructExecutedQuery(query);
		QString errorText = query.lastError().text();

		Debug(_log, "Database Error: '%s', SqlQuery: '%s'", QSTRING_CSTR(errorText), QSTRING_CSTR(finalQuery));
		Error(_log, "Database Error: '%s'", QSTRING_CSTR(errorText));

		return false;
	}

#ifdef SQLQUERY_LOGGING
	QString finalQuery = constructExecutedQuery(query);
	Debug(_log, "SqlQuery executed: '%s'", QSTRING_CSTR(finalQuery));
#endif

	return true;
}
