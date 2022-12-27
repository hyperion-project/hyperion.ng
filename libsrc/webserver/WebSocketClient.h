#pragma once

#include <utils/Logger.h>
#include "WebSocketUtils.h"

class QTcpSocket;

class QtHttpRequest;
class Hyperion;
class JsonAPI;

class WebSocketClient : public QObject
{
	Q_OBJECT
public:
	WebSocketClient(QtHttpRequest* request, QTcpSocket* sock, bool localConnection, QObject* parent);

	struct WebSocketHeader
	{
		bool          fin;
		quint8        opCode;
		bool          masked;
		quint64       payloadLength;
		char          key[4];
	};

private:
	QTcpSocket* _socket;
	Logger* _log;
	Hyperion* _hyperion;
	JsonAPI* _jsonAPI;

	void getWsFrameHeader(WebSocketHeader* header);
	void sendClose(int status, const QString& reason = "");
	void handleBinaryMessage(QByteArray &data);
	qint64 sendMessage_Raw(const char* data, quint64 size);
	qint64 sendMessage_Raw(QByteArray &data);
	QByteArray makeFrameHeader(quint8 opCode, quint64 payloadLength, bool lastFrame);

	/// The buffer used for reading data from the socket
	QByteArray _receiveBuffer;

	/// buffer for websockets multi frame receive
	QByteArray _wsReceiveBuffer;
	quint8 _maskKey[4];

	bool _onContinuation = false;

	// true when data is missing for parsing
	bool _notEnoughData = false;

	// websocket header store
	WebSocketHeader _wsh;

	//opCode of first frame (in case of fragmented frames)
	quint8 _frameOpCode;

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

	static const quint64 FRAME_SIZE_IN_BYTES = 512 * 512 * 2;  //maximum size of a frame when sending a message

private slots:
	void handleWebSocketFrame();
	qint64 sendMessage(QJsonObject obj);
};
