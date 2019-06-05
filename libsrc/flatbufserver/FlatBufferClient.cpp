#include "FlatBufferClient.h"

// qt
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QRgb>

#include <hyperion/Hyperion.h>
FlatBufferClient::FlatBufferClient(QTcpSocket* socket, const int &timeout, QObject *parent)
	: QObject(parent)
	, _log(Logger::getInstance("FLATBUFSERVER"))
	, _socket(socket)
	, _clientAddress("@"+socket->peerAddress().toString())
	, _timeoutTimer(new QTimer(this))
	, _timeout(timeout * 1000)
	, _priority()
	, _hyperion(Hyperion::getInstance())
{
	// timer setup
	_timeoutTimer->setSingleShot(true);
	_timeoutTimer->setInterval(_timeout);
	connect(_timeoutTimer, &QTimer::timeout, this, &FlatBufferClient::forceClose);

	// connect socket signals
	connect(_socket, &QTcpSocket::readyRead, this, &FlatBufferClient::readyRead);
	connect(_socket, &QTcpSocket::disconnected, this, &FlatBufferClient::disconnected);
}

void FlatBufferClient::readyRead()
{
	_timeoutTimer->start();

	_receiveBuffer += _socket->readAll();

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
		const QByteArray msg = _receiveBuffer.right(messageSize);
		_receiveBuffer.remove(0, messageSize + 4);

		const auto* msgData = reinterpret_cast<const uint8_t*>(msg.constData());
		flatbuffers::Verifier verifier(msgData, messageSize);

		if (hyperionnet::VerifyRequestBuffer(verifier))
		{
			auto message = hyperionnet::GetRequest(msgData);
			handleMessage(message);
			continue;
		}
		sendErrorReply("Unable to parse message");
	}
}

void FlatBufferClient::forceClose()
{
	_socket->close();
}

void FlatBufferClient::disconnected()
{
	Debug(_log, "Socket Closed");
    _socket->deleteLater();
	_hyperion->clear(_priority);
	emit clientDisconnected();
}

void FlatBufferClient::handleMessage(const hyperionnet::Request * req)
{
	const void* reqPtr;
	if ((reqPtr = req->command_as_Color()) != nullptr) {
		handleColorCommand(static_cast<const hyperionnet::Color*>(reqPtr));
	} else if ((reqPtr = req->command_as_Image()) != nullptr) {
		handleImageCommand(static_cast<const hyperionnet::Image*>(reqPtr));
	} else if ((reqPtr = req->command_as_Clear()) != nullptr) {
		handleClearCommand(static_cast<const hyperionnet::Clear*>(reqPtr));
	} else if ((reqPtr = req->command_as_Register()) != nullptr) {
		handleRegisterCommand(static_cast<const hyperionnet::Register*>(reqPtr));
	} else {
		sendErrorReply("Received invalid packet.");
	}
}

void FlatBufferClient::handleColorCommand(const hyperionnet::Color *colorReq)
{
	// extract parameters
	const int32_t rgbData = colorReq->data();
	ColorRgb color;
	color.red = qRed(rgbData);
	color.green = qGreen(rgbData);
	color.blue = qBlue(rgbData);

	// set output
	_hyperion->setColor(_priority, color, colorReq->duration());

	// send reply
	sendSuccessReply();
}

void FlatBufferClient::handleRegisterCommand(const hyperionnet::Register *regReq)
{
	_priority = regReq->priority();
	_hyperion->registerInput(_priority, hyperion::COMP_FLATBUFSERVER, regReq->origin()->c_str()+_clientAddress);

	auto reply = hyperionnet::CreateReplyDirect(_builder, nullptr, -1, (_priority ? _priority : -1));
	_builder.Finish(reply);

	// send reply
	sendMessage();
}

void FlatBufferClient::handleImageCommand(const hyperionnet::Image *image)
{
	// extract parameters
	int duration = image->duration();

	const void* reqPtr;
	if ((reqPtr = image->data_as_RawImage()) != nullptr)
	{
		const auto *img = static_cast<const hyperionnet::RawImage*>(reqPtr);
		const auto & imageData = img->data();
		const int width = img->width();
		const int height = img->height();

		if ((int) imageData->size() != width*height*3)
		{
			sendErrorReply("Size of image data does not match with the width and height");
			return;
		}

		Image<ColorRgb> imageDest(width, height);
		memmove(imageDest.memptr(), imageData->data(), imageData->size());
		_hyperion->setInputImage(_priority, imageDest, duration);
	}

	// send reply
	sendSuccessReply();
}


void FlatBufferClient::handleClearCommand(const hyperionnet::Clear *clear)
{
	// extract parameters
	const int priority = clear->priority();

	if (priority == -1) {
		_hyperion->clearall();
	}
	else {
		// Check if we are clearing ourselves.
		if (priority == _priority) {
			_priority = -1;
		}

		_hyperion->clear(priority);
	}

	sendSuccessReply();
}

void FlatBufferClient::handleNotImplemented()
{
	sendErrorReply("Command not implemented");
}

void FlatBufferClient::sendMessage()
{
	auto size = _builder.GetSize();
	const uint8_t* buffer = _builder.GetBufferPointer();
	uint8_t sizeData[] = {uint8_t(size >> 24), uint8_t(size >> 16), uint8_t(size >> 8), uint8_t(size)};
	_socket->write((const char *) sizeData, sizeof(sizeData));
	_socket->write((const char *)buffer, size);
	_socket->flush();
	_builder.Clear();
}

void FlatBufferClient::sendSuccessReply()
{
	auto reply = hyperionnet::CreateReplyDirect(_builder);
	_builder.Finish(reply);

	// send reply
	sendMessage();
}

void FlatBufferClient::sendErrorReply(const std::string &error)
{
	// create reply
	auto reply = hyperionnet::CreateReplyDirect(_builder, error.c_str());
	_builder.Finish(reply);

	// send reply
	sendMessage();
}
