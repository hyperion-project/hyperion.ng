// project includes
#include <api/API.h>

// stl includes
#include <iostream>
#include <iterator>

// Qt includes
#include <QResource>
#include <QDateTime>
#include <QCryptographicHash>
#include <QImage>
#include <QBuffer>
#include <QByteArray>
#include <QTimer>

// hyperion includes
#include <leddevice/LedDeviceWrapper.h>
#include <hyperion/GrabberWrapper.h>
#include <utils/jsonschema/QJsonFactory.h>
#include <utils/jsonschema/QJsonSchemaChecker.h>
#include <HyperionConfig.h>
#include <utils/SysInfo.h>
#include <utils/ColorSys.h>
#include <utils/Process.h>
//#include <utils/ApiSync.h>

// bonjour wrapper
#include <bonjour/bonjourbrowserwrapper.h>

// ledmapping int <> string transform methods
#include <hyperion/ImageProcessor.h>

// api includes
#include <api/JsonCB.h>

using namespace hyperion;

API::API(Logger *log, const bool &localConnection, QObject *parent)
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

    _hyperion = _instanceManager->getHyperionInstance(0);
    _currInstanceIndex = 0;
    // TODO FIXME
    // report back current registers when a Hyperion instance request it
    //connect(ApiSync::getInstance(), &ApiSync::requestActiveRegister, this, &API::requestActiveRegister, Qt::QueuedConnection);

    // connect to possible token responses that has been requested
    connect(_authManager, &AuthManager::tokenResponse, this, &API::checkTokenResponse);
}

void API::init(void)
{
    bool apiAuthRequired = _authManager->isAuthRequired();

    // For security we block external connections if default PW is set
    if (!_localConnection && API::hasHyperionDefaultPw())
    {
        emit forceClose();
    }
    // if this is localConnection and network allows unauth locals, set authorized flag
    if (apiAuthRequired && _localConnection)
        _authorized = !_authManager->isLocalAuthRequired();

    // admin access is allowed, when the connection is local and the option for local admin isn't set. Con: All local connections get full access
    if (_localConnection)
    {
        _adminAuthorized = !_authManager->isLocalAdminAuthRequired();
        // just in positive direction
        if (_adminAuthorized)
            _authorized = true;
    }
}

void API::setColor(const int &priority, const std::vector<uint8_t> &ledColors, const int &timeout_ms, const QString &origin, const hyperion::Components &callerComp)
{
    std::vector<ColorRgb> fledColors;
    if (ledColors.size() % 3 == 0)
    {
        for (unsigned i = 0; i < ledColors.size(); i += 3)
        {
            fledColors.emplace_back(ColorRgb{ledColors[i], ledColors[i + 1], ledColors[i + 2]});
        }
        QMetaObject::invokeMethod(_hyperion, "setColor", Qt::QueuedConnection, Q_ARG(int, priority), Q_ARG(std::vector<ColorRgb>, fledColors), Q_ARG(int, timeout_ms), Q_ARG(QString, origin));
        //QMetaObject::invokeMethod(ApiSync::getInstance(), "setColor", Qt::QueuedConnection, Q_ARG(QObject *, _hyperion), Q_ARG(int, priority), Q_ARG(std::vector<ColorRgb>, fledColors), Q_ARG(int, timeout_ms), Q_ARG(QString, origin), Q_ARG(hyperion::Components, callerComp));
    }
}

