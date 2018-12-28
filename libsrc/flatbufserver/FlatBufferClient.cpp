#include "FlatBufferClient.h"

// qt
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>

#include <hyperion/Hyperion.h>
#include <QDebug>
FlatBufferClient::FlatBufferClient(QTcpSocket* socket, const int &timeout, QObject *parent)
	: QObject(parent)
	, _log(Logger::getInstance("FLATBUFSERVER"))
	, _socket(socket)
	, _clientAddress(socket->peerAddress().toString())
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
	qDebug()<<"readyRead";
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

		// remove header + msg from buffer
		const QByteArray& msg = _receiveBuffer.remove(0, messageSize + 4);

		const uint8_t* msgData = reinterpret_cast<const uint8_t*>(msg.mid(3, messageSize).constData());
		flatbuffers::Verifier verifier(msgData, messageSize);

		if (flatbuf::VerifyHyperionRequestBuffer(verifier))
		{
			auto message = flatbuf::GetHyperionRequest(msgData);
			handleMessage(message);
			continue;
		}
		qDebug()<<"Unable to pasrse msg";
		sendErrorReply("Unable to parse message");
	}
		//emit newMessage(msgData,messageSize);


	// Emit this to send a new priority register event to all Hyperion instances,
	// emit registerGlobalInput(_priority, hyperion::COMP_FLATBUFSERVER, QString("%1@%2").arg("PLACE_ORIGIN_STRING_FROM_SENDER_HERE",_socket->peerAddress()));

	// Emit this to send the image data event to all Hyperion instances
	// emit setGlobalInput(_priority, _image, _timeout);
}

void FlatBufferClient::forceClose()
{
	_socket->close();
}

void FlatBufferClient::disconnected()
{
	qDebug()<<"Socket Closed";
	//emit clearGlobalPriority(_priority, hyperion::COMP_FLATBUFSERVER);
    _socket->deleteLater();
	emit clientDisconnected();
}

void FlatBufferClient::handleMessage(const flatbuf::HyperionRequest * message)
{
	switch (message->command())
	{
	case flatbuf::Command_COLOR:
		qDebug()<<"handle colorReuest";
		if (!flatbuffers::IsFieldPresent(message, flatbuf::HyperionRequest::VT_COLORREQUEST))
		{
			sendErrorReply("Received COLOR command without ColorRequest");
			break;
		}
		//handleColorCommand(message->colorRequest());
		break;
	case flatbuf::Command_IMAGE:
		qDebug()<<"handle imageReuest";
		if (!flatbuffers::IsFieldPresent(message, flatbuf::HyperionRequest::VT_IMAGEREQUEST))
		{
			sendErrorReply("Received IMAGE command without ImageRequest");
			break;
		}
		handleImageCommand(message->imageRequest());
		break;
	case flatbuf::Command_CLEAR:
		if (!flatbuffers::IsFieldPresent(message, flatbuf::HyperionRequest::VT_CLEARREQUEST))
		{
			sendErrorReply("Received CLEAR command without ClearRequest");
			break;
		}
		//handleClearCommand(message->clearRequest());
		break;
	case flatbuf::Command_CLEARALL:
		//handleClearallCommand();
		break;
	default:
		qDebug()<<"handleNotImplemented";
		handleNotImplemented();
	}
}

void FlatBufferClient::handleImageCommand(const flatbuf::ImageRequest *message)
{
	// extract parameters
	int priority = message->priority();
	int duration = message->duration();
	int width = message->imagewidth();
	int height = message->imageheight();
	const auto & imageData = message->imagedata();

	// make sure the prio is registered before setInput()
	if(priority != _priority)
	{
		_hyperion->clear(_priority);
		_hyperion->registerInput(priority, hyperion::COMP_FLATBUFSERVER, "proto@"+_clientAddress);
		_priority = priority;
	}

	// check consistency of the size of the received data
	if ((int) imageData->size() != width*height*3)
	{
		sendErrorReply("Size of image data does not match with the width and height");
		return;
	}

	// create ImageRgb
	Image<ColorRgb> image(width, height);
	memcpy(image.memptr(), imageData->data(), imageData->size());

	_hyperion->setInputImage(_priority, image, duration);

	// send reply
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
	auto reply = flatbuf::CreateHyperionReplyDirect(_builder, flatbuf::Type_REPLY, true);
	_builder.Finish(reply);

	// send reply
	sendMessage();
}

void FlatBufferClient::sendErrorReply(const std::string &error)
{
	// create reply
	auto reply = flatbuf::CreateHyperionReplyDirect(_builder, flatbuf::Type_REPLY, false, error.c_str());
	_builder.Finish(reply);

	// send reply
	sendMessage();
}
