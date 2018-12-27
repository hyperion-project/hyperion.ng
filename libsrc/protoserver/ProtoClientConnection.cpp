// system includes
#include <stdexcept>
#include <cassert>

// stl includes
#include <iostream>
#include <sstream>
#include <iterator>

// Qt includes
#include <QRgb>
#include <QResource>
#include <QDateTime>
#include <QHostInfo>

// hyperion util includes
#include "utils/ColorRgb.h"

// project includes
#include "ProtoClientConnection.h"

ProtoClientConnection::ProtoClientConnection(QTcpSocket *socket)
	: QObject()
	, _socket(socket)
	, _hyperion(Hyperion::getInstance())
	, _receiveBuffer()
	, _priority(-1)
	, _clientAddress(QHostInfo::fromName(socket->peerAddress().toString()).hostName())
{
	// connect internal signals and slots
	connect(_socket, SIGNAL(disconnected()), this, SLOT(socketClosed()));
	connect(_socket, SIGNAL(readyRead()), this, SLOT(readData()));
}

ProtoClientConnection::~ProtoClientConnection()
{
	delete _socket;
}

void ProtoClientConnection::readData()
{
	_receiveBuffer += _socket->readAll();

	// check if we can read a message size
	if (_receiveBuffer.size() <= 4)
	{
		return;
	}

	// read the message size
	uint32_t messageSize =
			((_receiveBuffer[0]<<24) & 0xFF000000) |
			((_receiveBuffer[1]<<16) & 0x00FF0000) |
			((_receiveBuffer[2]<< 8) & 0x0000FF00) |
			((_receiveBuffer[3]    ) & 0x000000FF);

	// check if we can read a complete message
	if ((uint32_t) _receiveBuffer.size() < messageSize + 4)
	{
		return;
	}

	// read a message
	proto::HyperionRequest message;
	if (!message.ParseFromArray(_receiveBuffer.data() + 4, messageSize))
	{
		sendErrorReply("Unable to parse message");
	}

	// handle the message
	handleMessage(message);

	// remove message data from buffer
	_receiveBuffer = _receiveBuffer.mid(messageSize + 4);
}

void ProtoClientConnection::socketClosed()
{
	_hyperion->clear(_priority);
	emit connectionClosed(this);
}

void ProtoClientConnection::setVideoMode(const VideoMode videoMode)
{
	int video_Mode = (int)videoMode;
	proto::HyperionReply vMode;

	// create proto message
	vMode.set_type(proto::HyperionReply::VIDEO);
	vMode.set_video(video_Mode);

	// send message
	sendMessage(vMode);
}

void ProtoClientConnection::handleMessage(const proto::HyperionRequest & message)
{
	// forward messages
	emit newMessage(&message);

	switch (message.command())
	{
	case proto::HyperionRequest::COLOR:
		if (!message.HasExtension(proto::ColorRequest::colorRequest))
		{
			sendErrorReply("Received COLOR command without ColorRequest");
			break;
		}
		handleColorCommand(message.GetExtension(proto::ColorRequest::colorRequest));
		break;
	case proto::HyperionRequest::IMAGE:
		if (!message.HasExtension(proto::ImageRequest::imageRequest))
		{
			sendErrorReply("Received IMAGE command without ImageRequest");
			break;
		}
		handleImageCommand(message.GetExtension(proto::ImageRequest::imageRequest));
		break;
	case proto::HyperionRequest::CLEAR:
		if (!message.HasExtension(proto::ClearRequest::clearRequest))
		{
			sendErrorReply("Received CLEAR command without ClearRequest");
			break;
		}
		handleClearCommand(message.GetExtension(proto::ClearRequest::clearRequest));
		break;
	case proto::HyperionRequest::CLEARALL:
		handleClearallCommand();
		break;
	default:
		handleNotImplemented();
	}
}

void ProtoClientConnection::handleColorCommand(const proto::ColorRequest &message)
{
	// extract parameters
	int priority = message.priority();
	int duration = message.has_duration() ? message.duration() : -1;
	ColorRgb color;
	color.red = qRed(message.rgbcolor());
	color.green = qGreen(message.rgbcolor());
	color.blue = qBlue(message.rgbcolor());

	// make sure the prio is registered before setColor()
	if(priority != _priority)
	{
		_hyperion->clear(_priority);
		_hyperion->registerInput(priority, hyperion::COMP_PROTOSERVER, "proto@"+_clientAddress);
		_priority = priority;
	}

	// set output
	_hyperion->setColor(_priority, color, duration);

	// send reply
	sendSuccessReply();
}

void ProtoClientConnection::handleImageCommand(const proto::ImageRequest &message)
{
	// extract parameters
	int priority = message.priority();
	int duration = message.has_duration() ? message.duration() : -1;
	int width = message.imagewidth();
	int height = message.imageheight();
	const std::string & imageData = message.imagedata();

	// make sure the prio is registered before setInput()
	if(priority != _priority)
	{
		_hyperion->clear(_priority);
		_hyperion->registerInput(priority, hyperion::COMP_PROTOSERVER, "proto@"+_clientAddress);
		_priority = priority;
	}

	// check consistency of the size of the received data
	if ((int) imageData.size() != width*height*3)
	{
		sendErrorReply("Size of image data does not match with the width and height");
		return;
	}

	// create ImageRgb
	Image<ColorRgb> image(width, height);
	memcpy(image.memptr(), imageData.c_str(), imageData.size());

	_hyperion->setInputImage(_priority, image, duration);

	// send reply
	sendSuccessReply();
}


void ProtoClientConnection::handleClearCommand(const proto::ClearRequest &message)
{
	// extract parameters
	int priority = message.priority();

	// clear priority
	_hyperion->clear(priority);
	// send reply
	sendSuccessReply();
}

void ProtoClientConnection::handleClearallCommand()
{
	// clear priority
	_hyperion->clearall();

	// send reply
	sendSuccessReply();
}


void ProtoClientConnection::handleNotImplemented()
{
	sendErrorReply("Command not implemented");
}

void ProtoClientConnection::sendMessage(const google::protobuf::Message &message)
{
	std::string serializedReply = message.SerializeAsString();
	uint32_t size = serializedReply.size();
	uint8_t sizeData[] = {uint8_t(size >> 24), uint8_t(size >> 16), uint8_t(size >> 8), uint8_t(size)};
	_socket->write((const char *) sizeData, sizeof(sizeData));
	_socket->write(serializedReply.data(), serializedReply.length());
	_socket->flush();
}

void ProtoClientConnection::sendSuccessReply()
{
	// create reply
	proto::HyperionReply reply;
	reply.set_type(proto::HyperionReply::REPLY);
	reply.set_success(true);

	// send reply
	sendMessage(reply);
}

void ProtoClientConnection::sendErrorReply(const std::string &error)
{
	// create reply
	proto::HyperionReply reply;
	reply.set_type(proto::HyperionReply::REPLY);
	reply.set_success(false);
	reply.set_error(error);

	// send reply
	sendMessage(reply);
}
