// stl includes

// Qt includes
#include <QRgb>

// flatbuffer includes
#include <flatbufserver/FlatBufferConnection.h>

// flatbuffer FBS
#include "hyperion_reply_generated.h"
#include "hyperion_request_generated.h"

FlatBufferConnection::FlatBufferConnection(const QString& origin, const QHostAddress& host, int priority, bool skipReply, quint16 port)
	: _socket()
	, _origin(origin)
	, _priority(priority)
	, _host(host)
	, _port(port)
	, _log(Logger::getInstance("FLATBUFCONN"))
	, _builder(1024)
	, _isRegistered(false)
{
	connect(&_socket, &QTcpSocket::connected, this, &FlatBufferConnection::onConnected);
	connect(&_socket, &QTcpSocket::disconnected, this, &FlatBufferConnection::onDisconnected);
	if(!skipReply)
	{
		connect(&_socket, &QTcpSocket::readyRead, this, &FlatBufferConnection::readData, Qt::UniqueConnection);
	}

	// init connect
	connectToRemoteHost();

	_timer.setInterval(5000);
	connect(&_timer, &QTimer::timeout, this, &FlatBufferConnection::connectToRemoteHost);

	//Trigger the retry timer when connection dropped
	connect(this, &FlatBufferConnection::isDisconnected, &_timer, static_cast<void (QTimer::*)()>(&QTimer::start));
	_timer.start();
}

FlatBufferConnection::~FlatBufferConnection()
{
	_timer.stop();

	//Stop retrying on disconnect
	disconnect(this, &FlatBufferConnection::isDisconnected, &_timer, static_cast<void (QTimer::*)()>(&QTimer::start));

	Debug(_log, "Closing connection with host: %s, port [%u]", QSTRING_CSTR(_host.toString()), _port);
	_socket.close();
}

void FlatBufferConnection::connectToRemoteHost()
{
	if (_socket.state() == QAbstractSocket::UnconnectedState)
	{
		Info(_log, "Connecting to target host: %s, port [%u]", QSTRING_CSTR(_host.toString()), _port);
		_socket.connectToHost(_host, _port);
	}
}

void FlatBufferConnection::onDisconnected()
{
	_isRegistered = false,
	Info(_log, "Disconnected from target host: %s, port [%u]", QSTRING_CSTR(_host.toString()), _port);
	emit isDisconnected();
}


void FlatBufferConnection::onConnected()
{
	Info(_log, "Connected to target host: %s, port [%u]", QSTRING_CSTR(_host.toString()), _port);
	if (!isClientRegistered())
	{
		registerClient(_origin, _priority);
	}
}

void FlatBufferConnection::sendMessage(const uint8_t* data, size_t size)
{
	const uint8_t header[4] = {
		uint8_t((size >> 24) & 0xFF),
		uint8_t((size >> 16) & 0xFF),
		uint8_t((size >>  8) & 0xFF),
		uint8_t((size	   ) & 0xFF)};

	// write message
	_socket.write(reinterpret_cast<const char*>(header), sizeof(header));
	_socket.write(reinterpret_cast<const char *>(data), size);
	_socket.flush();
}

void FlatBufferConnection::registerClient(const QString& origin, int priority)
{
	Debug(_log, "Register client \"%s\" with to host: %s, port [%u]", QSTRING_CSTR(_origin), QSTRING_CSTR(_host.toString()), _port);

	_builder.Clear();
	auto registerReq = hyperionnet::CreateRegister(_builder, _builder.CreateString(QSTRING_CSTR(origin)), priority);
	auto req = hyperionnet::CreateRequest(_builder, hyperionnet::Command_Register, registerReq.Union());

	_builder.Finish(req);
	sendMessage(_builder.GetBufferPointer(), _builder.GetSize());
}

bool FlatBufferConnection::isClientRegistered()
{
	DebugIf(!_isRegistered,_log,"Client \"%s\" is not registered with target host: %s, port [%u]", QSTRING_CSTR(_origin), QSTRING_CSTR(_host.toString()), _port);
	return _isRegistered;
}

void FlatBufferConnection::setColor(const ColorRgb& color, int duration)
{
	if (!isClientRegistered()) return;

	_builder.Clear();
	auto colorReq = hyperionnet::CreateColor(_builder, (color.red << 16) | (color.green << 8) | color.blue, duration);
	auto req = hyperionnet::CreateRequest(_builder, hyperionnet::Command_Color, colorReq.Union());

	_builder.Finish(req);
	sendMessage(_builder.GetBufferPointer(), _builder.GetSize());
}

