// Qt includes
#include <QCryptographicHash>
#include <QtEndian>

// project includes
#include "JsonClientConnection.h"

const quint64 FRAME_SIZE_IN_BYTES = 512 * 512 * 2;  //maximum size of a frame when sending a message

JsonClientConnection::JsonClientConnection(QTcpSocket *socket)
	: QObject()
	, _socket(socket)
	, _hyperion(Hyperion::getInstance())
	, _receiveBuffer()
	, _webSocketHandshakeDone(false)
	, _log(Logger::getInstance("JSONCLIENTCONNECTION"))
	, _clientAddress(socket->peerAddress())
{
	// connect internal signals and slots
	connect(_socket, SIGNAL(disconnected()), this, SLOT(socketClosed()));
	connect(_socket, SIGNAL(readyRead()), this, SLOT(readData()));

	// create a new instance of JsonProcessor
	_jsonProcessor = new JsonProcessor(_clientAddress.toString(), _log);
	// get the callback messages from JsonProcessor and send it to the client
	connect(_jsonProcessor,SIGNAL(callbackMessage(QJsonObject)),this,SLOT(sendMessage(QJsonObject)));
}

JsonClientConnection::~JsonClientConnection()
{
	delete _socket;
	delete _jsonProcessor;
}

void JsonClientConnection::readData()
{
	_receiveBuffer += _socket->readAll();

	if (_webSocketHandshakeDone)
	{
		// websocket mode, data frame
		handleWebSocketFrame();
	}
	else
	{
		// might be a handshake request or raw socket data
		if(_receiveBuffer.contains("Upgrade: websocket"))
		{
			doWebSocketHandshake();
		} else
		{
			// raw socket data, handling as usual
			int bytes = _receiveBuffer.indexOf('\n') + 1;
			while(bytes > 0)
			{
				// create message string
				QString message(QByteArray(_receiveBuffer.data(), bytes));

				// remove message data from buffer
				_receiveBuffer = _receiveBuffer.mid(bytes);

				// handle message
				_jsonProcessor->handleMessage(message);

				// try too look up '\n' again
				bytes = _receiveBuffer.indexOf('\n') + 1;
			}
		}
	}
}

void JsonClientConnection::handleWebSocketFrame()
{
	if ((_receiveBuffer.at(0) & BHB0_FIN) == BHB0_FIN)
	{
		// final bit found, frame complete
		quint8 * maskKey = NULL;
		quint8 opCode = _receiveBuffer.at(0) & BHB0_OPCODE;
		bool isMasked = (_receiveBuffer.at(1) & BHB0_FIN) == BHB0_FIN;
		quint64 payloadLength = _receiveBuffer.at(1) & BHB1_PAYLOAD;
		quint32 index = 2;
//printf("%ld\n", payloadLength);
		switch (payloadLength)
		{
			case payload_size_code_16bit:
				payloadLength = ((_receiveBuffer.at(2) << 8) & 0xFF00) | (_receiveBuffer.at(3) & 0xFF);
				index += 2;
				break;
			case payload_size_code_64bit:
				payloadLength = 0;
				for (uint i=0; i < 8; i++)
				{
					payloadLength |= ((quint64)(_receiveBuffer.at(index+i) & 0xFF)) << (8*(7-i));
				}
				index += 8;
				break;
			default:
				break;
		}

		if (isMasked)
		{
			// if the data is masked we need to get the key for unmasking
			maskKey = new quint8[4];
			for (uint i=0; i < 4; i++)
			{
				maskKey[i] = _receiveBuffer.at(index + i);
			}
			index += 4;
		}

		// check the type of data frame
		switch (opCode)
		{
			case OPCODE::TEXT:
			{
				// frame contains text, extract it
				QByteArray result = _receiveBuffer.mid(index, payloadLength);
				_receiveBuffer.clear();

				// unmask data if necessary
				if (isMasked)
				{
					for (uint i=0; i < payloadLength; i++)
					{
						result[i] = (result[i] ^ maskKey[i % 4]);
					}
					if (maskKey != NULL)
					{
						delete[] maskKey;
						maskKey = NULL;
					}
				}

				_jsonProcessor->handleMessage(QString(result));
			}
			break;
		case OPCODE::CLOSE:
			{
				// close request, confirm
				quint8 close[] = {0x88, 0};
				_socket->write((const char*)close, 2);
				_socket->flush();
				_socket->close();
			}
			break;
		case OPCODE::PING:
			{
				// ping received, send pong
				quint8 pong[] = {OPCODE::PONG, 0};
				_socket->write((const char*)pong, 2);
				_socket->flush();
			}
			break;
		}
	}
	else
	{
		Error(_log, "Someone is sending very big messages over several frames... it's not supported yet");
		quint8 close[] = {0x88, 0};
		_socket->write((const char*)close, 2);
		_socket->flush();
		_socket->close();
	}
}

