// stl includes
#include <stdexcept>

// Qt includes
#include <QRgb>

// protoserver includes
#include <flatbufserver/FlatBufferConnection.h>

FlatBufferConnection::FlatBufferConnection(const QString & address) :
	_socket(),
	_skipReply(false),
	_prevSocketState(QAbstractSocket::UnconnectedState),
	_log(Logger::getInstance("FLATBUFCONNECTION"))
	{
	QStringList parts = address.split(":");
	if (parts.size() != 2)
	{
		throw std::runtime_error(QString("FLATBUFCONNECTION ERROR: Wrong address: Unable to parse address (%1)").arg(address).toStdString());
	}
	_host = parts[0];

	bool ok;
	_port = parts[1].toUShort(&ok);
	if (!ok)
	{
		throw std::runtime_error(QString("FLATBUFCONNECTION ERROR: Wrong port: Unable to parse the port number (%1)").arg(parts[1]).toStdString());
	}

	// try to connect to host
	Info(_log, "Connecting to Hyperion: %s:%d", _host.toStdString().c_str(), _port);
	connectToHost();

	// start the connection timer
	_timer.setInterval(5000);
	_timer.setSingleShot(false);

	connect(&_timer,SIGNAL(timeout()), this, SLOT(connectToHost()));
	connect(&_socket, SIGNAL(readyRead()), this, SLOT(readData()));
	_timer.start();
}

FlatBufferConnection::~FlatBufferConnection()
{
	_timer.stop();
	_socket.close();
}

void FlatBufferConnection::readData()
{
	qint64 bytesAvail;
	while((bytesAvail = _socket.bytesAvailable()))
	{
		// ignore until we get 4 bytes.
		if (bytesAvail < 4) {
			continue;
		}

		char sizeBuf[4];
		 _socket.read(sizeBuf, sizeof(sizeBuf));

		uint32_t messageSize =
			((sizeBuf[0]<<24) & 0xFF000000) |
			((sizeBuf[1]<<16) & 0x00FF0000) |
			((sizeBuf[2]<< 8) & 0x0000FF00) |
			((sizeBuf[3]    ) & 0x000000FF);

		QByteArray buffer;
		while((uint32_t)buffer.size() < messageSize)
		{
			_socket.waitForReadyRead();
			buffer.append(_socket.read(messageSize - buffer.size()));
		}

		const uint8_t* replyData = reinterpret_cast<const uint8_t*>(buffer.constData());
		flatbuffers::Verifier verifier(replyData, messageSize);

		if (!flatbuf::VerifyHyperionReplyBuffer(verifier))
		{
			Error(_log, "Error while reading data from host");
			return;
		}

		auto reply = flatbuf::GetHyperionReply(replyData);

		parseReply(reply);
	}
}

void FlatBufferConnection::setSkipReply(bool skip)
{
	_skipReply = skip;
}

void FlatBufferConnection::setColor(const ColorRgb & color, int priority, int duration)
{
	auto colorReq = flatbuf::CreateColorRequest(_builder, priority, (color.red << 16) | (color.green << 8) | color.blue, duration);
	auto req = flatbuf::CreateHyperionRequest(_builder,flatbuf::Command_COLOR, colorReq);

	_builder.Finish(req);
	sendMessage(_builder.GetBufferPointer(), _builder.GetSize());
}

void FlatBufferConnection::setImage(const Image<ColorRgb> &image, int priority, int duration)
{
	/* #TODO #BROKEN auto imgData = _builder.CreateVector<flatbuffers::Offset<uint8_t>>(image.memptr(), image.width() * image.height() * 3);
	auto imgReq = flatbuf::CreateImageRequest(_builder, priority, image.width(), image.height(), imgData, duration);
	auto req = flatbuf::CreateHyperionRequest(_builder,flatbuf::Command_IMAGE,0,imgReq);
	_builder.Finish(req);
	sendMessage(_builder.GetBufferPointer(), _builder.GetSize());*/
}

void FlatBufferConnection::clear(int priority)
{
	auto clearReq = flatbuf::CreateClearRequest(_builder, priority);
	auto req = flatbuf::CreateHyperionRequest(_builder,flatbuf::Command_CLEAR,0,0,clearReq);

	_builder.Finish(req);
	sendMessage(_builder.GetBufferPointer(), _builder.GetSize());
}

void FlatBufferConnection::clearAll()
{
	auto req = flatbuf::CreateHyperionRequest(_builder,flatbuf::Command_CLEARALL);

	// send command message
	_builder.Finish(req);
	sendMessage(_builder.GetBufferPointer(), _builder.GetSize());
}

void FlatBufferConnection::connectToHost()
{
	// try connection only when
	if (_socket.state() == QAbstractSocket::UnconnectedState)
	{
	   _socket.connectToHost(_host, _port);
	   //_socket.waitForConnected(1000);
	}
}

void FlatBufferConnection::sendMessage(const uint8_t* buffer, uint32_t size)
{
	// print out connection message only when state is changed
	if (_socket.state() != _prevSocketState )
	{
	  switch (_socket.state() )
	  {
		case QAbstractSocket::UnconnectedState:
		  Info(_log, "No connection to Hyperion: %s:%d", _host.toStdString().c_str(), _port);
		  break;

		case QAbstractSocket::ConnectedState:
		  Info(_log, "Connected to Hyperion: %s:%d", _host.toStdString().c_str(), _port);
		  break;

		default:
		  Debug(_log, "Connecting to Hyperion: %s:%d", _host.toStdString().c_str(), _port);
		  break;
	  }
	  _prevSocketState = _socket.state();
	}


	if (_socket.state() != QAbstractSocket::ConnectedState)
	{
		return;
	}

	const uint8_t header[] = {
		uint8_t((size >> 24) & 0xFF),
		uint8_t((size >> 16) & 0xFF),
		uint8_t((size >>  8) & 0xFF),
		uint8_t((size	  ) & 0xFF)};

	// write message
	int count = 0;
	count += _socket.write(reinterpret_cast<const char *>(header), 4);
	count += _socket.write(reinterpret_cast<const char *>(buffer), size);
	if (!_socket.waitForBytesWritten())
	{
		Error(_log, "Error while writing data to host");
		return;
	}
}

bool FlatBufferConnection::parseReply(const flatbuf::HyperionReply *reply)
{
	bool success = false;

	switch (reply->type())
	{
		case flatbuf::Type_REPLY:
		{
			if (!_skipReply)
			{
				if (!reply->success())
				{
					if (flatbuffers::IsFieldPresent(reply, flatbuf::HyperionReply::VT_ERROR))
					{
						throw std::runtime_error("PROTOCONNECTION ERROR: " + reply->error()->str());
					}
					else
					{
						throw std::runtime_error("PROTOCONNECTION ERROR: No error info");
					}
				}
				else
				{
					success = true;
				}
			}
			break;
		}
		case flatbuf::Type_VIDEO:
		{
			VideoMode vMode = (VideoMode)reply->video();
			emit setVideoMode(vMode);
			break;
		}
	}

	return success;
}
