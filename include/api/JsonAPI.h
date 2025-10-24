#pragma once

// parent class
#include <api/API.h>
#include <api/JsonApiCommand.h>

#include <events/EventEnum.h>

// hyperion includes
#include <utils/Components.h>
#include <hyperion/Hyperion.h>
#include <hyperion/HyperionIManager.h>
#include <utils/RgbChannelAdjustment.h>

// qt includes
#include <QJsonObject>
#include <QString>
#include <QSharedPointer>
#include <QScopedPointer>

class QTimer;
class JsonCallbacks;
class AuthManager;

class JsonAPI : public API
{
	Q_OBJECT

public:
	///
	/// Constructor
	///
	/// @param peerAddress provide the Address of the peer
	/// @param log         The Logger class of the creator
	/// @param parent      Parent QObject
	/// @param localConnection True when the sender has origin home network
	/// @param noListener  if true, this instance won't listen for hyperion push events
	///
	JsonAPI(QString peerAddress, QSharedPointer<Logger>log, bool localConnection, QObject *parent, bool noListener = false);

	///
	/// Handle an incoming JSON message
	///
	/// @param message the incoming message as string
	///
	void handleMessage(const QString &message, const QString &httpAuthHeader = "");

	///
	/// @brief Initialization steps
	///
	void initialize();

	QSharedPointer<JsonCallbacks> getCallBack() const;

public slots:

private slots:
	///
	/// @brief Handle emits from API of a new Token request.
	/// @param  identifier The identifier of the request
	/// @param  comment The comment which needs to be accepted
	///
	void issueNewPendingTokenRequest(const QString &identifier, const QString &comment);

	///
	/// @brief Handle emits from AuthManager of accepted/denied/timeouts token request, just if QObject matches with this instance we are allowed to send response.
	/// @param  success If true the request was accepted else false and no token was created
	/// @param  token   The new token that is now valid
	/// @param  comment The comment that was part of the request
	/// @param  identifier The identifier that was part of the request
	/// @param  tan     The tan that was part of the request
	///
	void handleTokenResponse(bool success, const QString &token, const QString &comment, const QString &identifier, const int &tan);

	///
	/// @brief Handle whenever the state of a instance (HyperionIManager) changes according to enum instanceState
	/// @param instaneState  A state from enum
	/// @param instanceId    The index of instance
	/// @param name          The name of the instance, just available with H_CREATED
	///
	void handleInstanceStateChange(InstanceState state, quint8 instanceId, const QString &name = QString());

signals:
	///
	/// Signal emits with the reply message provided with handleMessage()
	///
	void callbackReady(QJsonObject);

	///
	/// Signal emits whenever a JSON-message should be forwarded
	///
	void forwardJsonMessage(const QJsonObject, quint8);

	///
	/// Signal emits whenever a hyperion event request for all instances should be forwarded
	///
	void signalEvent(Event event);

private:

	void handleCommand(const JsonApiCommand& cmd, const QJsonObject &message);
	void handleInstanceCommand(const JsonApiCommand& cmd, const QJsonObject &message);

	///
	/// @brief Handle the switches of Hyperion instances
	/// @param instanceId the instance to switch
	/// @param forced     indicate if it was a forced switch by system
	/// @return true on success. false if not found
	///
	bool handleInstanceSwitch(quint8 instanceId = 0, bool forced = false);

	///
	/// Handle an incoming JSON Color message
	///
	/// @param message the incoming message
	///
	void handleColorCommand(const QJsonObject& message, const JsonApiCommand& cmd);

