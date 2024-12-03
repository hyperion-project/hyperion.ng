// project includes
#include <api/API.h>

// stl includes

// Qt includes
#include <QResource>
#include <QDateTime>
#include <QCryptographicHash>
#include <QImage>
#include <QImageReader>
#include <QBuffer>
#include <QByteArray>
#include <QTimer>
#include <QThread>
#include <QSharedPointer>

// hyperion includes
#include <utils/jsonschema/QJsonFactory.h>
#include <utils/jsonschema/QJsonSchemaChecker.h>
#include <HyperionConfig.h>
#include <utils/SysInfo.h>
#include <utils/ColorSys.h>
#include <utils/Process.h>

// ledmapping int <> string transform methods
#include <hyperion/ImageProcessor.h>

using namespace hyperion;

// Constants
namespace {

const int IMAGE_HEIGHT_MAX = 2000;
const int IMAGE_WIDTH_MAX = 2000;
const int IMAGE_SCALE = 2000;
}

API::API(Logger *log, bool localConnection, QObject *parent)
	: QObject(parent)
{
	qRegisterMetaType<int64_t>("int64_t");
	qRegisterMetaType<VideoMode>("VideoMode");
	qRegisterMetaType<std::map<int, registerData>>("std::map<int,registerData>");

	// Init
	_log = log;
	_authManager = AuthManager::getInstance();
	_instanceManager = HyperionIManager::getInstance();
	_localConnection = localConnection;

	_authorized = false;
	_adminAuthorized = false;

	_currInstanceIndex = 0;

	// connect to possible token responses that has been requested
	connect(_authManager, &AuthManager::tokenResponse, this, [=] (bool success, const QObject *caller, const QString &token, const QString &comment, const QString &tokenId, const int &tan)
	{
		if (this == caller)
		{
			emit onTokenResponse(success, token, comment, tokenId, tan);
		}
	});

	connect(_instanceManager, &HyperionIManager::startInstanceResponse, this, [=] (const QObject *caller, const int &tan)
	{
		if (this == caller)
		{
			emit onStartInstanceResponse(tan);
		}
	});
}

void API::init()
{
	_hyperion = _instanceManager->getHyperionInstance(0);
	_authorized = false;

	// For security we block external connections, if default PW is set
	if (!_localConnection && API::hasHyperionDefaultPw())
	{
		Warning(_log, "Non local network connect attempt identified, but default Hyperion passwort set! - Reject connection.");
		emit forceClose();
	}

	// if this is localConnection and network allows unauth locals
	if ( _localConnection && !_authManager->isLocalAuthRequired())
	{
		_authorized = true;
	}

	// // admin access is only allowed after login via user & password or via authorization via token.
	_adminAuthorized = false;
}

void API::setColor(int priority, const std::vector<uint8_t> &ledColors, int timeout_ms, const QString &origin, hyperion::Components /*callerComp*/)
{
	if (ledColors.size() % 3 == 0)
	{
		std::vector<ColorRgb> fledColors;
		for (unsigned i = 0; i < ledColors.size(); i += 3)
		{
			fledColors.emplace_back(ColorRgb{ledColors[i], ledColors[i + 1], ledColors[i + 2]});
		}

		QMetaObject::invokeMethod(_hyperion.get(), "setColor", Qt::QueuedConnection, Q_ARG(int, priority), Q_ARG(std::vector<ColorRgb>, fledColors), Q_ARG(int, timeout_ms), Q_ARG(QString, origin));
	}
}

