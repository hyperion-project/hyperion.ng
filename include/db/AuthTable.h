#ifndef AUTHSTABLE_H
#define AUTHSTABLE_H

#include <db/DBManager.h>

namespace hyperion {
const char DEFAULT_USER[] = "Hyperion";
const char DEFAULT_PASSWORD[] = "hyperion";
}

///
/// @brief Authentication table interface
///
class AuthTable : public DBManager
{

public:
	/// construct wrapper with auth table
	explicit AuthTable(QObject* parent = nullptr);

	///
	/// @brief      Create a user record, if called on a existing user the auth is recreated
	/// @param[in]  user           The username
	/// @param[in]  password       The password
	/// @return     true on success else false
	///
	bool createUser(const QString& user, const QString& password);

	///
	/// @brief      Test if user record exists
	/// @param[in]  user   The user id
	/// @return     true on success else false
	///
	bool userExist(const QString& user);

	///
	/// @brief Test if a user is authorized for access with given pw.
	/// @param user     The user name
	/// @param password The password
	/// @return         True on success else false
	///
	bool isUserAuthorized(const QString& user, const QString& password);

	///
	/// @brief Test if a user token is authorized for access.
	/// @param usr   The user name
	/// @param token The token
	/// @return       True on success else false
	///
	bool isUserTokenAuthorized(const QString& usr, const QString& token);

	///
	/// @brief Update token of a user. It's an alternate login path which is replaced on startup. This token is NOT hashed(!)
	/// @param user   The user name
	/// @return       True on success else false
	///
	bool setUserToken(const QString& user);

	///
	/// @brief Get token of a user. This token is NOT hashed(!)
	/// @param user   The user name
	/// @return       The token
	///
	const QByteArray getUserToken(const QString& user);

	///
	/// @brief update password of given user. The user should be tested (isUserAuthorized) to verify this change
	/// @param user        The user name
	/// @param newassword  The new password to set
	/// @return             True on success else false
	///
	bool updateUserPassword(const QString& user, const QString& newPassword);

	///
	/// @brief Reset password of Hyperion user !DANGER! Used in Hyperion main.cpp
	/// @return       True on success else false
	///
	bool resetHyperionUser();

	///
	/// @brief Update 'last_use' column entry for the corresponding user
	/// @param[in]  user   The user to search for
	///
	void updateUserUsed(const QString& user);

	///
	/// @brief      Test if token record exists, updates last_use on success
	/// @param[in]  token       The token id
	/// @return     true on success else false
	///
	bool tokenExist(const QString& token);

	///
	/// @brief      Create a new token record with comment
	/// @param[in]  token   The token id as plaintext
	/// @param[in]  comment The comment for the token (eg a human readable identifier)
	/// @param[in]  identifier      The identifier for the token
	/// @return     true on success else false
	///
	bool createToken(const QString& token, const QString& comment, const QString& identifier);

	///
	/// @brief      Delete token record by identifier
	/// @param[in]  identifier    The token identifier
	/// @return     true on success else false
	///
	bool deleteToken(const QString& identifier);

	///
	/// @brief      Rename token record by identifier
	/// @param[in]  identifier    The token identifier
	/// @param[in]  comment The new comment
	/// @return     true on success else false
	///
	bool renameToken(const QString &identifier, const QString &comment);

	///
	/// @brief Get all 'comment', 'last_use' and 'id' column entries
	/// @return            A vector of all lists
	///
	const QVector<QVariantMap> getTokenList();

	///
	/// @brief      Test if identifier exists
	/// @param[in]  identifier      The identifier
	/// @return     true on success else false
	///
	bool identifierExist(const QString& identifier);

	///
	/// @brief Get the passwort hash of a user from db
	/// @param   user  The user name
	/// @return         password as hash
	///
	const QByteArray getPasswordHashOfUser(const QString& user);

	///
	/// @brief Calc the password hash of a user based on user name and password
	/// @param   user  The user name
	/// @param   pw    The password
	/// @return        The calced password hash
	///
	const QByteArray calcPasswordHashOfUser(const QString& user, const QString& password);

	///
	/// @brief Create a password hash of plaintex password + salt
	/// @param  password   The plaintext password
	/// @param  salt  The salt
	/// @return       The password hash with salt
	///
	const QByteArray hashPasswordWithSalt(const QString& password, const QByteArray& salt);

	///
	/// @brief Create a token hash
	/// @param  token The plaintext token
	/// @return The token hash
	///
	const QByteArray hashToken(const QString& token);
};

#endif // AUTHSTABLE_H