	///
	/// Handle an incoming JSON Image message
	///
	/// @param message the incoming message
	///
	void handleImageCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	///
	/// Handle an incoming JSON Effect message
	///
	/// @param message the incoming message
	///
	void handleEffectCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	///
	/// Handle an incoming JSON Effect message (Write JSON Effect)
	///
	/// @param message the incoming message
	///
	void handleCreateEffectCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	///
	/// Handle an incoming JSON Effect message (Delete JSON Effect)
	///
	/// @param message the incoming message
	///
	void handleDeleteEffectCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	///
	/// Handle an incoming JSON System info message
	///
	/// @param message the incoming message
	///
	void handleSysInfoCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	///
	/// Handle an incoming JSON Server info message
	///
	/// @param message the incoming message
	///
	void handleServerInfoCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	///
	/// Handle an incoming JSON Clear message
	///
	/// @param message the incoming message
	///
	void handleClearCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	///
	/// Handle an incoming JSON Clearall message
	///
	/// @param message the incoming message
	///
	void handleClearallCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	///
	/// Handle an incoming JSON Adjustment message
	///
	/// @param message the incoming message
	///
	void handleAdjustmentCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	///
	/// Handle an incoming JSON SourceSelect message
	///
	/// @param message the incoming message
	///
	void handleSourceSelectCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	/// Handle an incoming JSON Config message and check subcommand
	///
	/// @param message the incoming message
	///
	void handleConfigCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	/// Handle an incoming JSON GetSchema message from handleConfigCommand()
	///
	/// @param message the incoming message
	///
	void handleSchemaGetCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	/// Handle an incoming JSON SetConfig message from handleConfigCommand()
	///
	/// @param message the incoming message
	///
	void handleConfigSetCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	/// Handle an incoming JSON GetConfig message from handleConfigCommand()
	///
	/// @param message the incoming message
	///
	void handleConfigGetCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	/// Handle an incoming JSON RestoreConfig message from handleConfigCommand()
	///
	/// @param message the incoming message
	///
	void handleConfigRestoreCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	///
	/// Handle an incoming JSON Component State message
	///
	/// @param message the incoming message
	///
	void handleComponentStateCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	/// Handle an incoming JSON Led Colors message
	///
	/// @param message the incoming message
	///
	void handleLedColorsCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	/// Handle an incoming JSON Logging message
	///
	/// @param message the incoming message
	///
	void handleLoggingCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	/// Handle an incoming JSON Processing message
	///
	/// @param message the incoming message
	///
	void handleProcessingCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	/// Handle an incoming JSON VideoMode message
	///
	/// @param message the incoming message
	///
	void handleVideoModeCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	/// Handle an incoming JSON plugin message
	///
	/// @param message the incoming message
	///
	void handleAuthorizeCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	/// Handle an incoming JSON instance message
	///
	/// @param message the incoming message
	///
	void handleInstanceCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	/// Handle an incoming JSON Led Device message
	///
	/// @param message the incoming message
	///
	void handleLedDeviceCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	/// Handle an incoming JSON message regarding Input Sources (Grabbers)
	///
	/// @param message the incoming message
	///
	void handleInputSourceCommand(const QJsonObject& message, const JsonApiCommand& cmd);

	/// Handle an incoming JSON message to request remote hyperion servers providing a given hyperion service
	///
	/// @param message the incoming message
	///
	void handleServiceCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	/// Handle an incoming JSON message for actions related to the overall Hyperion system
	///
	/// @param message the incoming message
	///
	void handleSystemCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	///  Handle an incoming data request message
	/// 
	/// @param message the incoming message
	/// 
	void handleInstanceDataCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	/// Handle an incoming JSON message to request the current image
	///
	/// @param message the incoming message
	///
	void handleGetImageSnapshotCommand(const QJsonObject &message, const JsonApiCommand& cmd);

	/// Handle an incoming JSON message to request the current led colors
	///
	/// @param message the incoming message
	///
	void handleGetLedSnapshotCommand(const QJsonObject &message, const JsonApiCommand& cmd);


	void applyColorAdjustments(const QJsonObject &adjustment, ColorAdjustment *colorAdjustment);
	void applyColorAdjustment(const QString &colorName, const QJsonObject &adjustment, RgbChannelAdjustment &rgbAdjustment);
	void applyGammaTransform(const QString &transformName, const QJsonObject &adjustment, RgbTransform &rgbTransform, char channel);

	void applyTransforms(const QJsonObject &adjustment, ColorAdjustment *colorAdjustment);
	template<typename T>
	void applyTransform(const QString &transformName, const QJsonObject &adjustment, T &transform, void (T::*setFunction)(bool));
	template<typename T>
	void applyTransform(const QString &transformName, const QJsonObject &adjustment, T &transform, void (T::*setFunction)(double));
	template<typename T>
	void applyTransform(const QString &transformName, const QJsonObject &adjustment, T &transform, void (T::*setFunction)(int));
	template<typename T>
	void applyTransform(const QString &transformName, const QJsonObject &adjustment, T &transform, void (T::*setFunction)(uint8_t));

