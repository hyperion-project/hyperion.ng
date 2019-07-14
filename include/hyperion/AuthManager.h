#pragma once

#include <utils/Logger.h>
#include <utils/settings.h>

//qt
#include <QMap>

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
	AuthManager(QObject* parent = 0);

public:
	struct AuthDefinition{
		QString id;
		QString comment;
		QObject* caller;
		uint64_t timeoutTime;
		QString token;
		QString lastUse;
	};

	///
	/// @brief Get the unique id (imported from removed class 'Stats')
	/// @return The unique id
	///
	const QString & getID() { return _uuid; };

	///
	/// @brief Get all available token entries
	///
	const QVector<AuthDefinition> getTokenList();

	///
	/// @brief Check authorization is required according to the user setting
	/// @return       True if authorization required else false
	///
	bool & isAuthRequired();

	///
	/// @brief Check if authorization is required for local network connections
	/// @return       True if authorization required else false
	///
	bool & isLocalAuthRequired();

	///
	/// @brief Create a new token and skip the usual chain
	/// @param  comment The comment that should be used for
	/// @return         The new Auth definition
	///
	const AuthDefinition createToken(const QString& comment);

	///
	/// @brief Check if user is authorized
	/// @param  user  The username
	/// @param  pw    The password
	/// @return        True if authorized else false
	///
	bool isUserAuthorized(const QString& user, const QString& pw);

	///
	/// @brief Check if token is authorized
	/// @param  token  The token
	/// @return        True if authorized else false
	///
	bool isTokenAuthorized(const QString& token);

	///
	/// @brief Generate a new pending token request with the provided comment and id as identifier helper
	/// @param  caller  The QObject of the caller to deliver the reply
	/// @param  comment The comment as ident helper
	/// @param  id      The id created by the caller
	///
	void setNewTokenRequest(QObject* caller, const QString& comment, const QString& id);

	///
	/// @brief Accept a token request by id, generate token and inform token caller
	/// @param id      The id of the request
	/// @return        True on success, false if not found
	///
	bool acceptTokenRequest(const QString& id);

	///
	/// @brief Deny a token request by id, inform the requester
	/// @param id      The id of the request
	/// @return        True on success, false if not found
	///
	bool denyTokenRequest(const QString& id);

	///
	/// @brief Get pending requests
	/// @return       All pending requests
	///
	const QMap<QString, AuthDefinition> getPendingRequests();

	///
	/// @brief Delete a token by id
	/// @param  id    The token id
	/// @return        True on success else false (or not found)
	///
	bool deleteToken(const QString& id);

	/// Pointer of this instance
	static AuthManager* manager;
	/// Get Pointer of this instance
	static AuthManager* getInstance() { return manager; };

public slots:
	///
	/// @brief Handle settings update from Hyperion Settingsmanager emit
	/// @param type   settings type from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(const settings::type& type, const QJsonDocument& config);

signals:
	///
	/// @brief Emits whenever a new token Request has been created along with the id and comment
	/// @param id       The id of the request
	/// @param comment  The comment of the request
	///
	void newPendingTokenRequest(const QString& id, const QString& comment);

	///
	/// @brief Emits when the user has accepted or denied a token
	/// @param  success If true the request was accepted else false and no token will be available
	/// @param  caller  The origin caller instance who requested this token
	/// @param  token   The new token that is now valid
	/// @param  comment The comment that was part of the request
	/// @param  id      The id that was part of the request
	///
	void tokenResponse(const bool& success, QObject* caller, const QString& token, const QString& comment, const QString& id);

private:
	/// Database interface for auth table
	AuthTable* _authTable;

	/// Database interface for meta table
	MetaTable* _metaTable;

	/// Unique ID (imported from removed class 'Stats')
	QString _uuid;

	/// All pending requests
	QMap<QString,AuthDefinition> _pendingRequests;

	/// Reflect state of global auth
	bool _authRequired;

	/// Reflect state of local auth
	bool _localAuthRequired;

	/// Timer for counting against pendingRequest timeouts
	QTimer* _timer;

private slots:
	///
	/// @brief Check timeout of pending requests
	///
	void checkTimeout();
};
