#pragma once

// hyperion
#include <db/DBManager.h>
#include <QCryptographicHash>

// qt
#include <QDateTime>
#include <QUuid>

///
/// @brief Authentication table interface
///
class AuthTable : public DBManager
{

public:
	/// construct wrapper with auth table
	AuthTable(const QString& rootPath = "", QObject* parent = nullptr, bool readonlyMode = false)
		: DBManager(parent)
	{
		setReadonlyMode(readonlyMode);
		if(!rootPath.isEmpty()){
			// Init Hyperion database usage
			setRootPath(rootPath);
			setDatabaseName("hyperion");
		}
		// init Auth table
		setTable("auth");
		// create table columns
		createTable(QStringList()<<"user TEXT"<<"password BLOB"<<"token BLOB"<<"salt BLOB"<<"comment TEXT"<<"id TEXT"<<"created_at TEXT"<<"last_use TEXT");
	};

	///
	/// @brief      Create a user record, if called on a existing user the auth is recreated
	/// @param[in]  user           The username
	/// @param[in]  pw             The password
	/// @return     true on success else false
	///
	inline bool createUser(const QString& user, const QString& pw)
	{
		// new salt
		QByteArray salt = QCryptographicHash::hash(QUuid::createUuid().toByteArray(), QCryptographicHash::Sha512).toHex();
		QVariantMap map;
		map["user"] = user;
		map["salt"] = salt;
		map["password"] = hashPasswordWithSalt(pw,salt);
		map["created_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

		VectorPair cond;
		cond.append(CPair("user",user));
		return createRecord(cond, map);
	}

	///
	/// @brief      Test if user record exists
	/// @param[in]  user   The user id
	/// @return     true on success else false
	///
	inline bool userExist(const QString& user)
	{
		VectorPair cond;
		cond.append(CPair("user",user));
		return recordExists(cond);
	}

	///
	/// @brief Test if a user is authorized for access with given pw.
	/// @param user   The user name
	/// @param pw     The password
	/// @return       True on success else false
	///
	inline bool isUserAuthorized(const QString& user, const QString& pw)
	{
		if(userExist(user) && (calcPasswordHashOfUser(user, pw) == getPasswordHashOfUser(user)))
		{
			updateUserUsed(user);
			return true;
		}
		return false;
	}

	///
	/// @brief Test if a user token is authorized for access.
	/// @param usr   The user name
	/// @param token The token
	/// @return       True on success else false
	///
	inline bool isUserTokenAuthorized(const QString& usr, const QString& token)
	{
		if(getUserToken(usr) == token.toUtf8())
		{
			updateUserUsed(usr);
			return true;
		}
		return false;
	}

	///
	/// @brief Update token of a user. It's an alternate login path which is replaced on startup. This token is NOT hashed(!)
	/// @param user   The user name
	/// @return       True on success else false
	///
	inline bool setUserToken(const QString& user)
	{
		QVariantMap map;
		map["token"] = QCryptographicHash::hash(QUuid::createUuid().toByteArray(), QCryptographicHash::Sha512).toHex();

		VectorPair cond;
		cond.append(CPair("user", user));
		return updateRecord(cond, map);
	}

	///
	/// @brief Get token of a user. This token is NOT hashed(!)
	/// @param user   The user name
	/// @return       The token
	///
	inline const QByteArray getUserToken(const QString& user)
	{
		QVariantMap results;
		VectorPair cond;
		cond.append(CPair("user", user));
		getRecord(cond, results, QStringList()<<"token");

		return results["token"].toByteArray();
	}

	///
	/// @brief update password of given user. The user should be tested (isUserAuthorized) to verify this change
	/// @param user   The user name
	/// @param newPw  The new password to set
	/// @return       True on success else false
	///
	inline bool updateUserPassword(const QString& user, const QString& newPw)
	{
		QVariantMap map;
		map["password"] = calcPasswordHashOfUser(user, newPw);

		VectorPair cond;
		cond.append(CPair("user", user));
		return updateRecord(cond, map);
	}

	///
	/// @brief Reset password of Hyperion user !DANGER! Used in Hyperion main.cpp
	/// @return       True on success else false
	///
	inline bool resetHyperionUser()
	{
		QVariantMap map;
		map["password"] = calcPasswordHashOfUser("Hyperion", "hyperion");

		VectorPair cond;
		cond.append(CPair("user", "Hyperion"));
		return updateRecord(cond, map);
	}

	///
	/// @brief Update 'last_use' column entry for the corresponding user
	/// @param[in]  user   The user to search for
	///
	inline void updateUserUsed(const QString& user)
	{
		QVariantMap map;
		map["last_use"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

		VectorPair cond;
		cond.append(CPair("user", user));
		updateRecord(cond, map);
	}

	///
	/// @brief      Test if token record exists, updates last_use on success
	/// @param[in]  token       The token id
	/// @return     true on success else false
	///
	inline bool tokenExist(const QString& token)
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

	///
	/// @brief      Create a new token record with comment
	/// @param[in]  token   The token id as plaintext
	/// @param[in]  comment The comment for the token (eg a human readable identifier)
	/// @param[in]  id      The id for the token
	/// @return     true on success else false
	///
	inline bool createToken(const QString& token, const QString& comment, const QString& id)
	{
		QVariantMap map;
		map["comment"] = comment;
		map["id"] = idExist(id) ? QUuid::createUuid().toString().remove("{").remove("}").left(5) : id;
		map["created_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

		VectorPair cond;
		cond.append(CPair("token", hashToken(token)));
		return createRecord(cond, map);
	}

	///
	/// @brief      Delete token record by id
	/// @param[in]  id    The token id
	/// @return     true on success else false
	///
	inline bool deleteToken(const QString& id)
	{
		VectorPair cond;
		cond.append(CPair("id", id));
		return deleteRecord(cond);
	}

	///
	/// @brief      Rename token record by id
	/// @param[in]  id    The token id
	/// @param[in]  comment The new comment
	/// @return     true on success else false
	///
	inline bool renameToken(const QString &id, const QString &comment)
	{
		QVariantMap map;
		map["comment"] = comment;

		VectorPair cond;
		cond.append(CPair("id", id));
		return updateRecord(cond, map);
	}

	///
	/// @brief Get all 'comment', 'last_use' and 'id' column entries
	/// @return            A vector of all lists
	///
	inline const QVector<QVariantMap> getTokenList()
	{
		QVector<QVariantMap> results;
		getRecords(results, QStringList() << "comment" << "id" << "last_use");

		return results;
	}

	///
	/// @brief      Test if id exists
	/// @param[in]  id      The id
	/// @return     true on success else false
	///
	inline bool idExist(const QString& id)
	{

		VectorPair cond;
		cond.append(CPair("id", id));
		return recordExists(cond);
	}

	///
	/// @brief Get the passwort hash of a user from db
	/// @param   user  The user name
	/// @return         password as hash
	///
	inline const QByteArray getPasswordHashOfUser(const QString& user)
	{
		QVariantMap results;
		VectorPair cond;
		cond.append(CPair("user", user));
		getRecord(cond, results, QStringList()<<"password");

		return results["password"].toByteArray();
	}

	///
	/// @brief Calc the password hash of a user based on user name and password
	/// @param   user  The user name
	/// @param   pw    The password
	/// @return        The calced password hash
	///
	inline const QByteArray calcPasswordHashOfUser(const QString& user, const QString& pw)
	{
		// get salt
		QVariantMap results;
		VectorPair cond;
		cond.append(CPair("user", user));
		getRecord(cond, results, QStringList()<<"salt");

		// calc
		return hashPasswordWithSalt(pw,results["salt"].toByteArray());
	}

	///
	/// @brief Create a password hash of plaintex password + salt
	/// @param  pw    The plaintext password
	/// @param  salt  The salt
	/// @return       The password hash with salt
	///
	inline const QByteArray hashPasswordWithSalt(const QString& pw, const QByteArray& salt)
	{
		return QCryptographicHash::hash(pw.toUtf8().append(salt), QCryptographicHash::Sha512).toHex();
	}

	///
	/// @brief Create a token hash
	/// @param  token The plaintext token
	/// @return The token hash
	///
	inline const QByteArray hashToken(const QString& token)
	{
		return QCryptographicHash::hash(token.toUtf8(), QCryptographicHash::Sha512).toHex();
	}
};
