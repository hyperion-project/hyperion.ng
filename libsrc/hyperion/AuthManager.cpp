#include <hyperion/AuthManager.h>

// db
#include <db/AuthTable.h>
#include <db/MetaTable.h>

// qt
#include <QJsonObject>
#include <QTimer>

AuthManager *AuthManager::manager = nullptr;

AuthManager::AuthManager(QObject *parent, bool readonlyMode)
	: QObject(parent)
	, _authTable(new AuthTable("", this, readonlyMode))
	, _metaTable(new MetaTable(this, readonlyMode))
	, _pendingRequests()
	, _authRequired(true)
	, _timer(new QTimer(this))
	, _authBlockTimer(new QTimer(this))
{
	AuthManager::manager = this;

	// get uuid
	_uuid = _metaTable->getUUID();

	// Register meta
	qRegisterMetaType<QVector<AuthManager::AuthDefinition>>("QVector<AuthManager::AuthDefinition>");

	// setup timer
	_timer->setInterval(1000);
	connect(_timer, &QTimer::timeout, this, &AuthManager::checkTimeout);

	// setup authBlockTimer
	_authBlockTimer->setInterval(60000);
	connect(_authBlockTimer, &QTimer::timeout, this, &AuthManager::checkAuthBlockTimeout);

	// init with default user and password
	if (!_authTable->userExist("Hyperion"))
	{
		_authTable->createUser("Hyperion", "hyperion");
	}

	// update Hyperion user token on startup
	_authTable->setUserToken("Hyperion");
}

AuthManager::AuthDefinition AuthManager::createToken(const QString &comment)
{
	const QString token = QUuid::createUuid().toString().mid(1, 36);
	const QString id = QUuid::createUuid().toString().mid(1, 36).left(5);

	_authTable->createToken(token, comment, id);

	AuthDefinition def;
	def.comment = comment;
	def.token = token;
	def.id = id;

	emit tokenChange(getTokenList());
	return def;
}

QVector<AuthManager::AuthDefinition> AuthManager::getTokenList() const
{
	QVector<QVariantMap> vector = _authTable->getTokenList();
	QVector<AuthManager::AuthDefinition> finalVec;
	for (const auto &entry : vector)
	{
		AuthDefinition def;
		def.comment = entry["comment"].toString();
		def.id = entry["id"].toString();
		def.lastUse = entry["last_use"].toString();

		// don't add empty ids
		if (!entry["id"].toString().isEmpty())
			finalVec.append(def);
	}
	return finalVec;
}

QString AuthManager::getUserToken(const QString &usr) const
{
	QString tok = _authTable->getUserToken(usr);
	return QString(_authTable->getUserToken(usr));
}

void AuthManager::setAuthBlock(bool user)
{
	// current timestamp +10 minutes
	if (user)
		_userAuthAttempts.append(QDateTime::currentMSecsSinceEpoch() + 600000);
	else
		_tokenAuthAttempts.append(QDateTime::currentMSecsSinceEpoch() + 600000);

	_authBlockTimer->start();
}

bool AuthManager::isUserAuthorized(const QString &user, const QString &pw)
{
	if (isUserAuthBlocked())
		return false;

	if (!_authTable->isUserAuthorized(user, pw))
	{
		setAuthBlock(true);
		return false;
	}
	return true;
}

bool AuthManager::isTokenAuthorized(const QString &token)
{
	if (isTokenAuthBlocked())
		return false;

	if (!_authTable->tokenExist(token))
	{
		setAuthBlock();
		return false;
	}
	// timestamp update
	tokenChange(getTokenList());
	return true;
}

bool AuthManager::isUserTokenAuthorized(const QString &usr, const QString &token)
{
	if (isUserAuthBlocked())
		return false;

	if (!_authTable->isUserTokenAuthorized(usr, token))
	{
		setAuthBlock(true);
		return false;
	}
	return true;
}

bool AuthManager::updateUserPassword(const QString &user, const QString &pw, const QString &newPw)
{
	if (isUserAuthorized(user, pw))
		return _authTable->updateUserPassword(user, newPw);

	return false;
}

bool AuthManager::resetHyperionUser()
{
	return _authTable->resetHyperionUser();
}

