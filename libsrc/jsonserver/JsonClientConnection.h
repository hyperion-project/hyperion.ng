#pragma once

// stl includes
#include <string>

// Qt includes
#include <QByteArray>
#include <QTcpSocket>
#include <QMutex>

// Hyperion includes
#include <hyperion/Hyperion.h>

// util includes
#include <utils/jsonschema/QJsonSchemaChecker.h>
#include <utils/Logger.h>
#include <utils/Components.h>

class ImageProcessor;


/// Constants and utility functions related to WebSocket opcodes
/**
 * WebSocket Opcodes are 4 bits. See RFC6455 section 5.2.
 */
namespace OPCODE {
	enum value {
		CONTINUATION = 0x0,
		TEXT = 0x1,
		BINARY = 0x2,
		RSV3 = 0x3,
		RSV4 = 0x4,
		RSV5 = 0x5,
		RSV6 = 0x6,
		RSV7 = 0x7,
		CLOSE = 0x8,
		PING = 0x9,
		PONG = 0xA,
		CONTROL_RSVB = 0xB,
		CONTROL_RSVC = 0xC,
		CONTROL_RSVD = 0xD,
		CONTROL_RSVE = 0xE,
		CONTROL_RSVF = 0xF
	};

	/// Check if an opcode is reserved
	/**
	 * @param v The opcode to test.
	 * @return Whether or not the opcode is reserved.
	 */
	inline bool reserved(value v) {
		return (v >= RSV3 && v <= RSV7) || (v >= CONTROL_RSVB && v <= CONTROL_RSVF);
	}

	/// Check if an opcode is invalid
	/**
	 * Invalid opcodes are negative or require greater than 4 bits to store.
	 *
	 * @param v The opcode to test.
	 * @return Whether or not the opcode is invalid.
	 */
	inline bool invalid(value v) {
		return (v > 0xF || v < 0);
	}

	/// Check if an opcode is for a control frame
	/**
	 * @param v The opcode to test.
	 * @return Whether or not the opcode is a control opcode.
	*/
	inline bool is_control(value v) {
		return v >= 0x8;
	}
}

struct find_schema: std::unary_function<EffectSchema, bool>
{
	QString pyFile;
	find_schema(QString pyFile):pyFile(pyFile) { }
	bool operator()(EffectSchema const& schema) const
	{
		return schema.pyFile == pyFile;
	}
};

struct find_effect: std::unary_function<EffectDefinition, bool>
{
	QString effectName;
	find_effect(QString effectName) :effectName(effectName) { }
	bool operator()(EffectDefinition const& effectDefinition) const
	{
		return effectDefinition.name == effectName;
	}
};

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
	void incommingLogMessage(Logger::T_LOG_MESSAGE);
	void setImage(int priority, const Image<ColorRgb> & image, int duration_ms);

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
	void handleMessage(const QString & message);

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
	void handleEffectCommand(const QJsonObject & message, const QString &command, const int tan);

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
	/// Handle an incoming JSON Transform message
	///
	/// @param message the incoming message
	///
	void handleTransformCommand(const QJsonObject & message, const QString &command, const int tan);

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
	
	/// Handle an incoming JSON GetConfig message
	///
	/// @param message the incoming message
	///
	void handleConfigCommand(const QJsonObject & message, const QString &command, const int tan);

	/// Handle an incoming JSON GetConfig message
	///
	/// @param message the incoming message
	///
	void handleSchemaGetCommand(const QJsonObject & message, const QString &command, const int tan);

	/// Handle an incoming JSON GetConfig message
	///
	/// @param message the incoming message
	///
	void handleConfigGetCommand(const QJsonObject & message, const QString &command, const int tan);

	///
	/// Handle an incoming JSON SetConfig message
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

	///
	/// Handle an incoming JSON message of unknown type
	///
	void handleNotImplemented();

	///
	/// Send a message to the connected client
	///
	/// @param message The JSON message to send
	///
	void sendMessage(const QJsonObject & message);
	void sendMessage(const QJsonObject & message, QTcpSocket * socket);

	///
	/// Send a standard reply indicating success
	///
	void sendSuccessReply(const QString &command="", const int tan=0);

	///
	/// Send an error message back to the client
	///
	/// @param error String describing the error
	///
	void sendErrorReply(const QString & error, const QString &command="", const int tan=0);
	
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
	void forwardJsonMessage(const QJsonObject & message);

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
	bool checkJson(const QJsonObject & message, const QString &schemaResource, QString & errors, bool ignoreRequired = false);

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
	
	/// timer for ledcolors streaming
	QTimer _timer_ledcolors;

	// streaming buffers
	QJsonObject _streaming_leds_reply;
	QJsonObject _streaming_image_reply;
	QJsonObject _streaming_logging_reply;

	/// flag to determine state of log streaming
	bool _streaming_logging_activated;

	/// mutex to determine state of image streaming
	QMutex _image_stream_mutex;

	/// timeout for live video refresh
	volatile qint64 _image_stream_timeout;
	
	// masks for fields in the basic header
	static uint8_t const BHB0_OPCODE = 0x0F;
	static uint8_t const BHB0_RSV3 = 0x10;
	static uint8_t const BHB0_RSV2 = 0x20;
	static uint8_t const BHB0_RSV1 = 0x40;
	static uint8_t const BHB0_FIN = 0x80;

	static uint8_t const BHB1_PAYLOAD = 0x7F;
	static uint8_t const BHB1_MASK = 0x80;

	static uint8_t const payload_size_code_16bit = 0x7E; // 126
	static uint8_t const payload_size_code_64bit = 0x7F; // 127
};