bool API::setImage(ImageCmdData &data, hyperion::Components comp, QString &replyMsg, const hyperion::Components &callerComp)
{
    // truncate name length
    data.imgName.truncate(16);

    if (data.format == "auto")
    {
        QImage img = QImage::fromData(data.data);
        if (img.isNull())
        {
            replyMsg = "Failed to parse picture, the file might be corrupted";
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
        if (img.width() > 2000 || img.height() > 2000)
        {
            data.scale = 2000;
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
        data.data.reserve(img.width() * img.height() * 3);
        for (int i = 0; i < img.height(); ++i)
        {
            const QRgb *scanline = reinterpret_cast<const QRgb *>(img.scanLine(i));
            for (int j = 0; j < img.width(); ++j)
            {
                data.data.append((char)qRed(scanline[j]));
                data.data.append((char)qGreen(scanline[j]));
                data.data.append((char)qBlue(scanline[j]));
            }
        }
    }
    else
    {
        // check consistency of the size of the received data
        if (data.data.size() != data.width * data.height * 3)
        {
            replyMsg = "Size of image data does not match with the width and height";
            return false;
        }
    }

    // copy image
    Image<ColorRgb> image(data.width, data.height);
    memcpy(image.memptr(), data.data.data(), data.data.size());

    QMetaObject::invokeMethod(_hyperion, "registerInput", Qt::QueuedConnection, Q_ARG(int, data.priority), Q_ARG(hyperion::Components, comp), Q_ARG(QString, data.origin), Q_ARG(QString, data.imgName));
    QMetaObject::invokeMethod(_hyperion, "setInputImage", Qt::QueuedConnection, Q_ARG(int, data.priority), Q_ARG(Image<ColorRgb>, image), Q_ARG(int64_t, data.duration));

    //QMetaObject::invokeMethod(ApiSync::getInstance(), "registerInput", Qt::QueuedConnection, Q_ARG(QObject *, _hyperion), Q_ARG(int, data.priority), Q_ARG(hyperion::Components, comp), Q_ARG(QString, data.origin), Q_ARG(QString, data.imgName), Q_ARG(hyperion::Components, callerComp));
    //QMetaObject::invokeMethod(ApiSync::getInstance(), "setInputImage", Qt::QueuedConnection, Q_ARG(QObject *, _hyperion), Q_ARG(int, data.priority), Q_ARG(Image<ColorRgb>, image), Q_ARG(int64_t, data.duration), Q_ARG(hyperion::Components, comp), Q_ARG(hyperion::Components, callerComp));

    return true;
}

bool API::clearPriority(const int &priority, QString &replyMsg, const hyperion::Components &callerComp)
{
    if (priority < 0 || (priority > 0 && priority < 254))
    {
        QMetaObject::invokeMethod(_hyperion, "clear", Qt::QueuedConnection, Q_ARG(int, priority));
        //QMetaObject::invokeMethod(ApiSync::getInstance(), "clearPriority", Qt::QueuedConnection, Q_ARG(QObject *, _hyperion), Q_ARG(int, priority), Q_ARG(hyperion::Components, callerComp));
    }
    else
    {
        replyMsg = QString("Priority %1 is not allowed to be cleared").arg(priority);
        return false;
    }
    return true;
}

bool API::setComponentState(const QString &comp, bool &compState, QString &replyMsg, const hyperion::Components &callerComp)
{
    Components component = stringToComponent(comp);

    if (component != COMP_INVALID)
    {
        QMetaObject::invokeMethod(_hyperion, "compStateChangeRequest", Qt::QueuedConnection, Q_ARG(hyperion::Components, component), Q_ARG(bool, compState));
        //QMetaObject::invokeMethod(ApiSync::getInstance(), "compStateChangeRequest", Qt::QueuedConnection, Q_ARG(QObject *, _hyperion), Q_ARG(hyperion::Components, component), Q_ARG(bool, compState), Q_ARG(hyperion::Components, callerComp));
        return true;
    }
    replyMsg = QString("Unknown component name: %1").arg(comp);
    return false;
}

void API::setLedMappingType(const int &type, const hyperion::Components &callerComp)
{
    QMetaObject::invokeMethod(_hyperion, "setLedMappingType", Qt::QueuedConnection, Q_ARG(int, type));
    //QMetaObject::invokeMethod(ApiSync::getInstance(), "setLedMappingType", Qt::QueuedConnection, Q_ARG(QObject *, _hyperion), Q_ARG(int, type), Q_ARG(hyperion::Components, callerComp));
}

void API::setVideoMode(const VideoMode &mode, const hyperion::Components &callerComp)
{
    QMetaObject::invokeMethod(_hyperion, "setVideoMode", Qt::QueuedConnection, Q_ARG(VideoMode, mode));
    //QMetaObject::invokeMethod(ApiSync::getInstance(), "setVideoMode", Qt::QueuedConnection, Q_ARG(QObject *, _hyperion), Q_ARG(VideoMode, mode), Q_ARG(hyperion::Components, callerComp));
}

void API::setEffect(const EffectCmdData &dat, const hyperion::Components &callerComp)
{
    if (!dat.args.isEmpty())
    {
        QMetaObject::invokeMethod(_hyperion, "setEffect", Qt::QueuedConnection, Q_ARG(QString, dat.effectName), Q_ARG(QJsonObject, dat.args), Q_ARG(int, dat.priority), Q_ARG(int, dat.duration), Q_ARG(QString, dat.pythonScript), Q_ARG(QString, dat.origin), Q_ARG(QString, dat.data));
    }
    else
    {
        QMetaObject::invokeMethod(_hyperion, "setEffect", Qt::QueuedConnection, Q_ARG(QString, dat.effectName), Q_ARG(int, dat.priority), Q_ARG(int, dat.duration), Q_ARG(QString, dat.origin));
        //QMetaObject::invokeMethod(ApiSync::getInstance(), "setEffect", Qt::QueuedConnection, Q_ARG(QObject *, _hyperion), Q_ARG(QString, dat.effectName), Q_ARG(int, dat.priority), Q_ARG(int, dat.duration), Q_ARG(QString, dat.origin), Q_ARG(hyperion::Components, callerComp));
    }
}

void API::setSourceAutoSelect(const bool state, const hyperion::Components &callerComp)
{
    QMetaObject::invokeMethod(_hyperion, "setSourceAutoSelect", Qt::QueuedConnection, Q_ARG(bool, state));
    //QMetaObject::invokeMethod(ApiSync::getInstance(), "setSourceAutoSelect", Qt::QueuedConnection, Q_ARG(QObject *, _hyperion), Q_ARG(bool, state), Q_ARG(hyperion::Components, callerComp));
}

void API::setVisiblePriority(const int &priority, const hyperion::Components &callerComp)
{
    QMetaObject::invokeMethod(_hyperion, "setVisiblePriority", Qt::QueuedConnection, Q_ARG(int, priority));
    //QMetaObject::invokeMethod(ApiSync::getInstance(), "setVisiblePriority", Qt::QueuedConnection, Q_ARG(QObject *, _hyperion), Q_ARG(int, priority), Q_ARG(hyperion::Components, callerComp));
}

void API::registerInput(const int &priority, const hyperion::Components &component, const QString &origin, const QString &owner, const hyperion::Components &callerComp)
{
    if (_activeRegisters.count(priority))
        _activeRegisters.erase(priority);

    _activeRegisters.insert({priority, registerData{component, origin, owner, callerComp}});

    QMetaObject::invokeMethod(_hyperion, "registerInput", Qt::QueuedConnection, Q_ARG(int, priority), Q_ARG(hyperion::Components, component), Q_ARG(QString, origin), Q_ARG(QString, owner));
    //QMetaObject::invokeMethod(ApiSync::getInstance(), "registerInput", Qt::QueuedConnection, Q_ARG(QObject *, _hyperion), Q_ARG(int, priority), Q_ARG(hyperion::Components, component), Q_ARG(QString, origin), Q_ARG(QString, owner), Q_ARG(hyperion::Components, callerComp));
}

void API::unregisterInput(const int &priority)
{
    if (_activeRegisters.count(priority))
        _activeRegisters.erase(priority);
}

bool API::setHyperionInstance(const quint8 &inst)
{
    if (_currInstanceIndex == inst)
        return true;
    bool isRunning;
    QMetaObject::invokeMethod(_instanceManager, "IsInstanceRunning", Qt::DirectConnection, Q_RETURN_ARG(bool, isRunning), Q_ARG(quint8, inst));
    if (!isRunning)
        return false;

    disconnect(_hyperion, 0, this, 0);
    QMetaObject::invokeMethod(_instanceManager, "getHyperionInstance", Qt::DirectConnection, Q_RETURN_ARG(Hyperion *, _hyperion), Q_ARG(quint8, inst));
    _currInstanceIndex = inst;
    return true;
}

std::map<hyperion::Components, bool> API::getAllComponents()
{
    std::map<hyperion::Components, bool> comps;
    //QMetaObject::invokeMethod(_hyperion, "getAllComponents", Qt::BlockingQueuedConnection, Q_RETURN_ARG(std::map<hyperion::Components, bool>, comps));
    return comps;
}

bool API::isHyperionEnabled()
{
    int res;
    QMetaObject::invokeMethod(_hyperion, "isComponentEnabled", Qt::BlockingQueuedConnection, Q_RETURN_ARG(int, res), Q_ARG(hyperion::Components, hyperion::COMP_ALL));
    return res > 0;
}

QVector<QVariantMap> API::getAllInstanceData(void)
{
    QVector<QVariantMap> vec;
    QMetaObject::invokeMethod(_instanceManager, "getInstanceData", Qt::DirectConnection, Q_RETURN_ARG(QVector<QVariantMap>, vec));
    return vec;
}

void API::startInstance(const quint8 &index)
{
    QMetaObject::invokeMethod(_instanceManager, "startInstance", Qt::QueuedConnection, Q_ARG(quint8, index));
}

void API::stopInstance(const quint8 &index)
{
    QMetaObject::invokeMethod(_instanceManager, "stopInstance", Qt::QueuedConnection, Q_ARG(quint8, index));
}

void API::requestActiveRegister(QObject *callerInstance)
{
    // TODO FIXME
    //if (_activeRegisters.size())
    //   QMetaObject::invokeMethod(ApiSync::getInstance(), "answerActiveRegister", Qt::QueuedConnection, Q_ARG(QObject *, callerInstance), Q_ARG(MapRegister, _activeRegisters));
}

bool API::deleteInstance(const quint8 &index, QString &replyMsg)
{
    if (_adminAuthorized)
    {
        QMetaObject::invokeMethod(_instanceManager, "deleteInstance", Qt::QueuedConnection, Q_ARG(quint8, index));
        return true;
    }
    replyMsg = NO_AUTH;
    return false;
}

QString API::createInstance(const QString &name)
{
    if (_adminAuthorized)
    {
        bool success;
        QMetaObject::invokeMethod(_instanceManager, "createInstance", Qt::DirectConnection, Q_RETURN_ARG(bool, success), Q_ARG(QString, name));
        if (!success)
            return QString("Instance name '%1' is already in use").arg(name);

        return "";
    }
    return NO_AUTH;
}

QString API::setInstanceName(const quint8 &index, const QString &name)
{
    if (_adminAuthorized)
    {
        QMetaObject::invokeMethod(_instanceManager, "saveName", Qt::QueuedConnection, Q_ARG(quint8, index), Q_ARG(QString, name));
        return "";
    }
    return NO_AUTH;
}

QString API::deleteEffect(const QString &name)
{
    if (_adminAuthorized)
    {
        QString res;
        QMetaObject::invokeMethod(_hyperion, "deleteEffect", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QString, res), Q_ARG(QString, name));
        return res;
    }
    return NO_AUTH;
}

QString API::saveEffect(const QJsonObject &data)
{
    if (_adminAuthorized)
    {
        QString res;
        QMetaObject::invokeMethod(_hyperion, "saveEffect", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QString, res), Q_ARG(QJsonObject, data));
        return res;
    }
    return NO_AUTH;
}

