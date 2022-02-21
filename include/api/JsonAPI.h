#pragma once

// parent class
#include <api/API.h>

// hyperion includes
#include <utils/Components.h>
#include <hyperion/Hyperion.h>
#include <hyperion/HyperionIManager.h>

// qt includes
#include <QJsonObject>
#include <QString>

class QTimer;
class JsonCB;
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
	JsonAPI(QString peerAddress, Logger *log, bool localConnection, QObject *parent, bool noListener = false);

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

public slots:
	///
	/// @brief Is called whenever the current Hyperion instance pushes new led raw values (if enabled)
	/// @param ledColors  The current led colors
	///
	void streamLedcolorsUpdate(const std::vector<ColorRgb> &ledColors);

	///
	/// @brief Push images whenever hyperion emits (if enabled)
	/// @param image  The current image
	///
	void setImage(const Image<ColorRgb> &image);

	///
	/// @brief Process and push new log messages from logger (if enabled)
	///
	void incommingLogMessage(const Logger::T_LOG_MESSAGE &);

private slots:
	///
	/// @brief Handle emits from API of a new Token request.
	/// @param  id      The id of the request
	/// @param  comment The comment which needs to be accepted
	///
	void newPendingTokenRequest(const QString &id, const QString &comment);

	///
	/// @brief Handle emits from AuthManager of accepted/denied/timeouts token request, just if QObject matches with this instance we are allowed to send response.
	/// @param  success If true the request was accepted else false and no token was created
	/// @param  token   The new token that is now valid
	/// @param  comment The comment that was part of the request
	/// @param  id      The id that was part of the request
	/// @param  tan     The tan that was part of the request
	///
	void handleTokenResponse(bool success, const QString &token, const QString &comment, const QString &id, const int &tan);

	///
	/// @brief Handle whenever the state of a instance (HyperionIManager) changes according to enum instanceState
	/// @param instaneState  A state from enum
	/// @param instance      The index of instance
	/// @param name          The name of the instance, just available with H_CREATED
	///
	void handleInstanceStateChange(InstanceState state, quint8 instance, const QString &name = QString());

signals:
	///
	/// Signal emits with the reply message provided with handleMessage()
	///
	void callbackMessage(QJsonObject);

	///
	/// Signal emits whenever a JSON-message should be forwarded
	///
	void forwardJsonMessage(QJsonObject);

private:
	// true if further callbacks are forbidden (http)
	bool _noListener;

	/// The peer address of the client
	QString _peerAddress;

	// The JsonCB instance which handles data subscription/notifications
	JsonCB *_jsonCB;

	// streaming buffers
	QJsonObject _streaming_leds_reply;
	QJsonObject _streaming_image_reply;
	QJsonObject _streaming_logging_reply;

	/// flag to determine state of log streaming
	bool _streaming_logging_activated;

	/// timer for led color refresh
	QTimer *_ledStreamTimer;

	/// led stream connection handle
	QMetaObject::Connection _ledStreamConnection;

	/// the current streaming led values
	std::vector<ColorRgb> _currentLedValues;

	///
	/// @brief Handle the switches of Hyperion instances
	/// @param instance the instance to switch
	/// @param forced  indicate if it was a forced switch by system
	/// @return true on success. false if not found
	///
	bool handleInstanceSwitch(quint8 instance = 0, bool forced = false);

	///
	/// Handle an incoming JSON Color message
	///
	/// @param message the incoming message
	///
	void handleColorCommand(const QJsonObject &message, const QString &command, int tan);

	///
	/// Handle an incoming JSON Image message
	///
	/// @param message the incoming message
	///
	void handleImageCommand(const QJsonObject &message, const QString &command, int tan);

#if defined(ENABLE_EFFECTENGINE)
	///
	/// Handle an incoming JSON Effect message
	///
	/// @param message the incoming message
	///
	void handleEffectCommand(const QJsonObject &message, const QString &command, int tan);

	///
	/// Handle an incoming JSON Effect message (Write JSON Effect)
	///
	/// @param message the incoming message
	///
	void handleCreateEffectCommand(const QJsonObject &message, const QString &command, int tan);

	///
	/// Handle an incoming JSON Effect message (Delete JSON Effect)
	///
	/// @param message the incoming message
	///
	void handleDeleteEffectCommand(const QJsonObject &message, const QString &command, int tan);
