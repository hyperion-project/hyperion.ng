#include <db/DBManager.h>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QThreadStorage>
#include <QUuid>
#include <QDir>

#ifdef _WIN32
	#include <stdexcept>
#endif

// not in header because of linking
static QString _rootPath;
static QThreadStorage<QSqlDatabase> _databasePool;

DBManager::DBManager(QObject* parent)
	: QObject(parent)
	, _log(Logger::getInstance("DB"))
	, _readonlyMode (false)
{
}

DBManager::~DBManager()
{
}

void DBManager::setRootPath(const QString& rootPath)
{
	_rootPath = rootPath;
	// create directory
	QDir().mkpath(_rootPath+"/db");
}

void DBManager::setTable(const QString& table)
{
	_table = table;
}

QSqlDatabase DBManager::getDB() const
{
	if(_databasePool.hasLocalData())
		return _databasePool.localData();
	else
	{
		auto db = QSqlDatabase::addDatabase("QSQLITE", QUuid::createUuid().toString());
		_databasePool.setLocalData(db);
		db.setDatabaseName(_rootPath+"/db/"+_dbn+".db");
		if(!db.open())
		{
			Error(_log, "%s", QSTRING_CSTR(db.lastError().text()));
			throw std::runtime_error("Failed to open database connection!");
		}
		return db;
	}
}

bool DBManager::createRecord(const VectorPair& conditions, const QVariantMap& columns) const
{
	if ( _readonlyMode )
	{
		return false;
	}

	if(recordExists(conditions))
	{
		// if there is no column data, return
		if(columns.isEmpty())
			return true;

		if(!updateRecord(conditions, columns))
			return false;

		return true;
	}

	QSqlDatabase idb = getDB();
	QSqlQuery query(idb);
	query.setForwardOnly(true);

	QVariantList cValues;
	QStringList prep;
	QStringList placeh;
	// prep merge columns & condition
	QVariantMap::const_iterator i = columns.constBegin();
	while (i != columns.constEnd()) {
		prep.append(i.key());
		cValues += i.value();
		placeh.append("?");

		++i;
	}
	for(const auto& pair : conditions)
	{
		// remove the condition statements
		QString tmp = pair.first;
		prep << tmp.remove("AND");
		cValues << pair.second;
		placeh.append("?");
	}
	query.prepare(QString("INSERT INTO %1 ( %2 ) VALUES ( %3 )").arg(_table,prep.join(", ")).arg(placeh.join(", ")));
	// add column & condition values
	doAddBindValue(query, cValues);
	if(!query.exec())
	{
		Error(_log, "Failed to create record: '%s' in table: '%s' Error: %s", QSTRING_CSTR(prep.join(", ")), QSTRING_CSTR(_table), QSTRING_CSTR(idb.lastError().text()));
		return false;
	}
	return true;
}

bool DBManager::recordExists(const VectorPair& conditions) const
{
	if(conditions.isEmpty())
		return false;

	QSqlDatabase idb = getDB();
	QSqlQuery query(idb);
	query.setForwardOnly(true);

	QStringList prepCond;
	QVariantList bindVal;
	prepCond << "WHERE";

	for(const auto& pair : conditions)
	{
		prepCond << pair.first+"=?";
		bindVal << pair.second;
	}
	query.prepare(QString("SELECT * FROM %1 %2").arg(_table,prepCond.join(" ")));
	doAddBindValue(query, bindVal);
	if(!query.exec())
	{
		Error(_log, "Failed recordExists(): '%s' in table: '%s' Error: %s", QSTRING_CSTR(prepCond.join(" ")), QSTRING_CSTR(_table), QSTRING_CSTR(idb.lastError().text()));
		return false;
	}

	int entry = 0;
	while (query.next()) {
		entry++;
	}

	if(entry)
		return true;

	return false;
}

bool DBManager::updateRecord(const VectorPair& conditions, const QVariantMap& columns) const
{
	if ( _readonlyMode )
	{
		return false;
	}

	QSqlDatabase idb = getDB();
	QSqlQuery query(idb);
	query.setForwardOnly(true);

	QVariantList values;
	QStringList prep;

	// prepare columns valus
	QVariantMap::const_iterator i = columns.constBegin();
	while (i != columns.constEnd()) {
		prep += i.key()+"=?";
		values += i.value();

		++i;
	}

	// prepare condition values
	QStringList prepCond;
	QVariantList prepBindVal;
	if(!conditions.isEmpty())
		prepCond << "WHERE";

	for(const auto& pair : conditions)
	{
		prepCond << pair.first+"=?";
		prepBindVal << pair.second;
	}

	query.prepare(QString("UPDATE %1 SET %2 %3").arg(_table,prep.join(", ")).arg(prepCond.join(" ")));
	// add column values
	doAddBindValue(query, values);
	// add condition values
	doAddBindValue(query, prepBindVal);
	if(!query.exec())
	{
		Error(_log, "Failed to update record: '%s' in table: '%s' Error: %s", QSTRING_CSTR(prepCond.join(" ")), QSTRING_CSTR(_table), QSTRING_CSTR(idb.lastError().text()));
		return false;
	}
	return true;
}

