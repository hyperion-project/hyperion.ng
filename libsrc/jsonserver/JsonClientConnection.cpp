// Qt includes
#include <QCryptographicHash>
#include <QtEndian>
#include <unistd.h>

// project includes
#include "JsonClientConnection.h"

const quint64 FRAME_SIZE_IN_BYTES = 512 * 512 * 2;  //maximum size of a frame when sending a message

JsonClientConnection::JsonClientConnection(QTcpSocket *socket)
	: QObject()
	, _socket(socket)
	, _hyperion(Hyperion::getInstance())
	, _receiveBuffer()
	, _webSocketHandshakeDone(false)
	, _onContinuation(false)
	, _log(Logger::getInstance("JSONCLIENTCONNECTION"))
	, _notEnoughData(false)
	, _clientAddress(socket->peerAddress())
	, _connectionMode(CON_MODE::INIT)
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
	switch(_connectionMode)
	{
		case CON_MODE::INIT:
			_receiveBuffer = _socket->readAll(); // initial read to determine connection
			_connectionMode = (_receiveBuffer.contains("Upgrade: websocket")) ? CON_MODE::WEBSOCKET : CON_MODE::RAW;

			// init websockets
			if (_connectionMode == CON_MODE::WEBSOCKET)
			{
				doWebSocketHandshake();
				break;
			}
			// if no ws, hand over the data to raw handling

		case CON_MODE::RAW:
			handleRawJsonData();
			break;

		case CON_MODE::WEBSOCKET:
			handleWebSocketFrame();
			break;
	}
}