void API::saveSettings(const QJsonObject &data)
{
    if (!_adminAuthorized)
        return;
    QMetaObject::invokeMethod(_hyperion, "saveSettings", Qt::QueuedConnection, Q_ARG(QJsonObject, data), Q_ARG(bool, true));
}

bool API::updateHyperionPassword(const QString &password, const QString &newPassword)
{
    if (!_adminAuthorized)
        return false;
    bool res;
    QMetaObject::invokeMethod(_authManager, "updateUserPassword", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, res), Q_ARG(QString, "Hyperion"), Q_ARG(QString, password), Q_ARG(QString, newPassword));
    return res;
}

QString API::createToken(const QString &comment, AuthManager::AuthDefinition &def)
{
    if (!_adminAuthorized)
        return NO_AUTH;
    if (comment.isEmpty())
        return "comment is empty";
    QMetaObject::invokeMethod(_authManager, "createToken", Qt::BlockingQueuedConnection, Q_RETURN_ARG(AuthManager::AuthDefinition, def), Q_ARG(QString, comment));
    return "";
}

QString API::renameToken(const QString &id, const QString &comment)
{
    if (!_adminAuthorized)
        return NO_AUTH;
    if (comment.isEmpty() || id.isEmpty())
        return "Empty comment or id";

    QMetaObject::invokeMethod(_authManager, "renameToken", Qt::QueuedConnection, Q_ARG(QString, id), Q_ARG(QString, comment));
    return "";
}

