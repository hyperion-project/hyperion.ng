#include <hyperion/AuthManager.h>

// util
#include <db/AuthTable.h>

// qt
#include <QJsonObject>
#include <QTimer>

AuthManager* AuthManager::manager = nullptr;

AuthManager::AuthManager(const QString& rootPath, QObject* parent)
	: QObject(parent)
	, _authTable(new AuthTable(rootPath, this))
	, _pendingRequests()
	, _authRequired(true)
	, _timer(new QTimer(this))
{
	AuthManager::manager = this;

	// setup timer
	_timer->setInterval(1000);
	connect(_timer, &QTimer::timeout, this, &AuthManager::checkTimeout);

	// init with default user and password
	if(!_authTable->userExist("Hyperion"))
	{
		_authTable->createUser("Hyperion","hyperion");
	}
}

const bool & AuthManager::isAuthRequired()
{
	return _authRequired;
}

const bool & AuthManager::isLocalAuthRequired()
{
	return _localAuthRequired;
}

const AuthManager::AuthDefinition AuthManager::createToken(const QString& comment)
{
	const QString token = QUuid::createUuid().toString().mid(1, 36);
	const QString id = QUuid::createUuid().toString().mid(1, 36).left(5);

	_authTable->createToken(token, comment, id);

	AuthDefinition def;
	def.comment = comment;
	def.token = token;
	def.id = id;

	return def;
}

const QVector<AuthManager::AuthDefinition> AuthManager::getTokenList()
{
	QVector<QVariantMap> vector = _authTable->getTokenList();
	QVector<AuthManager::AuthDefinition> finalVec;
	for(const auto& entry : vector)
	{
		AuthDefinition def;
		def.comment = entry["comment"].toString();
		def.id = entry["id"].toString();
		def.lastUse = entry["last_use"].toString();

		// don't add empty ids
		if(!entry["id"].toString().isEmpty())
			finalVec.append(def);
	}
	return finalVec;
}

const bool AuthManager::isUserAuthorized(const QString& user, const QString& pw)
{
	return _authTable->isUserAuthorized(user, pw);
}

const bool AuthManager::isTokenAuthorized(const QString& token)
{
	return _authTable->tokenExist(token);
}

void AuthManager::setNewTokenRequest(QObject* caller, const QString& comment, const QString& id)
{
	if(!_pendingRequests.contains(id))
	{
		AuthDefinition newDef {id, comment, caller, uint64_t(QDateTime::currentMSecsSinceEpoch()+60000)};
		_pendingRequests[id] = newDef;
		_timer->start();
		emit newPendingTokenRequest(id, comment);
	}
}

const bool AuthManager::acceptTokenRequest(const QString& id)
{
	if(_pendingRequests.contains(id))
	{
		const QString token = QUuid::createUuid().toString().remove("{").remove("}");
		AuthDefinition def = _pendingRequests.take(id);
		_authTable->createToken(token, def.comment, id);
		emit tokenResponse(true, def.caller, token, def.comment, id);
		return true;
	}
	return false;
}

const bool AuthManager::denyTokenRequest(const QString& id)
{
	if(_pendingRequests.contains(id))
	{
		AuthDefinition def = _pendingRequests.take(id);
		emit tokenResponse(false, def.caller, QString(), def.comment, id);
		return true;
	}
	return false;
}

const QMap<QString, AuthManager::AuthDefinition> AuthManager::getPendingRequests()
{
	return _pendingRequests;
}

const bool AuthManager::deleteToken(const QString& id)
{
	if(_authTable->deleteToken(id))
	{
		//emit tokenDeleted(token);
		return true;
	}
	return false;
}

void AuthManager::handleSettingsUpdate(const settings::type& type, const QJsonDocument& config)
{
	if(type == settings::NETWORK)
	{
		const QJsonObject& obj = config.object();
		_authRequired = obj["apiAuth"].toBool(true);
		_localAuthRequired = obj["localApiAuth"].toBool(false);
	}
}

void AuthManager::checkTimeout()
{
	const uint64_t now = QDateTime::currentMSecsSinceEpoch();

	QMapIterator<QString, AuthDefinition> i(_pendingRequests);
	while (i.hasNext())
	{
	    i.next();

		const AuthDefinition& def = i.value();
		if(def.timeoutTime <= now)
		{
			emit tokenResponse(false, def.caller, QString(), def.comment, def.id);
			_pendingRequests.remove(i.key());
		}
	}
	// abort if empty
	if(_pendingRequests.isEmpty())
		_timer->stop();
}