void FlatBufferConnection::setImage(const Image<ColorRgb> &image)
{
	if (!isClientRegistered()) return;

	const uint8_t* buffer = reinterpret_cast<const uint8_t*>(image.memptr());
	qsizetype bufferSize = image.size();

	// Convert the buffer into QByteArray
	QByteArray imageData = QByteArray::fromRawData(reinterpret_cast<const char*>(buffer), bufferSize);
	setImage(imageData, image.width(), image.height());
}

void FlatBufferConnection::setImage(const QByteArray& imageData, int width, int height, int duration)
{
	if (!isClientRegistered()) return;

	_builder.Clear();
	auto imageDataVector = _builder.CreateVector(reinterpret_cast<const uint8_t*>(imageData.constData()), imageData.size());
	auto rawImage = hyperionnet::CreateRawImage(_builder, imageDataVector, width, height);
	auto image = hyperionnet::CreateImage(_builder, hyperionnet::ImageType_RawImage, rawImage.Union(), duration);
	auto req = hyperionnet::CreateRequest(_builder, hyperionnet::Command_Image, image.Union());

	_builder.Finish(req);
	sendMessage(_builder.GetBufferPointer(), _builder.GetSize());
}

void FlatBufferConnection::clearPriority(int priority)
{
	if (!isClientRegistered()) return;

	_builder.Clear();
	auto clearReq = hyperionnet::CreateClear(_builder, priority);
	auto req = hyperionnet::CreateRequest(_builder,hyperionnet::Command_Clear, clearReq.Union());

	_builder.Finish(req);
	sendMessage(_builder.GetBufferPointer(), _builder.GetSize());
}

void FlatBufferConnection::clearAllPriorities()
{
	clearPriority(-1);
}

void FlatBufferConnection::readData()
{
	_receiveBuffer += _socket.readAll();

	// check if we can read a header
	while(_receiveBuffer.size() >= 4)
	{
		uint32_t messageSize =
				((_receiveBuffer[0]<<24) & 0xFF000000) |
				((_receiveBuffer[1]<<16) & 0x00FF0000) |
				((_receiveBuffer[2]<< 8) & 0x0000FF00) |
				((_receiveBuffer[3]    ) & 0x000000FF);

		// check if we can read a complete message
		if((uint32_t) _receiveBuffer.size() < messageSize + 4) return;

		// extract message only and remove header + msg from buffer :: QByteArray::remove() does not return the removed data
		const QByteArray msg = _receiveBuffer.mid(4, messageSize);
		_receiveBuffer.remove(0, messageSize + 4);
		const uint8_t* msgData = reinterpret_cast<const uint8_t*>(msg.constData());
		flatbuffers::Verifier verifier(msgData, messageSize);

		if (hyperionnet::VerifyReplyBuffer(verifier))
		{
			parseReply(hyperionnet::GetReply(msgData));
			continue;
		}
		Error(_log, "Unable to parse reply");
	}
}

void FlatBufferConnection::setSkipReply(bool skip)
{
	if(skip)
	{
		disconnect(&_socket, &QTcpSocket::readyRead, 0, 0);
	}
	else
	{
		connect(&_socket, &QTcpSocket::readyRead, this, &FlatBufferConnection::readData, Qt::UniqueConnection);
	}
}

bool FlatBufferConnection::parseReply(const hyperionnet::Reply *reply)
{
	if (!reply->error())
	{
		// no error set must be a success or registered or video
		const auto videoMode = reply->video();

		if (videoMode != -1) {
			// We got a video reply.
			emit setVideoMode(static_cast<VideoMode>(videoMode));
			return true;
		}

		const auto registered = reply->registered();

		if (!_isRegistered)
		{
			// We got a client is registered reply.
			if (registered == -1 || registered != _priority)
			{
				registerClient(_origin, _priority);
			}
			else
			{
				_isRegistered = true;
				_timer.stop();
				Debug(_log,"Client \"%s\" registered successfully with target host: %s, port [%u]", QSTRING_CSTR(_origin), QSTRING_CSTR(_host.toString()), _port);
				emit isReadyToSend();
			}
		}
		return true;
	}
	else
	{
		_timer.stop();
		QString error = reply->error()->c_str();
		Error(_log, "Reply error: %s", QSTRING_CSTR(error));
		emit errorOccured(error);
	}
	return false;
}