bool API::setImage(ImageCmdData &data, hyperion::Components comp, QString &replyMsg, hyperion::Components /*callerComp*/)
{
	// truncate name length
	data.imgName.truncate(16);

	if (!data.format.isEmpty())
	{
		if (data.format == "auto")
		{
			data.format = "";
		}
		else
		{
			if (!QImageReader::supportedImageFormats().contains(data.format.toLower().toUtf8()))
			{
				replyMsg = "The given format [" + data.format + "] is not supported";
				return false;
			}
		}

		QImage img = QImage::fromData(data.data, QSTRING_CSTR(data.format));
		if (img.isNull())
		{
			replyMsg = "Failed to parse picture, the file might be corrupted or content does not match the given format [" + data.format + "]";
			return false;
		}

		// check for requested scale
		if (data.scale > 24)
		{
			if (img.height() > data.scale)
			{
				img = img.scaledToHeight(data.scale);
			}
			if (img.width() > data.scale)
			{
				img = img.scaledToWidth(data.scale);
			}
		}

		// check if we need to force a scale
		if (img.width() > IMAGE_WIDTH_MAX || img.height() > IMAGE_HEIGHT_MAX)
		{
			data.scale = IMAGE_SCALE;
			if (img.height() > data.scale)
			{
				img = img.scaledToHeight(data.scale);
			}
			if (img.width() > data.scale)
			{
				img = img.scaledToWidth(data.scale);
			}
		}

		data.width = img.width();
		data.height = img.height();

		// extract image
		img = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
		data.data.clear();
		data.data.reserve(static_cast<int>(img.width() * img.height() * 3));
		for (int i = 0; i < img.height(); ++i)
		{
			const QRgb *scanline = reinterpret_cast<const QRgb *>(img.scanLine(i));
			for (int j = 0; j < img.width(); ++j)
			{
				data.data.append(static_cast<char>(qRed(scanline[j])));
				data.data.append(static_cast<char>(qGreen(scanline[j])));
				data.data.append(static_cast<char>(qBlue(scanline[j])));
			}
		}
	}
	else
	{
		// check consistency of the size of the received data
		if (static_cast<size_t>(data.data.size()) != static_cast<size_t>(data.width) * static_cast<size_t>(data.height) * 3)
		{
			replyMsg = "Size of image data does not match with the width and height";
			return false;
		}
	}

	// copy image
	Image<ColorRgb> image(data.width, data.height);
	memcpy(image.memptr(), data.data.data(), static_cast<size_t>(data.data.size()));

	QMetaObject::invokeMethod(_hyperion.get(), "registerInput", Qt::QueuedConnection, Q_ARG(int, data.priority), Q_ARG(hyperion::Components, comp), Q_ARG(QString, data.origin), Q_ARG(QString, data.imgName));
	QMetaObject::invokeMethod(_hyperion.get(), "setInputImage", Qt::QueuedConnection, Q_ARG(int, data.priority), Q_ARG(Image<ColorRgb>, image), Q_ARG(int64_t, data.duration));

	return true;
}

bool API::clearPriority(int priority, QString &replyMsg, hyperion::Components /*callerComp*/)
{
	if (priority < 0 || (priority > 0 && priority < PriorityMuxer::BG_PRIORITY))
	{
		QMetaObject::invokeMethod(_hyperion.get(), "clear", Qt::QueuedConnection, Q_ARG(int, priority));
	}
	else
	{
		replyMsg = QString("Priority %1 is not allowed to be cleared").arg(priority);
		return false;
	}
	return true;
}

bool API::setComponentState(const QString &comp, bool &compState, QString &replyMsg, hyperion::Components /*callerComp*/)
{
	Components component = stringToComponent(comp);

	if (component != COMP_INVALID)
	{
		QMetaObject::invokeMethod(_hyperion.get(), "compStateChangeRequest", Qt::QueuedConnection, Q_ARG(hyperion::Components, component), Q_ARG(bool, compState));
		return true;
	}
	replyMsg = QString("Unknown component name: %1").arg(comp);
	return false;
}

void API::setLedMappingType(int type, hyperion::Components /*callerComp*/)
{
	QMetaObject::invokeMethod(_hyperion.get(), "setLedMappingType", Qt::QueuedConnection, Q_ARG(int, type));
}

void API::setVideoMode(VideoMode mode, hyperion::Components /*callerComp*/)
{
	QMetaObject::invokeMethod(_hyperion.get(), "setVideoMode", Qt::QueuedConnection, Q_ARG(VideoMode, mode));
}