	void handleTokenRequired(const JsonApiCommand& cmd);
	void handleAdminRequired(const JsonApiCommand& cmd);
	void handleNewPasswordRequired(const JsonApiCommand& cmd);
	void handleLogout(const JsonApiCommand& cmd);
	void handleNewPassword(const QJsonObject &message, const JsonApiCommand& cmd);
	void handleCreateToken(const QJsonObject &message, const JsonApiCommand& cmd);
	void handleRenameToken(const QJsonObject &message, const JsonApiCommand& cmd);
	void handleDeleteToken(const QJsonObject &message, const JsonApiCommand& cmd);
	void handleRequestToken(const QJsonObject &message, const JsonApiCommand& cmd);
	void handleGetPendingTokenRequests(const JsonApiCommand& cmd);
	void handleAnswerRequest(const QJsonObject &message, const JsonApiCommand& cmd);
	void handleGetTokenList(const JsonApiCommand& cmd);
	void handleLogin(const QJsonObject &message, const JsonApiCommand& cmd);

	void handleLedDeviceDiscover(LedDevice& ledDevice, const QJsonObject& message, const JsonApiCommand& cmd);
	void handleLedDeviceGetProperties(LedDevice& ledDevice, const QJsonObject& message, const JsonApiCommand& cmd);
	void handleLedDeviceIdentify(LedDevice& ledDevice, const QJsonObject& message, const JsonApiCommand& cmd);
	void handleLedDeviceAddAuthorization(LedDevice& ledDevice, const QJsonObject& message, const JsonApiCommand& cmd);

	QJsonObject getBasicCommandReply(bool success, const QString &command, int tan, InstanceCmd::Type isInstanceCmd) const;

	///
	/// Send a standard reply indicating success
	///
	void sendSuccessReply(const JsonApiCommand& cmd);

	///
	/// Send a standard reply indicating success
	///
	void sendSuccessReply(const QString &command = "", int tan = 0, InstanceCmd::Type isInstanceCmd = InstanceCmd::No);

	///
	/// Send a standard reply indicating success with data
	///
	void sendSuccessDataReply(const QJsonValue &infoData, const JsonApiCommand& cmd);

	///
	/// Send a standard reply indicating success with data
	///
	void sendSuccessDataReply(const QJsonValue &infoData, const QString &command = "", int tan = 0, InstanceCmd::Type isInstanceCmd = InstanceCmd::No);

	///
	/// Send a standard reply indicating success with data and error details
	///
	void sendSuccessDataReplyWithError(const QJsonValue &infoData, const JsonApiCommand& cmd, const QStringList& errorDetails = {});

	///
	/// Send a standard reply indicating success with data and error details
	///
	void sendSuccessDataReplyWithError(const QJsonValue &infoData, const QString &command = "", int tan = 0, const QStringList& errorDetails = {}, InstanceCmd::Type isInstanceCmd = InstanceCmd::No);

	///
	/// Send a message with data.
	/// Note: To be used as a new message and not as a response to a previous request.
	///
	void sendNewRequest(const QJsonValue &infoData, const JsonApiCommand& cmd);
	
	///
	/// Send a message with data
	/// Note: To be used as a new message and not as a response to a previous request.	
	///	
	void sendNewRequest(const QJsonValue &infoData, const QString &command, InstanceCmd::Type isInstanceCmd = InstanceCmd::No);

	///
	/// Send an error message back to the client
	///
	/// @param error String describing the error
	///
	void sendErrorReply(const QString &error, const JsonApiCommand& cmd);

	///
	/// Send an error message back to the client
	///
	/// @param error String describing the error
	/// @param errorDetails additional information detailing the error scenario
	///
	void sendErrorReply(const QString &error, const QStringList& errorDetails, const JsonApiCommand& cmd);

	///
	/// Send an error message back to the client
	///
	/// @param error String describing the error
	/// @param errorDetails additional information detailing the error scenario	
	///
	void sendErrorReply(const QString &error, const QStringList& errorDetails = {}, const QString &command = "", int tan = 0, InstanceCmd::Type isInstanceCmd = InstanceCmd::No);

	void sendNoAuthorization(const JsonApiCommand& cmd);

	///
	/// @brief Kill all signal/slot connections to stop possible data emitter
	///
	void stopDataConnections() override;

	static QString findCommand (const QString& jsonS);
	static int findTan (const QString& jsonString);

	// true if further callbacks are forbidden (http)
	bool _noListener;

	/// The peer address of the client
	QString _peerAddress;

	// The JsonCallbacks instance which handles data subscription/notifications
	QSharedPointer<JsonCallbacks> _jsonCB;

	bool _isServiceAvailable;
};
