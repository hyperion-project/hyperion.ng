
// hyperion
#include <db/AuthTable.h>
#include <QCryptographicHash>

// qt
#include <QDateTime>
#include <QUuid>

/// construct wrapper with auth table
AuthTable::AuthTable(QObject* parent)
	: DBManager("auth", parent)
{
	TRACK_SCOPE();	
	_log = Logger::getInstance("DB-AUTH");
	
	// init Auth table and create table columns
	createTable(QStringList()<<"user TEXT"<<"password BLOB"<<"token BLOB"<<"salt BLOB"<<"comment TEXT"<<"id TEXT"<<"created_at TEXT"<<"last_use TEXT");
};

bool AuthTable::createUser(const QString& user, const QString& password)
{
	// new salt
	QByteArray salt = QCryptographicHash::hash(QUuid::createUuid().toByteArray(), QCryptographicHash::Sha512).toHex();
	QVariantMap map;
	map["user"] = user;
	map["salt"] = salt;
	map["password"] = hashPasswordWithSalt(password,salt);
	map["created_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

	return createRecord({{"user",user}}, map);
}

bool AuthTable::userExist(const QString& user)
{
	return recordExists({{"user",user}});
}

bool AuthTable::isUserAuthorized(const QString& user, const QString& password)
{
	if(userExist(user) && (calcPasswordHashOfUser(user,  password) == getPasswordHashOfUser(user)))
	{
		updateUserUsed(user);
		return true;
	}
	return false;
}

bool AuthTable::isUserTokenAuthorized(const QString& usr, const QString& token)
{
	if(getUserToken(usr) == token.toUtf8())
	{
		updateUserUsed(usr);
		return true;
	}
	return false;
}

bool AuthTable::setUserToken(const QString& user)
{
	QVariantMap map;
	map["token"] = QCryptographicHash::hash(QUuid::createUuid().toByteArray(), QCryptographicHash::Sha512).toHex();

	return updateRecord({{"user",user}}, map);
}

const QByteArray AuthTable::getUserToken(const QString& user)
{
	QVariantMap results;
	getRecord({{"user",user}}, results, QStringList()<<"token");

	return results["token"].toByteArray();
}

bool AuthTable::updateUserPassword(const QString& user, const QString& newPassword)
{
	QVariantMap map;
	map["password"] = calcPasswordHashOfUser(user, newPassword);

	return updateRecord({{"user",user}}, map);
}

bool AuthTable::resetHyperionUser()
{
	QVariantMap map;
	map["password"] = calcPasswordHashOfUser(hyperion::DEFAULT_USER, hyperion::DEFAULT_PASSWORD);

	return updateRecord({{"user", hyperion::DEFAULT_USER}}, map);
}

void AuthTable::updateUserUsed(const QString& user)
{
	QVariantMap map;
	map["last_use"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

	updateRecord({{"user",user}}, map);
}

bool AuthTable::tokenExist(const QString& token)
{
	QVariantMap map;
	map["last_use"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

	VectorPair cond;
	cond.append(CPair("token", hashToken(token)));
	if(recordExists(cond))
	{
		// update it
		createRecord(cond,map);
		return true;
	}
	return false;
}

bool AuthTable::createToken(const QString& token, const QString& comment, const QString& identifier)
{
	QVariantMap map;
	map["comment"] = comment;
	map["id"] = identifierExist(identifier) ? QUuid::createUuid().toString().remove("{").remove("}").left(5) : identifier;
	map["created_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

	return createRecord({{"token", hashToken(token)}}, map);
}

bool AuthTable::deleteToken(const QString& identifier)
{
	return deleteRecord({{"id", identifier}});
}

bool AuthTable::renameToken(const QString &identifier, const QString &comment)
{
	QVariantMap map;
	map["comment"] = comment;

	return updateRecord({{"id", identifier}}, map);
}

const QVector<QVariantMap> AuthTable::getTokenList()
{
	QVector<QVariantMap> results;
	getRecords(results, QStringList() << "comment" << "id" << "last_use");

	return results;
}

bool AuthTable::identifierExist(const QString& identifier)
{
	return recordExists({{"id", identifier}});
}

const QByteArray AuthTable::getPasswordHashOfUser(const QString& user)
{
	QVariantMap results;
	getRecord({{"user",user}}, results, QStringList()<<"password");

	return results["password"].toByteArray();
}

const QByteArray AuthTable::calcPasswordHashOfUser(const QString& user, const QString&  password)
{
	// get salt
	QVariantMap results;
	getRecord({{"user",user}}, results, QStringList()<<"salt");

	// calc
	return hashPasswordWithSalt(password,results["salt"].toByteArray());
}

const QByteArray AuthTable::hashPasswordWithSalt(const QString&  password, const QByteArray& salt)
{
	return QCryptographicHash::hash(password.toUtf8().append(salt), QCryptographicHash::Sha512).toHex();
}

const QByteArray AuthTable::hashToken(const QString& token)
{
	return QCryptographicHash::hash(token.toUtf8(), QCryptographicHash::Sha512).toHex();
}