void JsonClientConnection::handleRawJsonData()
{
	_receiveBuffer += _socket->readAll();
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

void JsonClientConnection::getWsFrameHeader(WebSocketHeader* header)
{
	char fin_rsv_opcode, mask_length;
	_socket->getChar(&fin_rsv_opcode);
	_socket->getChar(&mask_length);

	header->fin    = (fin_rsv_opcode & BHB0_FIN) == BHB0_FIN;
	header->opCode = fin_rsv_opcode  & BHB0_OPCODE;
	header->masked = (mask_length & BHB1_MASK) == BHB1_MASK;
	header->payloadLength = mask_length  & BHB1_PAYLOAD;

	// get size of payload
	switch (header->payloadLength)
	{
		case payload_size_code_16bit:
		{
			QByteArray buf = _socket->read(2);
			header->payloadLength = ((buf.at(0) << 8) & 0xFF00) | (buf.at(1) & 0xFF);
		}
		break;

		case payload_size_code_64bit:
		{
			QByteArray buf = _socket->read(8);
			header->payloadLength = 0;
			for (uint i=0; i < 8; i++)
			{
				header->payloadLength |= ((quint64)(buf.at(i) & 0xFF)) << (8*(7-i));
			}
		}
		break;
	}

	// if the data is masked we need to get the key for unmasking
	if (header->masked)
	{
		_socket->read(header->key, 4);
	}
}


void JsonClientConnection::handleWebSocketFrame()
{
	//printf("frame\n");

	// we are on no continious reading from socket from call before
	if (!_notEnoughData)
	{
		getWsFrameHeader(&_wsh);
	}

	if(_socket->bytesAvailable() < (qint64)_wsh.payloadLength)
	{
		//printf("not enough data %llu %llu\n", _socket->bytesAvailable(),  _wsh.payloadLength);
		_notEnoughData=true;
		return;
	}
	_notEnoughData = false;

	QByteArray buf = _socket->read(_wsh.payloadLength);
	//printf("opcode %x payload bytes %llu avail: %llu\n", _wsh.opCode, _wsh.payloadLength, _socket->bytesAvailable());

	if (OPCODE::invalid((OPCODE::value)_wsh.opCode))
	{
		sendClose(CLOSECODE::INV_TYPE, "invalid opcode");
		return;
	}

	// check the type of data frame
	bool isContinuation=false;
	switch (_wsh.opCode)
	{
		case OPCODE::CONTINUATION:
			isContinuation = true;
			// no break here, just jump over to opcode text

		case OPCODE::BINARY:
		case OPCODE::TEXT:
		{
			// check for protocal violations
			if (_onContinuation && !isContinuation)
			{
				sendClose(CLOSECODE::VIOLATION, "protocol violation, somebody sends frames in between continued frames");
				return;
			}

			if (!_wsh.masked && _wsh.opCode == OPCODE::TEXT)
			{
				sendClose(CLOSECODE::VIOLATION, "protocol violation, unmasked text frames not allowed");
				return;
			}

			// unmask data
			for (int i=0; i < buf.size(); i++)
			{
				buf[i] = buf[i] ^ _wsh.key[i % 4];
			}

			_onContinuation = !_wsh.fin || isContinuation;

			// frame contains text, extract it, append data if this is a continuation
			if (_wsh.fin && ! isContinuation) // one frame
			{
				_wsReceiveBuffer.clear();
			}
			_wsReceiveBuffer.append(buf);

			// this is the final frame, decode and handle data
			if (_wsh.fin)
			{
				_onContinuation = false;
				if (_wsh.opCode == OPCODE::TEXT)
				{
					_jsonProcessor->handleMessage(QString(_wsReceiveBuffer));
				}
				else
				{
					handleBinaryMessage(_wsReceiveBuffer);
				}
				_wsReceiveBuffer.clear();
			}
		}
		break;

		case OPCODE::CLOSE:
			{
				sendClose(CLOSECODE::NORMAL);
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

		case OPCODE::PONG:
			{
				Error(_log, "pong recievied, protocol violation!");
			}

		default:
			Warning(_log, "strange %d\n%s\n",  _wsh.opCode, QSTRING_CSTR(QString(buf)));
	}
}

/// See http://tools.ietf.org/html/rfc6455#section-5.2 for more information
void JsonClientConnection::sendClose(int status, QString reason)
{
	Info(_log, "send close: %d %s", status, QSTRING_CSTR(reason));
	ErrorIf(!reason.isEmpty(), _log, QSTRING_CSTR(reason));
	_receiveBuffer.clear();
	QByteArray sendBuffer;

	sendBuffer.append(136+(status-1000));
	int length = reason.size();
	if(length >= 126)
	{
		sendBuffer.append( (length > 0xffff) ? 127 : 126);
		int num_bytes = (length > 0xffff) ? 8 : 2;

		for(int c = num_bytes - 1; c != -1; c--)
		{
			sendBuffer.append( quint8((static_cast<unsigned long long>(length) >> (8 * c)) % 256));
		}
	}
	else
	{
		sendBuffer.append(quint8(length));
	}

	sendBuffer.append(reason);

	_socket->write(sendBuffer);
	_socket->flush();
	_socket->close();
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
	QByteArray hash = QCryptographicHash::hash(value, QCryptographicHash::Sha1).toBase64();

	// prepare an answer
	QString data
	    = QString("HTTP/1.1 101 Switching Protocols\r\n")
		+ QString("Upgrade: websocket\r\n")
		+ QString("Connection: Upgrade\r\n")
		+ QString("Sec-WebSocket-Accept: ")+QString(hash.data()) + "\r\n\r\n";

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

QByteArray JsonClientConnection::makeFrameHeader(quint8 opCode, quint64 payloadLength, bool lastFrame)
{
	QByteArray header;

	if (payloadLength <= 0x7FFFFFFFFFFFFFFFULL)
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
		else
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

qint64 JsonClientConnection::sendMessage_Raw(QByteArray &data)
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

		QByteArray buf = makeFrameHeader(OPCODE::TEXT, frameSize, isLastFrame);
		sendMessage_Raw(buf);

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

void JsonClientConnection::handleBinaryMessage(QByteArray &data)
{
	uint8_t  priority   = data.at(0);
	unsigned duration_s = data.at(1);
	unsigned imgSize    = data.size() - 4;
	unsigned width      = ((data.at(2) << 8) & 0xFF00) | (data.at(3) & 0xFF);
	unsigned height     =  imgSize / width;

	if ( ! (imgSize) % width)
	{
		Error(_log, "data size is not multiple of width");
		return;
	}

	Image<ColorRgb> image;
	image.resize(width, height);

	memcpy(image.memptr(), data.data()+4, imgSize);
	_hyperion->setImage(priority, image, duration_s*1000);
}