#if defined(ENABLE_EFFECTENGINE)
bool API::setEffect(const EffectCmdData &dat, hyperion::Components /*callerComp*/)
{
	int isStarted;
	if (!dat.args.isEmpty())
	{
		QMetaObject::invokeMethod(_hyperion.get(), "setEffect", Qt::BlockingQueuedConnection, Q_RETURN_ARG(int, isStarted), Q_ARG(QString, dat.effectName), Q_ARG(QJsonObject, dat.args), Q_ARG(int, dat.priority), Q_ARG(int, dat.duration), Q_ARG(QString, dat.pythonScript), Q_ARG(QString, dat.origin), Q_ARG(QString, dat.data));
	}
	else
	{
		QMetaObject::invokeMethod(_hyperion.get(), "setEffect", Qt::BlockingQueuedConnection, Q_RETURN_ARG(int, isStarted), Q_ARG(QString, dat.effectName), Q_ARG(int, dat.priority), Q_ARG(int, dat.duration), Q_ARG(QString, dat.origin));
	}

	return isStarted >= 0;
}
#endif

void API::setSourceAutoSelect(bool state, hyperion::Components /*callerComp*/)
{
	QMetaObject::invokeMethod(_hyperion.get(), "setSourceAutoSelect", Qt::QueuedConnection, Q_ARG(bool, state));
}

void API::setVisiblePriority(int priority, hyperion::Components /*callerComp*/)
{
	QMetaObject::invokeMethod(_hyperion.get(), "setVisiblePriority", Qt::QueuedConnection, Q_ARG(int, priority));
}

void API::registerInput(int priority, hyperion::Components component, const QString &origin, const QString &owner, hyperion::Components callerComp)
{
	if (_activeRegisters.count(priority) != 0)
	{
		_activeRegisters.erase(priority);
	}

	_activeRegisters.insert({priority, registerData{component, origin, owner, callerComp}});

	QMetaObject::invokeMethod(_hyperion.get(), "registerInput", Qt::QueuedConnection, Q_ARG(int, priority), Q_ARG(hyperion::Components, component), Q_ARG(QString, origin), Q_ARG(QString, owner));
}

void API::unregisterInput(int priority)
{
	if (_activeRegisters.count(priority) != 0)
	{
		_activeRegisters.erase(priority);
	}
}

bool API::setHyperionInstance(quint8 inst)
{
	if (_currInstanceIndex == inst)
	{
		return true;
	}

	bool isRunning;
	QMetaObject::invokeMethod(_instanceManager, "IsInstanceRunning", Qt::DirectConnection, Q_RETURN_ARG(bool, isRunning), Q_ARG(quint8, inst));
	if (!isRunning)
	{
		return false;
	}

	disconnect(_hyperion.get(), nullptr, this, nullptr);
	QMetaObject::invokeMethod(_instanceManager, "getHyperionInstance", Qt::DirectConnection, Q_RETURN_ARG(QSharedPointer<Hyperion>, _hyperion), Q_ARG(quint8, inst));
	_currInstanceIndex = inst;

	return true;
}

bool API::isHyperionEnabled()
{
	int isEnabled;
	QMetaObject::invokeMethod(_hyperion.get(), "isComponentEnabled", Qt::BlockingQueuedConnection, Q_RETURN_ARG(int, isEnabled), Q_ARG(hyperion::Components, hyperion::COMP_ALL));
	return isEnabled > 0;
}

QVector<QVariantMap> API::getAllInstanceData() const
{
	QVector<QVariantMap> vec;
	QMetaObject::invokeMethod(_instanceManager, "getInstanceData", Qt::DirectConnection, Q_RETURN_ARG(QVector<QVariantMap>, vec));
	return vec;
}

bool API::startInstance(quint8 index, int tan)
{
	bool isStarted;
	(_instanceManager->thread() != this->thread())
			? QMetaObject::invokeMethod(_instanceManager, "startInstance", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, isStarted), Q_ARG(quint8, index), Q_ARG(bool, false), Q_ARG(QObject*, this), Q_ARG(int, tan))
			: isStarted = _instanceManager->startInstance(index, false, this, tan);

	return isStarted;
}

