#pragma once

#include <map>

// Qt includes
#include <QByteArray>
#include <QTcpSocket>
#include <QHostAddress>
#include <QString>

// Hyperion includes
#include <hyperion/Hyperion.h>

// util includes
#include <utils/Logger.h>
#include <utils/JsonProcessor.h>

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

namespace CLOSECODE {
	enum value {
		NORMAL    = 1000,
		AWAY      = 1001,
		TERM      = 1002,
		INV_TYPE  = 1003,
		INV_DATA  = 1007,
		VIOLATION = 1008,
		BIG_MSG   = 1009,
		UNEXPECTED= 1011
	};
}

namespace CON_MODE {
	enum value {
		INIT      = 0,
		RAW       = 1,
		WEBSOCKET = 2
	};
}


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
	///
	JsonClientConnection(QTcpSocket * socket);

	///
	/// Destructor
	///
	~JsonClientConnection();

	struct WebSocketHeader
	{
		bool          fin;
		quint8 opCode;
		bool          masked;
		quint64       payloadLength;
		char          key[4];
	};
public slots:
	qint64 sendMessage(QJsonObject);

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
	/// new instance of JsonProcessor
	JsonProcessor * _jsonProcessor;

	///
	/// Do handshake for a websocket connection
	///
	void doWebSocketHandshake();

	///
	/// Handle incoming websocket data frame
	///
	void handleWebSocketFrame();

	///
	/// Handle incoming raw data frame
	///
	void handleRawJsonData();
	
	///
	/// create ws header from socket and decode it
	///
	QByteArray makeFrameHeader(quint8 opCode, quint64 payloadLength, bool lastFrame);

	///
	/// handle binary message
	///
	/// This function should be placed elsewhere .... 
	///
	void handleBinaryMessage(QByteArray &data);

	qint64 sendMessage_Raw(const char* data, quint64 size);
	qint64 sendMessage_Raw(QByteArray &data);
	qint64 sendMessage_Websockets(QByteArray &data);
	void sendClose(int status, QString reason = "");

	void getWsFrameHeader(WebSocketHeader* header);
	
	/// The TCP-Socket that is connected tot the Json-client
	QTcpSocket * _socket;

	/// Link to Hyperion for writing led-values to a priority channel
	Hyperion * _hyperion;

	/// The buffer used for reading data from the socket
	QByteArray _receiveBuffer;

	/// buffer for websockets multi frame receive
	QByteArray _wsReceiveBuffer;
	quint8 _maskKey[4];
	
	/// used for WebSocket detection and connection handling
	bool _webSocketHandshakeDone;

	bool _onContinuation;

	/// The logger instance
	Logger * _log;

	WebSocketHeader _wsh;
	bool _notEnoughData;

	/// address of client
	QHostAddress _clientAddress;

	CON_MODE::value _connectionMode;
	// masks for fields in the basic header
	static uint8_t const BHB0_OPCODE = 0x0F;
	static uint8_t const BHB0_RSV3   = 0x10;
	static uint8_t const BHB0_RSV2   = 0x20;
	static uint8_t const BHB0_RSV1   = 0x40;
	static uint8_t const BHB0_FIN    = 0x80;

	static uint8_t const BHB1_PAYLOAD = 0x7F;
	static uint8_t const BHB1_MASK    = 0x80;

	static uint8_t const payload_size_code_16bit = 0x7E; // 126
	static uint8_t const payload_size_code_64bit = 0x7F; // 127
};
