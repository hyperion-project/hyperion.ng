#pragma once

// hyperion includes
#include <utils/Logger.h>
#include <utils/Components.h>
#include <hyperion/Hyperion.h>
#include <hyperion/HyperionIManager.h>

// qt includes
#include <QJsonObject>
#include <QString>

class QTimer;
class JsonCB;
class AuthManager;

class JsonAPI : public QObject
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
	JsonAPI(QString peerAddress, Logger* log, const bool& localConnection, QObject* parent, bool noListener = false);

	///
	/// Handle an incoming JSON message
	///
	/// @param message the incoming message as string
	///
	void handleMessage(const QString & message, const QString& httpAuthHeader = "");

	///
	/// @brief Initialization steps
	///
	void initialize(void);

public slots:
	///
	/// @brief Is called whenever the current Hyperion instance pushes new led raw values (if enabled)
	/// @param ledColors  The current led colors
	///
	void streamLedcolorsUpdate(const std::vector<ColorRgb>& ledColors);

	///
	/// @brief Push images whenever hyperion emits (if enabled)
	/// @param image  The current image
	///
	void setImage(const Image<ColorRgb> & image);

	///
	/// @brief Process and push new log messages from logger (if enabled)
	///
	void incommingLogMessage(const Logger::T_LOG_MESSAGE&);

private slots:
	///
	/// @brief Handle emits from AuthManager of new request, just _userAuthorized sessions are allowed to handle them
	/// @param id       The id of the request
	/// @param  The comment which needs to be accepted
	///
	void handlePendingTokenRequest(const QString& id, const QString& comment);

	///
	/// @brief Handle emits from AuthManager of accepted/denied/timeouts token request, just if QObject matches with this instance we are allowed to send response.
	/// @param  success If true the request was accepted else false and no token was created
	/// @param  caller  The origin caller instance who requested this token
	/// @param  token   The new token that is now valid
	/// @param  comment The comment that was part of the request
	/// @param  id      The id that was part of the request
	///
	void handleTokenResponse(const bool& success, QObject* caller, const QString& token, const QString& comment, const QString& id);

	///
	/// @brief Handle whenever the state of a instance (HyperionIManager) changes according to enum instanceState
	/// @param instaneState  A state from enum
	/// @param instance      The index of instance
	/// @param name          The name of the instance, just available with H_CREATED
	///
	void handleInstanceStateChange(const instanceState& state, const quint8& instance, const QString& name = QString());

signals:
	///
	/// Signal emits with the reply message provided with handleMessage()
	///
	void callbackMessage(QJsonObject);

	///
	/// Signal emits whenever a jsonmessage should be forwarded
	///
	void forwardJsonMessage(QJsonObject);

	///
	/// @brief The API might decide to block connections for security reasons, this emitter should close the socket
	///
	void forceClose();