void API::stopInstance(quint8 index)
{
	QMetaObject::invokeMethod(_instanceManager, "stopInstance", Qt::QueuedConnection, Q_ARG(quint8, index));
}

bool API::deleteInstance(quint8 index, QString &replyMsg)
{
	if (_adminAuthorized)
	{
		QMetaObject::invokeMethod(_instanceManager, "deleteInstance", Qt::QueuedConnection, Q_ARG(quint8, index));
		return true;
	}
	replyMsg = NO_AUTHORIZATION;
	return false;
}

QString API::createInstance(const QString &name)
{
	if (_adminAuthorized)
	{
		bool success;
		QMetaObject::invokeMethod(_instanceManager, "createInstance", Qt::DirectConnection, Q_RETURN_ARG(bool, success), Q_ARG(QString, name));
		if (!success)
		{
			return QString("Instance name '%1' is already in use").arg(name);
		}
		return "";
	}
	return NO_AUTHORIZATION;
}

QString API::setInstanceName(quint8 index, const QString &name)
{
	if (_adminAuthorized)
	{
		QMetaObject::invokeMethod(_instanceManager, "saveName", Qt::QueuedConnection, Q_ARG(quint8, index), Q_ARG(QString, name));
		return "";
	}
	return NO_AUTHORIZATION;
}

#if defined(ENABLE_EFFECTENGINE)
QString API::deleteEffect(const QString &name)
{
	if (_adminAuthorized)
	{
		QString res;
		QMetaObject::invokeMethod(_hyperion.get(), "deleteEffect", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QString, res), Q_ARG(QString, name));
		return res;
	}
	return NO_AUTHORIZATION;
}

QString API::saveEffect(const QJsonObject &data)
{
	if (_adminAuthorized)
	{
		QString res;
		QMetaObject::invokeMethod(_hyperion.get(), "saveEffect", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QString, res), Q_ARG(QJsonObject, data));
		return res;
	}
	return NO_AUTHORIZATION;
}
#endif

bool API::updateHyperionPassword(const QString &password, const QString &newPassword)
{
	bool isPwUpdated {true};
	if (!_adminAuthorized)
	{
		isPwUpdated = false;
	}
	else
	{
		QMetaObject::invokeMethod(_authManager, "updateUserPassword", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, isPwUpdated), Q_ARG(QString, DEFAULT_USER), Q_ARG(QString, password), Q_ARG(QString, newPassword));
	}
	return isPwUpdated;
}

QString API::createToken(const QString &comment, AuthManager::AuthDefinition &def)
{
	if (!_adminAuthorized)
	{
		return NO_AUTHORIZATION;
	}

	if (comment.isEmpty())
	{
		return "Missing token comment";
	}
	QMetaObject::invokeMethod(_authManager, "createToken", Qt::BlockingQueuedConnection, Q_RETURN_ARG(AuthManager::AuthDefinition, def), Q_ARG(QString, comment));
	return "";
}

QString API::renameToken(const QString &tokenId, const QString &comment)
{
	if (!_adminAuthorized)
	{
		return NO_AUTHORIZATION;
	}

	if (comment.isEmpty())
	{
		return "Missing token comment";
	}

	if (tokenId.isEmpty()) {
		return "Missing token id";
	}

	bool isTokenRenamed {false};
	QMetaObject::invokeMethod(_authManager, "renameToken", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, isTokenRenamed), Q_ARG(QString, tokenId),  Q_ARG(QString, comment));

	return (!isTokenRenamed) ? "Token does not exist" : "";
}

QString API::deleteToken(const QString &tokenId)
{
	if (!_adminAuthorized)
	{
		return NO_AUTHORIZATION;
	}

	if (tokenId.isEmpty())
	{
		return "Missing token id";
	}

	bool isTokenDeleted {false};
	QMetaObject::invokeMethod(_authManager, "deleteToken", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, isTokenDeleted), Q_ARG(QString, tokenId));

	return (!isTokenDeleted) ? "Token does not exist" : "";
}