void AuthManager::setNewTokenRequest(QObject *caller, const QString &comment, const QString &id, const int &tan)
{
	if (!_pendingRequests.contains(id))
	{
		AuthDefinition newDef{id, comment, caller, tan, uint64_t(QDateTime::currentMSecsSinceEpoch() + 180000)};
		_pendingRequests[id] = newDef;
		_timer->start();
		emit newPendingTokenRequest(id, comment);
	}
}

void AuthManager::cancelNewTokenRequest(QObject *caller, const QString &, const QString &id)
{
	if (_pendingRequests.contains(id))
	{
		AuthDefinition def = _pendingRequests.value(id);
		if (def.caller == caller)
			_pendingRequests.remove(id);
		emit newPendingTokenRequest(id, "");
	}
}

void AuthManager::handlePendingTokenRequest(const QString &id, bool accept)
{
	if (_pendingRequests.contains(id))
	{
		AuthDefinition def = _pendingRequests.take(id);

		if (accept)
		{
			const QString token = QUuid::createUuid().toString().remove("{").remove("}");
			_authTable->createToken(token, def.comment, id);
			emit tokenResponse(true, def.caller, token, def.comment, id, def.tan);
			emit tokenChange(getTokenList());
		}
		else
		{
			emit tokenResponse(false, def.caller, QString(), def.comment, id, def.tan);
		}
	}
}

QVector<AuthManager::AuthDefinition> AuthManager::getPendingRequests() const
{
	QVector<AuthManager::AuthDefinition> finalVec;
	for (const auto &entry : _pendingRequests)
	{
		AuthDefinition def;
		def.comment = entry.comment;
		def.id = entry.id;
		def.timeoutTime = entry.timeoutTime - QDateTime::currentMSecsSinceEpoch();
		finalVec.append(def);
	}
	return finalVec;
}

bool AuthManager::renameToken(const QString &id, const QString &comment)
{
	if (_authTable->renameToken(id, comment))
	{
		emit tokenChange(getTokenList());
		return true;
	}
	return false;
}

bool AuthManager::deleteToken(const QString &id)
{
	if (_authTable->deleteToken(id))
	{
		emit tokenChange(getTokenList());
		return true;
	}
	return false;
}

void AuthManager::handleSettingsUpdate(settings::type type, const QJsonDocument &config)
{
	if (type == settings::NETWORK)
	{
		const QJsonObject &obj = config.object();
		_authRequired = obj["apiAuth"].toBool(true);
		_localAuthRequired = obj["localApiAuth"].toBool(false);
		_localAdminAuthRequired = obj["localAdminAuth"].toBool(true);
	}
}

void AuthManager::checkTimeout()
{
	const uint64_t now = QDateTime::currentMSecsSinceEpoch();

	QMapIterator<QString, AuthDefinition> i(_pendingRequests);
	while (i.hasNext())
	{
		i.next();

		const AuthDefinition &def = i.value();
		if (def.timeoutTime <= now)
		{
			emit tokenResponse(false, def.caller, QString(), def.comment, def.id, def.tan);
			_pendingRequests.remove(i.key());
		}
	}
	// abort if empty
	if (_pendingRequests.isEmpty())
		_timer->stop();
}

void AuthManager::checkAuthBlockTimeout()
{
	// handle user auth block
	QMutableVectorIterator<uint64_t> itUserAuth(_userAuthAttempts);
	while (itUserAuth.hasNext()) {
		// after 10 minutes, we remove the entry
		if (itUserAuth.next() < static_cast<uint64_t>(QDateTime::currentMSecsSinceEpoch()))
			itUserAuth.remove();
	}

	// handle token auth block
	QMutableVectorIterator<uint64_t> itTokenAuth(_tokenAuthAttempts);
	while (itTokenAuth.hasNext()) {
		// after 10 minutes, we remove the entry
		if (itTokenAuth.next() < static_cast<uint64_t>(QDateTime::currentMSecsSinceEpoch()))
			itTokenAuth.remove();
	}

	// if the lists are empty we stop
	if (_userAuthAttempts.empty() && _tokenAuthAttempts.empty())
	{
		_authBlockTimer->stop();
	}
}