#endif

	///
	/// Handle an incoming JSON System info message
	///
	/// @param message the incoming message
	///
	void handleSysInfoCommand(const QJsonObject &message, const QString &command, int tan);

	///
	/// Handle an incoming JSON Server info message
	///
	/// @param message the incoming message
	///
	void handleServerInfoCommand(const QJsonObject &message, const QString &command, int tan);

	///
	/// Handle an incoming JSON Clear message
	///
	/// @param message the incoming message
	///
	void handleClearCommand(const QJsonObject &message, const QString &command, int tan);

	///
	/// Handle an incoming JSON Clearall message
	///
	/// @param message the incoming message
	///
	void handleClearallCommand(const QJsonObject &message, const QString &command, int tan);

	///
	/// Handle an incoming JSON Adjustment message
	///
	/// @param message the incoming message
	///
	void handleAdjustmentCommand(const QJsonObject &message, const QString &command, int tan);

	///
	/// Handle an incoming JSON SourceSelect message
	///
	/// @param message the incoming message
	///
	void handleSourceSelectCommand(const QJsonObject &message, const QString &command, int tan);

	/// Handle an incoming JSON GetConfig message and check subcommand
	///
	/// @param message the incoming message
	///
	void handleConfigCommand(const QJsonObject &message, const QString &command, int tan);

	/// Handle an incoming JSON GetSchema message from handleConfigCommand()
	///
	/// @param message the incoming message
	///
	void handleSchemaGetCommand(const QJsonObject &message, const QString &command, int tan);

	/// Handle an incoming JSON SetConfig message from handleConfigCommand()
	///
	/// @param message the incoming message
	///
	void handleConfigSetCommand(const QJsonObject &message, const QString &command, int tan);

	/// Handle an incoming JSON RestoreConfig message from handleConfigCommand()
	///
	/// @param message the incoming message
	///
	void handleConfigRestoreCommand(const QJsonObject &message, const QString &command, int tan);

	///
	/// Handle an incoming JSON Component State message
	///
	/// @param message the incoming message
	///
	void handleComponentStateCommand(const QJsonObject &message, const QString &command, int tan);

	/// Handle an incoming JSON Led Colors message
	///
	/// @param message the incoming message
	///
	void handleLedColorsCommand(const QJsonObject &message, const QString &command, int tan);

	/// Handle an incoming JSON Logging message
	///
	/// @param message the incoming message
	///
	void handleLoggingCommand(const QJsonObject &message, const QString &command, int tan);

	/// Handle an incoming JSON Processing message
	///
	/// @param message the incoming message
	///
	void handleProcessingCommand(const QJsonObject &message, const QString &command, int tan);

	/// Handle an incoming JSON VideoMode message
	///
	/// @param message the incoming message
	///
	void handleVideoModeCommand(const QJsonObject &message, const QString &command, int tan);

	/// Handle an incoming JSON plugin message
	///
	/// @param message the incoming message
	///
	void handleAuthorizeCommand(const QJsonObject &message, const QString &command, int tan);

	/// Handle an incoming JSON instance message
	///
	/// @param message the incoming message
	///
	void handleInstanceCommand(const QJsonObject &message, const QString &command, int tan);

	/// Handle an incoming JSON Led Device message
	///
	/// @param message the incoming message
	///
	void handleLedDeviceCommand(const QJsonObject &message, const QString &command, int tan);

	/// Handle an incoming JSON message regarding Input Sources (Grabbers)
	///
	/// @param message the incoming message
	///
	void handleInputSourceCommand(const QJsonObject& message, const QString& command, int tan);

	/// Handle an incoming JSON message to request remote hyperion servers providing a given hyperion service
	///
	/// @param message the incoming message
	///
	void handleServiceCommand(const QJsonObject &message, const QString &command, int tan);

	///
	/// Handle an incoming JSON message of unknown type
	///
	void handleNotImplemented(const QString &command, int tan);

	///
	/// Send a standard reply indicating success
	///
	void sendSuccessReply(const QString &command = "", int tan = 0);

	///
	/// Send a standard reply indicating success with data
	///
	void sendSuccessDataReply(const QJsonDocument &doc, const QString &command = "", int tan = 0);

	///
	/// Send an error message back to the client
	///
	/// @param error String describing the error
	///
	void sendErrorReply(const QString &error, const QString &command = "", int tan = 0);

	///
	/// @brief Kill all signal/slot connections to stop possible data emitter
	///
	void stopDataConnections();
};