bool DBManager::getRecord(const VectorPair& conditions, QVariantMap& results, const QStringList& tColumns, const QStringList& tOrder) const
{
	QSqlDatabase idb = getDB();
	QSqlQuery query(idb);
	query.setForwardOnly(true);

	QString sColumns("*");
	if(!tColumns.isEmpty())
		sColumns = tColumns.join(", ");

	QString sOrder("");
	if(!tOrder.isEmpty())
	{
		sOrder = " ORDER BY ";
		sOrder.append(tOrder.join(", "));
	}
	// prep conditions
	QStringList prepCond;
	QVariantList bindVal;
	if(!conditions.isEmpty())
		prepCond << " WHERE";

	for(const auto& pair : conditions)
	{
		prepCond << pair.first+"=?";
		bindVal << pair.second;
	}
	query.prepare(QString("SELECT %1 FROM %2%3%4").arg(sColumns,_table).arg(prepCond.join(" ")).arg(sOrder));
	doAddBindValue(query, bindVal);

	if(!query.exec())
	{
		Error(_log, "Failed to get record: '%s' in table: '%s' Error: %s", QSTRING_CSTR(prepCond.join(" ")), QSTRING_CSTR(_table), QSTRING_CSTR(idb.lastError().text()));
		return false;
	}

	// go to first row
	query.next();

	QSqlRecord rec = query.record();
	for(int i = 0; i<rec.count(); i++)
	{
		results[rec.fieldName(i)] = rec.value(i);
	}

	return true;
}

bool DBManager::getRecords(QVector<QVariantMap>& results, const QStringList& tColumns, const QStringList& tOrder) const
{
	QSqlDatabase idb = getDB();
	QSqlQuery query(idb);
	query.setForwardOnly(true);

	QString sColumns("*");
	if(!tColumns.isEmpty())
		sColumns = tColumns.join(", ");

	QString sOrder("");
	if(!tOrder.isEmpty())
	{
		sOrder = " ORDER BY ";
		sOrder.append(tOrder.join(", "));
	}

	query.prepare(QString("SELECT %1 FROM %2%3").arg(sColumns,_table,sOrder));

	if(!query.exec())
	{
		Error(_log, "Failed to get records: '%s' in table: '%s' Error: %s", QSTRING_CSTR(sColumns), QSTRING_CSTR(_table), QSTRING_CSTR(idb.lastError().text()));
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
	if ( _readonlyMode )
	{
		return false;
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
			prepCond << pair.first+"=?";
			bindValues << pair.second;
		}

		query.prepare(QString("DELETE FROM %1 %2").arg(_table,prepCond.join(" ")));
		doAddBindValue(query, bindValues);
		if(!query.exec())
		{
			Error(_log, "Failed to delete record: '%s' in table: '%s' Error: %s", QSTRING_CSTR(prepCond.join(" ")), QSTRING_CSTR(_table), QSTRING_CSTR(idb.lastError().text()));
			return false;
		}
		return true;
	}
	return false;
}

bool DBManager::createTable(QStringList& columns) const
{
	if ( _readonlyMode )
	{
		return false;
	}

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
		if(!query.exec(QString("CREATE TABLE %1 ( %2 )").arg(_table,tcolumn)))
		{
			Error(_log, "Failed to create table: '%s' Error: %s", QSTRING_CSTR(_table), QSTRING_CSTR(idb.lastError().text()));
			return false;
		}
	}
	// create columns if required
	QSqlRecord rec = idb.record(_table);
	int err = 0;
	for(const auto& column : columns)
	{
		QStringList id = column.split(' ');
		if(rec.indexOf(id.at(0)) == -1)
		{
			if(!createColumn(column))
			{
				err++;
			}
		}
	}
	if(err)
		return false;

	return true;
}

bool DBManager::createColumn(const QString& column) const
{
	if ( _readonlyMode )
	{
		return false;
	}

	QSqlDatabase idb = getDB();
	QSqlQuery query(idb);
	if(!query.exec(QString("ALTER TABLE %1 ADD COLUMN %2").arg(_table,column)))
	{
		Error(_log, "Failed to create column: '%s' in table: '%s' Error: %s", QSTRING_CSTR(column), QSTRING_CSTR(_table), QSTRING_CSTR(idb.lastError().text()));
		return false;
	}
	return true;
}

bool DBManager::tableExists(const QString& table) const
{
	QSqlDatabase idb = getDB();
	QStringList tables = idb.tables();
	if(tables.contains(table))
		return true;
	return false;
}

bool DBManager::deleteTable(const QString& table) const
{
	if ( _readonlyMode )
	{
		return false;
	}

	if(tableExists(table))
	{
		QSqlDatabase idb = getDB();
		QSqlQuery query(idb);
		if(!query.exec(QString("DROP TABLE %1").arg(table)))
		{
			Error(_log, "Failed to delete table: '%s' Error: %s", QSTRING_CSTR(table), QSTRING_CSTR(idb.lastError().text()));
			return false;
		}
	}
	return true;
}

void DBManager::doAddBindValue(QSqlQuery& query, const QVariantList& variants) const
{
	for(const auto& variant : variants)
	{
		auto t = variant.userType();
		switch(t)
		{
			case QVariant::UInt:
			case QVariant::Int:
			case QVariant::Bool:
				query.addBindValue(variant.toInt());
				break;
			case QVariant::Double:
				query.addBindValue(variant.toFloat());
				break;
			case QVariant::ByteArray:
				query.addBindValue(variant.toByteArray());
				break;
			default:
				query.addBindValue(variant.toString());
				break;
		}
	}
}