void API::setNewTokenRequest(const QString &comment, const QString &tokenId, const int &tan)
{
	QMetaObject::invokeMethod(_authManager, "setNewTokenRequest", Qt::QueuedConnection, Q_ARG(QObject *, this), Q_ARG(QString, comment), Q_ARG(QString, tokenId), Q_ARG(int, tan));
}

void API::cancelNewTokenRequest(const QString &comment, const QString &tokenId)
{
	QMetaObject::invokeMethod(_authManager, "cancelNewTokenRequest", Qt::QueuedConnection, Q_ARG(QObject *, this), Q_ARG(QString, comment), Q_ARG(QString, tokenId));
}

bool API::handlePendingTokenRequest(const QString &tokenId, bool accept)
{
	if (!_adminAuthorized)
	{
		return false;
	}
	QMetaObject::invokeMethod(_authManager, "handlePendingTokenRequest", Qt::QueuedConnection, Q_ARG(QString, tokenId), Q_ARG(bool, accept));
	return true;
}

bool API::getTokenList(QVector<AuthManager::AuthDefinition> &def)
{
	if (!_adminAuthorized)
	{
		return false;
	}
	QMetaObject::invokeMethod(_authManager, "getTokenList", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QVector<AuthManager::AuthDefinition>, def));
	return true;
}

bool API::getPendingTokenRequests(QVector<AuthManager::AuthDefinition> &map)
{
	if (!_adminAuthorized)
	{
		return false;
	}
	QMetaObject::invokeMethod(_authManager, "getPendingRequests", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QVector<AuthManager::AuthDefinition>, map));
	return true;
}

bool API::isUserTokenAuthorized(const QString &userToken)
{
	QMetaObject::invokeMethod(_authManager, "isUserTokenAuthorized", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, _authorized), Q_ARG(QString, DEFAULT_USER), Q_ARG(QString, userToken));
	_adminAuthorized = _authorized;

	if (_authorized)
	{
		// Listen for ADMIN ACCESS protected signals
		connect(_authManager, &AuthManager::newPendingTokenRequest, this, &API::onPendingTokenRequest);
	}
	else
	{
		disconnect(_authManager, &AuthManager::newPendingTokenRequest, this, &API::onPendingTokenRequest);
	}
	return _authorized;
}

bool API::getUserToken(QString &userToken)
{
	if (!_adminAuthorized)
	{
		return false;
	}
	QMetaObject::invokeMethod(_authManager, "getUserToken", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QString, userToken));
	return true;
}

bool API::isTokenAuthorized(const QString &token)
{
	(_authManager->thread() != this->thread())
			? QMetaObject::invokeMethod(_authManager, "isTokenAuthorized", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, _authorized), Q_ARG(QString, token))
			: _authorized = _authManager->isTokenAuthorized(token);
	_adminAuthorized = _authorized;

	return _authorized;
}

bool API::isUserAuthorized(const QString &password)
{
	bool isUserAuthorized;
	QMetaObject::invokeMethod(_authManager, "isUserAuthorized", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, isUserAuthorized), Q_ARG(QString, DEFAULT_USER), Q_ARG(QString, password));
	if (isUserAuthorized)
	{
		_authorized = true;
		_adminAuthorized = true;

		// Listen for ADMIN ACCESS protected signals
		connect(_authManager, &AuthManager::newPendingTokenRequest, this, &API::onPendingTokenRequest);
	}
	else
	{
		disconnect(_authManager, &AuthManager::newPendingTokenRequest, this, &API::onPendingTokenRequest);
	}
	return isUserAuthorized;
}

bool API::hasHyperionDefaultPw()
{
	bool isDefaultPassort;
	QMetaObject::invokeMethod(_authManager, "isUserAuthorized", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, isDefaultPassort), Q_ARG(QString, DEFAULT_USER), Q_ARG(QString, DEFAULT_PASSWORD));
	return isDefaultPassort;
}

void API::logout()
{
	_authorized = false;
	_adminAuthorized = false;
	// Stop listenig for ADMIN ACCESS protected signals
	disconnect(_authManager, &AuthManager::newPendingTokenRequest, this, &API::onPendingTokenRequest);
	stopDataConnections();
}