QString API::deleteToken(const QString &id)
{
    if (!_adminAuthorized)
        return NO_AUTH;
    if (id.isEmpty())
        return "Empty id";

    QMetaObject::invokeMethod(_authManager, "deleteToken", Qt::QueuedConnection, Q_ARG(QString, id));
    return "";
}

void API::setNewTokenRequest(const QString &comment, const QString &id)
{
    QMetaObject::invokeMethod(_authManager, "setNewTokenRequest", Qt::QueuedConnection, Q_ARG(QObject *, this), Q_ARG(QString, comment), Q_ARG(QString, id));
}

void API::cancelNewTokenRequest(const QString &comment, const QString &id)
{
    QMetaObject::invokeMethod(_authManager, "cancelNewTokenRequest", Qt::QueuedConnection, Q_ARG(QObject *, this), Q_ARG(QString, comment), Q_ARG(QString, id));
}

bool API::handlePendingTokenRequest(const QString &id, const bool accept)
{
    if (!_adminAuthorized)
        return false;
    QMetaObject::invokeMethod(_authManager, "handlePendingTokenRequest", Qt::QueuedConnection, Q_ARG(QString, id), Q_ARG(bool, accept));
    return true;
}

bool API::getTokenList(QVector<AuthManager::AuthDefinition> &def)
{
    if (!_adminAuthorized)
        return false;
    QMetaObject::invokeMethod(_authManager, "getTokenList", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QVector<AuthManager::AuthDefinition>, def));
    return true;
}

