#pragma once

#include <utils/Logger.h>
#include <utils/settings.h>

//qt
#include <QMap>
#include <QVector>

class AuthTable;
class MetaTable;
class QTimer;

///
/// @brief Manage the authorization of user and tokens. This class is created once as part of the HyperionDaemon
///  To work with the global instance use AuthManager::getInstance()
///
class AuthManager : public QObject
{
	Q_OBJECT
private:
	friend class HyperionDaemon;
	/// constructor is private, can be called from HyperionDaemon
	AuthManager(QObject *parent = nullptr, bool readonlyMode = false);

public:
	struct AuthDefinition
	{
		QString id;
		QString comment;
		QObject *caller;
		int      tan;
		uint64_t timeoutTime;
		QString token;
		QString lastUse;
	};

	///
	/// @brief Get the unique id (imported from removed class 'Stats')
	/// @return The unique id
	///
	QString getID() const { return _uuid; }

	///
	/// @brief Check authorization is required according to the user setting
	/// @return       True if authorization required else false
	///
	bool isAuthRequired() const { return _authRequired; }

	///
	/// @brief Check if authorization is required for local network connections
	/// @return       True if authorization required else false
	///
	bool isLocalAuthRequired() const { return _localAuthRequired; }

	///
	/// @brief Check if authorization is required for local network connections for admin access
	/// @return       True if authorization required else false
	///
	bool isLocalAdminAuthRequired() const { return _localAdminAuthRequired; }

	///
	/// @brief Reset Hyperion user
	/// @return        True on success else false
	///
	bool resetHyperionUser();

	///
	/// @brief Check if user auth is temporary blocked due to failed attempts
	/// @return True on blocked and no further Auth requests will be accepted
	///
	bool isUserAuthBlocked() const { return (_userAuthAttempts.length() >= 10); }

	///
	/// @brief Check if token auth is temporary blocked due to failed attempts
	/// @return True on blocked and no further Auth requests will be accepted
	///
	bool isTokenAuthBlocked() const { return (_tokenAuthAttempts.length() >= 25); }

	/// Pointer of this instance
	static AuthManager *manager;
	/// Get Pointer of this instance
	static AuthManager *getInstance() { return manager; }

public slots:

	///
	/// @brief Check if user is authorized
	/// @param  user  The username
	/// @param  pw    The password
	/// @return        True if authorized else false
	///
	bool isUserAuthorized(const QString &user, const QString &pw);

	///
	/// @brief Check if token is authorized
	/// @param  token  The token
	/// @return        True if authorized else false
	///
	bool isTokenAuthorized(const QString &token);

	///
	/// @brief Check if token is authorized
	/// @param  usr    The username
	/// @param  token  The token
	/// @return        True if authorized else false
	///
	bool isUserTokenAuthorized(const QString &usr, const QString &token);

	///
	/// @brief Create a new token and skip the usual chain
	/// @param  comment The comment that should be used for
	/// @return         The new Auth definition
	///
	AuthManager::AuthDefinition createToken(const QString &comment);

	///
	/// @brief Rename a token by id
	/// @param  id    The token id
	/// @param  comment The new comment
	/// @return        True on success else false (or not found)
	///
	bool renameToken(const QString &id, const QString &comment);

	///
	/// @brief Delete a token by id
	/// @param  id    The token id
	/// @return        True on success else false (or not found)
	///
	bool deleteToken(const QString &id);

	///
	/// @brief Change password of user
	/// @param  user  The username
	/// @param  pw    The CURRENT password
	/// @param  newPw The new password
	/// @return        True on success else false
	///
	bool updateUserPassword(const QString &user, const QString &pw, const QString &newPw);

	///
	/// @brief Generate a new pending token request with the provided comment and id as identifier helper
	/// @param  caller  The QObject of the caller to deliver the reply
	/// @param  comment The comment as ident helper
	/// @param  id      The id created by the caller
	/// @param  tan     The tan created by the caller
	///
	void setNewTokenRequest(QObject *caller, const QString &comment, const QString &id, const int &tan = 0);

	///
	/// @brief Cancel a pending token request with the provided comment and id as identifier helper
	/// @param  caller  The QObject of the caller to deliver the reply
	/// @param  id      The id created by the caller
	///
	void cancelNewTokenRequest(QObject *caller, const QString &, const QString &id);

	///
	/// @brief Handle a token request by id, generate token and inform token caller or deny
	/// @param id      The id of the request
	/// @param accept  The accept or deny the request
	///
	void handlePendingTokenRequest(const QString &id, bool accept);

	///
	/// @brief Get pending requests
	/// @return       All pending requests
	///
	QVector<AuthManager::AuthDefinition> getPendingRequests() const;

	///
	/// @brief Get the current valid token for user. Make sure this call is allowed!
	/// @param usr the defined user
	/// @return       The token
	///
	QString getUserToken(const QString &usr = "Hyperion") const;

	///
	/// @brief Get all available token entries
	///
	QVector<AuthManager::AuthDefinition> getTokenList() const;

	///
	/// @brief Handle settings update from Hyperion Settingsmanager emit
	/// @param type   settings type from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(settings::type type, const QJsonDocument &config);

signals:
	///
	/// @brief Emits whenever a new token Request has been created along with the id and comment
	/// @param id       The id of the request
	/// @param comment  The comment of the request; If the comment is EMPTY, it's a revoke of the caller!
	///
	void newPendingTokenRequest(const QString &id, const QString &comment);

	///
	/// @brief Emits when the user has accepted or denied a token
	/// @param  success If true the request was accepted else false and no token will be available
	/// @param  caller  The origin caller instance who requested this token
	/// @param  token   The new token that is now valid
	/// @param  comment The comment that was part of the request
	/// @param  id      The id that was part of the request
	/// @param  tan     The tan that was part of the request
	///
	void tokenResponse(bool success, QObject *caller, const QString &token, const QString &comment, const QString &id, const int &tan);

	///
	/// @brief Emits whenever the token list changes
	/// @param data  The full list of tokens
	///
	void tokenChange(QVector<AuthManager::AuthDefinition>);

private:
	///
	/// @brief Increment counter for token/user auth
	/// @param user If true we increment USER auth instead of token
	///
	void setAuthBlock(bool user = false);

	/// Database interface for auth table
	AuthTable *_authTable;

	/// Database interface for meta table
	MetaTable *_metaTable;

	/// Unique ID (imported from removed class 'Stats')
	QString _uuid;

	/// All pending requests
	QMap<QString, AuthDefinition> _pendingRequests;

	/// Reflect state of global auth
	bool _authRequired;

	/// Reflect state of local auth
	bool _localAuthRequired;

	/// Reflect state of local admin auth
	bool _localAdminAuthRequired;

	/// Timer for counting against pendingRequest timeouts
	QTimer *_timer;

	// Timer which cleans up the block counter
	QTimer *_authBlockTimer;

	// Contains timestamps of failed user login attempts
	QVector<uint64_t> _userAuthAttempts;

	// Contains timestamps of failed token login attempts
	QVector<uint64_t> _tokenAuthAttempts;

private slots:
	///
	/// @brief Check timeout of pending requests
	///
	void checkTimeout();

	///
	/// @brief Check if there are timeouts for failed login attempts
	///
	void checkAuthBlockTimeout();
};
