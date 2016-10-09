#pragma once

// stl includes
#include <string>

// Qt includes
#include <QByteArray>
#include <QTcpSocket>

// jsoncpp includes
#include <json/json.h>

// Hyperion includes
#include <hyperion/Hyperion.h>

// util includes
#include <utils/jsonschema/JsonSchemaChecker.h>
#include <utils/Logger.h>
#include <utils/Components.h>

class ImageProcessor;

///
/// The Connection object created by \a JsonServer when a new connection is establshed
///
class JsonClientConnection : public QObject
{
	Q_OBJECT

public:
	///
	/// Constructor
	/// @param socket The Socket object for this connection
	/// @param hyperion The Hyperion server
	///
	JsonClientConnection(QTcpSocket * socket);

	///
	/// Destructor
	///
	~JsonClientConnection();

public slots:
	void componentStateChanged(const hyperion::Components component, bool enable);
	void streamLedcolorsUpdate();

signals:
	///
	/// Signal which is emitted when the connection is being closed
	/// @param connection This connection object
	///
	void connectionClosed(JsonClientConnection * connection);

private slots:
	///
	/// Slot called when new data has arrived
	///
	void readData();

	///
	/// Slot called when this connection is being closed
	///
	void socketClosed();


private:
	///
	/// Handle an incoming JSON message
	///
	/// @param message the incoming message as string
	///
	void handleMessage(const std::string & message);

	///
	/// Handle an incoming JSON Color message
	///
	/// @param message the incoming message
	///
	void handleColorCommand(const Json::Value & message, const std::string &command, const int tan);

	///
	/// Handle an incoming JSON Image message
	///
	/// @param message the incoming message
	///
	void handleImageCommand(const Json::Value & message, const std::string &command, const int tan);

	///
	/// Handle an incoming JSON Effect message
	///
	/// @param message the incoming message
	///
	void handleEffectCommand(const Json::Value & message, const std::string &command, const int tan);

	///
	/// Handle an incoming JSON Server info message
	///
	/// @param message the incoming message
	///
	void handleServerInfoCommand(const Json::Value & message, const std::string &command, const int tan);

	///
	/// Handle an incoming JSON Clear message
	///
	/// @param message the incoming message
	///
	void handleClearCommand(const Json::Value & message, const std::string &command, const int tan);

	///
	/// Handle an incoming JSON Clearall message
	///
	/// @param message the incoming message
	///
	void handleClearallCommand(const Json::Value & message, const std::string &command, const int tan);

	///
	/// Handle an incoming JSON Transform message
	///
	/// @param message the incoming message
	///
	void handleTransformCommand(const Json::Value & message, const std::string &command, const int tan);

	///
	/// Handle an incoming JSON Adjustment message
	///
	/// @param message the incoming message
	///
	void handleAdjustmentCommand(const Json::Value & message, const std::string &command, const int tan);

	///
	/// Handle an incoming JSON SourceSelect message
	///
	/// @param message the incoming message
	///
	void handleSourceSelectCommand(const Json::Value & message, const std::string &command, const int tan);
	
	/// Handle an incoming JSON GetConfig message
	///
	/// @param message the incoming message
	///
	void handleConfigCommand(const Json::Value & message, const std::string &command, const int tan);

	/// Handle an incoming JSON GetConfig message
	///
	/// @param message the incoming message
	///
	void handleSchemaGetCommand(const Json::Value & message, const std::string &command, const int tan);

	/// Handle an incoming JSON GetConfig message
	///
	/// @param message the incoming message
	///
	void handleConfigGetCommand(const Json::Value & message, const std::string &command, const int tan);

	///
	/// Handle an incoming JSON SetConfig message
	///
	void handleConfigSetCommand(const Json::Value & message, const std::string &command, const int tan);
	
	///
	/// Handle an incoming JSON Component State message
	///
	/// @param message the incoming message
	///
	void handleComponentStateCommand(const Json::Value & message, const std::string &command, const int tan);

	/// Handle an incoming JSON Led Colors message
	///
	/// @param message the incoming message
	///
	void handleLedColorsCommand(const Json::Value & message, const std::string &command, const int tan);

	///
	/// Handle an incoming JSON message of unknown type
	///
	void handleNotImplemented();

	///
	/// Send a message to the connected client
	///
	/// @param message The JSON message to send
	///
	void sendMessage(const Json::Value & message);
	void sendMessage(const Json::Value & message, QTcpSocket * socket);

	///
	/// Send a standard reply indicating success
	///
	void sendSuccessReply(const std::string &command="", const int tan=0);

	///
	/// Send an error message back to the client
	///
	/// @param error String describing the error
	///
	void sendErrorReply(const std::string & error, const std::string &command="", const int tan=0);
	
	///
	/// Do handshake for a websocket connection
	///
	void doWebSocketHandshake();
	
	///
	/// Handle incoming websocket data frame
	///
	void handleWebSocketFrame();

	///
	/// forward json message
	///
	void forwardJsonMessage(const Json::Value & message);

private:
	///
	/// Check if a JSON messag is valid according to a given JSON schema
	///
	/// @param message JSON message which need to be checked
	/// @param schemaResource Qt Resource identifier with the JSON schema
	/// @param errors Output error message
	/// @param ignoreRequired ignore the required value in JSON schema
	///
	/// @return true if message conforms the given JSON schema
	///
	bool checkJson(const Json::Value & message, const QString &schemaResource, std::string & errors, bool ignoreRequired = false);

	/// The TCP-Socket that is connected tot the Json-client
	QTcpSocket * _socket;

	/// The processor for translating images to led-values
	ImageProcessor * _imageProcessor;

	/// Link to Hyperion for writing led-values to a priority channel
	Hyperion * _hyperion;

	/// The buffer used for reading data from the socket
	QByteArray _receiveBuffer;
	
	/// used for WebSocket detection and connection handling
	bool _webSocketHandshakeDone;

	/// The logger instance
	Logger * _log;

	/// Flag if forwarder is enabled
	bool _forwarder_enabled;
	
	/// 
	QTimer _timer_ledcolors;
	
	Json::Value _streaming_leds_reply;

};