void JsonClientConnection::doWebSocketHandshake()
{
	// http header, might not be a very reliable check...
	Debug(_log, "Websocket handshake");

	// get the key to prepare an answer
	int start = _receiveBuffer.indexOf("Sec-WebSocket-Key") + 19;
	QByteArray value = _receiveBuffer.mid(start, _receiveBuffer.indexOf("\r\n", start) - start);
	_receiveBuffer.clear();

	// must be always appended
	value += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

	// generate sha1 hash
	QByteArray hash = QCryptographicHash::hash(value, QCryptographicHash::Sha1);
	QByteArray hashB64 = hash.toBase64();

	// prepare an answer
	QString data
	    = QString("HTTP/1.1 101 Switching Protocols\r\n")
		+ QString("Upgrade: websocket\r\n")
		+ QString("Connection: Upgrade\r\n")
		+ QString("Sec-WebSocket-Accept: ")
		+ QString(hashB64.data()) + "\r\n\r\n";

	_socket->write(QSTRING_CSTR(data), data.size());
	_socket->flush();

	// we are in WebSocket mode, data frames should follow next
	_webSocketHandshakeDone = true;
}

void JsonClientConnection::socketClosed()
{
	_webSocketHandshakeDone = false;
	emit connectionClosed(this);
}

QByteArray JsonClientConnection::getFrameHeader(quint8 opCode, quint64 payloadLength, bool lastFrame)
{
	QByteArray header;
	bool ok = payloadLength <= 0x7FFFFFFFFFFFFFFFULL;

	if (ok)
	{
		//FIN, RSV1-3, opcode (RSV-1, RSV-2 and RSV-3 are zero)
		quint8 byte = static_cast<quint8>((opCode & 0x0F) | (lastFrame ? 0x80 : 0x00));
		header.append(static_cast<char>(byte));

		byte = 0x00;
		if (payloadLength <= 125)
		{
			byte |= static_cast<quint8>(payloadLength);
			header.append(static_cast<char>(byte));
		}
		else if (payloadLength <= 0xFFFFU)
		{
			byte |= 126;
			header.append(static_cast<char>(byte));
			quint16 swapped = qToBigEndian<quint16>(static_cast<quint16>(payloadLength));
			header.append(static_cast<const char *>(static_cast<const void *>(&swapped)), 2);
		}
		else if (payloadLength <= 0x7FFFFFFFFFFFFFFFULL)
		{
			byte |= 127;
			header.append(static_cast<char>(byte));
			quint64 swapped = qToBigEndian<quint64>(payloadLength);
			header.append(static_cast<const char *>(static_cast<const void *>(&swapped)), 8);
		}
	}
	else
	{
		Error(_log, "JsonClientConnection::getHeader: payload too big!");
	}

	return header;
}

qint64 JsonClientConnection::sendMessage(QJsonObject message)
{
	QJsonDocument writer(message);
	QByteArray serializedReply = writer.toJson(QJsonDocument::Compact) + "\n";

	if (!_socket || (_socket->state() != QAbstractSocket::ConnectedState)) return 0;
	if (_webSocketHandshakeDone) return sendMessage_Websockets(serializedReply);

	return sendMessage_Raw(serializedReply);
}

qint64 JsonClientConnection::sendMessage_Raw(const char* data, quint64 size)
{
	return _socket->write(data, size);
}

qint64 JsonClientConnection::sendMessage_Raw(QByteArray data)
{
	return _socket->write(data.data(), data.size());
}

qint64 JsonClientConnection::sendMessage_Websockets(QByteArray &data)
{
	qint64 payloadWritten = 0;
	quint32 payloadSize   = data.size();
	const char * payload  = data.data();

	qint32 numFrames = payloadSize / FRAME_SIZE_IN_BYTES + ((quint64(payloadSize) % FRAME_SIZE_IN_BYTES) > 0 ? 1 : 0);

	for (int i = 0; i < numFrames; ++i)
	{
		const bool isLastFrame = (i == (numFrames - 1));

		quint64 position  = i * FRAME_SIZE_IN_BYTES;
		quint32 frameSize = (payloadSize-position >= FRAME_SIZE_IN_BYTES) ? FRAME_SIZE_IN_BYTES : (payloadSize-position);

		sendMessage_Raw(getFrameHeader(OPCODE::TEXT, frameSize, isLastFrame));
		qint64 written = sendMessage_Raw(payload+position,frameSize);
		if (written > 0)
		{
			payloadWritten += written;
		}
		else
		{
			_socket->flush();
			Error(_log, "Error writing bytes to socket: %s", QSTRING_CSTR(_socket->errorString()));
			break;
		}
	}

	if (payloadSize != payloadWritten)
	{
		Error(_log, "Error writing bytes to socket %d bytes from %d writte", payloadWritten, payloadSize);
		return -1;
	}
	return payloadWritten;
}