bool API::getPendingTokenRequests(QVector<AuthManager::AuthDefinition> &map)
{
    if (!_adminAuthorized)
        return false;
    QMetaObject::invokeMethod(_authManager, "getPendingRequests", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QVector<AuthManager::AuthDefinition>, map));
    return true;
}

bool API::isUserTokenAuthorized(const QString &userToken)
{
    bool res;
    QMetaObject::invokeMethod(_authManager, "isUserTokenAuthorized", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, res), Q_ARG(QString, "Hyperion"), Q_ARG(QString, userToken));
    if (res)
    {
        _authorized = true;
        _adminAuthorized = true;
        // Listen for ADMIN ACCESS protected signals
        connect(_authManager, &AuthManager::newPendingTokenRequest, this, &API::onPendingTokenRequest, Qt::UniqueConnection);
    }
    return res;
}

bool API::getUserToken(QString &userToken)
{
    if (!_adminAuthorized)
        return false;
    QMetaObject::invokeMethod(_authManager, "getUserToken", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QString, userToken));
    return true;
}

bool API::isTokenAuthorized(const QString &token)
{
    bool res;
    QMetaObject::invokeMethod(_authManager, "isTokenAuthorized", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, res), Q_ARG(QString, token));
    if (res)
        _authorized = true;

    return res;
}

bool API::isUserAuthorized(const QString &password)
{
    bool res;
    QMetaObject::invokeMethod(_authManager, "isUserAuthorized", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, res), Q_ARG(QString, "Hyperion"), Q_ARG(QString, password));
    if (res)
    {
        _authorized = true;
        _adminAuthorized = true;
        // Listen for ADMIN ACCESS protected signals
        connect(_authManager, &AuthManager::newPendingTokenRequest, this, &API::onPendingTokenRequest, Qt::UniqueConnection);
    }
    return res;
}

bool API::hasHyperionDefaultPw()
{
    bool res;
    QMetaObject::invokeMethod(_authManager, "isUserAuthorized", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, res), Q_ARG(QString, "Hyperion"), Q_ARG(QString, "hyperion"));
    return res;
}

void API::logout()
{
    _authorized = false;
    _adminAuthorized = false;
    // Stop listenig for ADMIN ACCESS protected signals
    disconnect(_authManager, &AuthManager::newPendingTokenRequest, this, &API::onPendingTokenRequest);
    stopDataConnectionss();
}

void API::checkTokenResponse(const bool &success, QObject *caller, const QString &token, const QString &comment, const QString &id)
{
    if (this == caller)
        emit onTokenResponse(success, token, comment, id);
}

void API::stopDataConnectionss()
{
}