private:
	/// Auth management pointer
	AuthManager* _authManager;

	/// Reflect auth status of this client
	bool _authorized;
	bool _userAuthorized;

	/// Reflect auth required
	bool _apiAuthRequired;

	// true if further callbacks are forbidden (http)
	bool _noListener;

	/// The peer address of the client
	QString _peerAddress;

	/// Log instance
	Logger* _log;

	/// Is this a local connection
	bool _localConnection;

	/// Hyperion instance manager
	HyperionIManager* _instanceManager;

	/// Hyperion instance
	Hyperion* _hyperion;

	// The JsonCB instance which handles data subscription/notifications
	JsonCB* _jsonCB;

	// streaming buffers
	QJsonObject _streaming_leds_reply;
	QJsonObject _streaming_image_reply;
	QJsonObject _streaming_logging_reply;

	/// flag to determine state of log streaming
	bool _streaming_logging_activated;

	/// timer for live video refresh
	QTimer* _imageStreamTimer;

	/// image stream connection handle
	QMetaObject::Connection _imageStreamConnection;

	/// the current streaming image
	Image<ColorRgb> _currentImage;

	/// timer for led color refresh
	QTimer* _ledStreamTimer;

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
	bool handleInstanceSwitch(const quint8& instance = 0, const bool& forced = false);

	///
	/// Handle an incoming JSON Color message
	///
	/// @param message the incoming message
	///
	void handleColorCommand(const QJsonObject & message, const QString &command, const int tan);

	///
	/// Handle an incoming JSON Image message
	///
	/// @param message the incoming message
	///
	void handleImageCommand(const QJsonObject & message, const QString &command, const int tan);

	///
	/// Handle an incoming JSON Effect message
	///
	/// @param message the incoming message
	///
	void handleEffectCommand(const QJsonObject &message, const QString &command, const int tan);

	///
	/// Handle an incoming JSON Effect message (Write JSON Effect)
	///
	/// @param message the incoming message
	///
	void handleCreateEffectCommand(const QJsonObject & message, const QString &command, const int tan);

	///
	/// Handle an incoming JSON Effect message (Delete JSON Effect)
	///
	/// @param message the incoming message
	///
	void handleDeleteEffectCommand(const QJsonObject & message, const QString &command, const int tan);

	///
	/// Handle an incoming JSON System info message
	///
	/// @param message the incoming message
	///
	void handleSysInfoCommand(const QJsonObject & message, const QString &command, const int tan);

	///
	/// Handle an incoming JSON Server info message
	///
	/// @param message the incoming message
	///
	void handleServerInfoCommand(const QJsonObject & message, const QString &command, const int tan);

	///
	/// Handle an incoming JSON Clear message
	///
	/// @param message the incoming message
	///
	void handleClearCommand(const QJsonObject & message, const QString &command, const int tan);

	///
	/// Handle an incoming JSON Clearall message
	///
	/// @param message the incoming message
	///
	void handleClearallCommand(const QJsonObject & message, const QString &command, const int tan);

	///
	/// Handle an incoming JSON Adjustment message
	///
	/// @param message the incoming message
	///
	void handleAdjustmentCommand(const QJsonObject & message, const QString &command, const int tan);

	///
	/// Handle an incoming JSON SourceSelect message
	///
	/// @param message the incoming message
	///
	void handleSourceSelectCommand(const QJsonObject & message, const QString &command, const int tan);

	/// Handle an incoming JSON GetConfig message and check subcommand
	///
	/// @param message the incoming message
	///
	void handleConfigCommand(const QJsonObject & message, const QString &command, const int tan);

	/// Handle an incoming JSON GetConfig message from handleConfigCommand()
	///
	/// @param message the incoming message
	///
	void handleSchemaGetCommand(const QJsonObject & message, const QString &command, const int tan);

	/// Handle an incoming JSON SetConfig message from handleConfigCommand()
	///
	/// @param message the incoming message
	///
	void handleConfigSetCommand(const QJsonObject & message, const QString &command, const int tan);

	///
	/// Handle an incoming JSON Component State message
	///
	/// @param message the incoming message
	///
	void handleComponentStateCommand(const QJsonObject & message, const QString &command, const int tan);

	/// Handle an incoming JSON Led Colors message
	///
	/// @param message the incoming message
	///
	void handleLedColorsCommand(const QJsonObject & message, const QString &command, const int tan);

	/// Handle an incoming JSON Logging message
	///
	/// @param message the incoming message
	///
	void handleLoggingCommand(const QJsonObject & message, const QString &command, const int tan);

	/// Handle an incoming JSON Proccessing message
	///
	/// @param message the incoming message
	///
	void handleProcessingCommand(const QJsonObject & message, const QString &command, const int tan);

	/// Handle an incoming JSON VideoMode message
	///
	/// @param message the incoming message
	///
	void handleVideoModeCommand(const QJsonObject & message, const QString &command, const int tan);

	/// Handle an incoming JSON plugin message
	///
	/// @param message the incoming message
	///
	void handleAuthorizeCommand(const QJsonObject & message, const QString &command, const int tan);

	///
	/// Handle HTTP on-the-fly token authorization
	/// @param command  The command
	/// @param tan      The tan
	/// @param token    The token to verify
	/// @return True on succcess else false (pushes failed client feedback)
	///
	bool handleHTTPAuth(const QString& command, const int& tan, const QString& token);

	/// Handle an incoming JSON instance message
	///
	/// @param message the incoming message
	///
	void handleInstanceCommand(const QJsonObject & message, const QString &command, const int tan);

	///
	/// Handle an incoming JSON message of unknown type
	///
	void handleNotImplemented();

	///
	/// Send a standard reply indicating success
	///
	void sendSuccessReply(const QString &command="", const int tan=0);

	///
	/// Send a standard reply indicating success with data
	///
	void sendSuccessDataReply(const QJsonDocument &doc, const QString &command="", const int &tan=0);

	///
	/// Send an error message back to the client
	///
	/// @param error String describing the error
	///
	void sendErrorReply(const QString & error, const QString &command="", const int tan=0);

	///
	/// @brief Kill all signal/slot connections to stop possible data emitter
	///
	void stopDataConnections(void);
};
