// qt
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QRgb>

// project includes
#include "ProtoClientConnection.h"

Q_LOGGING_CATEGORY(proto_server_client_flow, "hyperion.proto.server.flow");
Q_LOGGING_CATEGORY(proto_server_client_cmd, "hyperion.proto.server.cmd");

// TODO Remove this class if third-party apps have been migrated (eg. Hyperion Android Grabber, Windows Screen grabber etc.)

ProtoClientConnection::ProtoClientConnection(QTcpSocket* socket, int timeout, QObject *parent)
	: QObject(parent)
	, _log(Logger::getInstance("PROTOSERVER"))
	, _socket(socket)
	, _clientAddress(socket->peerAddress().toString())
	, _timeoutTimer(new QTimer(this))
	, _timeout(timeout * 1000)
	, _priority()
{
	TRACK_SCOPE();
	// timer setup
	_timeoutTimer.reset(new QTimer());	
	_timeoutTimer->setSingleShot(true);
	_timeoutTimer->setInterval(_timeout);
	connect(_timeoutTimer.get(), &QTimer::timeout, this, &ProtoClientConnection::forceClose);

	// connect socket signals
	connect(_socket, &QTcpSocket::readyRead, this, &ProtoClientConnection::readyRead);
	connect(_socket, &QTcpSocket::disconnected, this, &ProtoClientConnection::disconnected);
}

void ProtoClientConnection::readyRead()
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

void ProtoClientConnection::forceClose()
{
	qCDebug(proto_server_client_flow) << "Forcing close of client" << QString("PROTO1@%2").arg( _clientAddress);
	_timeoutTimer->stop();
	_socket->close();
}

void ProtoClientConnection::disconnected()
{
	Debug(_log, "Socket Closed");
	_socket->deleteLater();
	emit clearGlobalInput(_priority);
	emit clientDisconnected();
}

void ProtoClientConnection::handleMessage(const proto::HyperionRequest & message)
{
	switch (message.command())
	{
	case proto::HyperionRequest::COLOR:
		if (!message.HasExtension(proto::ColorRequest::colorRequest))
		{
			qCDebug(proto_server_client_flow) << "Received COLOR command without ColorRequest from client" << QString("PROTO1@%2").arg( _clientAddress);
			sendErrorReply("Received COLOR command without ColorRequest");
			break;
		}
		handleColorCommand(message.GetExtension(proto::ColorRequest::colorRequest));
		break;
	case proto::HyperionRequest::IMAGE:
		if (!message.HasExtension(proto::ImageRequest::imageRequest))
		{
			qCDebug(proto_server_client_flow) << "Received IMAGE command without ImageRequest from client" << QString("PROTO1@%2").arg( _clientAddress);
			sendErrorReply("Received IMAGE command without ImageRequest");
			break;
		}
		handleImageCommand(message.GetExtension(proto::ImageRequest::imageRequest));
		break;
	case proto::HyperionRequest::CLEAR:
		if (!message.HasExtension(proto::ClearRequest::clearRequest))
		{
			qCDebug(proto_server_client_flow) << "Received CLEAR command without ClearRequest from client" << QString("PROTO1@%2").arg( _clientAddress);
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
	QVector<ColorRgb> color{ ColorRgb{ uint8_t(qRed(message.rgbcolor())), uint8_t(qGreen(message.rgbcolor())), uint8_t(qBlue(message.rgbcolor())) } };

	if (priority < 100 || priority >= 200)
	{
		sendErrorReply("The priority " + std::to_string(priority) + " is not in the valid priority range between 100 and 199.");
		Error(_log, "The priority %d is not in the proto-connection range between 100 and 199.", priority);
		return;
	}

	qCDebug(proto_server_client_cmd) << "Received color command from client" << QString("PROTO1@%2").arg( _clientAddress) << "with priority" << priority;

	// make sure the prio is registered before setColor()
	if(priority != _priority)
	{
		emit clearGlobalInput(_priority);
		emit registerGlobalInput(priority, hyperion::COMP_PROTOSERVER, "Proto@"+_clientAddress);
		_priority = priority;
	}

	// set output
	emit setGlobalInputColor(_priority, color, duration);

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

	if (priority < 100 || priority >= 200)
	{
		sendErrorReply("The priority " + std::to_string(priority) + " is not in the valid priority range between 100 and 199.");
		Error(_log, "The priority %d is not in the proto-connection range between 100 and 199.", priority);
		return;
	}

	// make sure the prio is registered before setInput()
	if(priority != _priority)
	{
		emit clearGlobalInput(_priority);
		emit registerGlobalInput(priority, hyperion::COMP_PROTOSERVER, "Proto@"+_clientAddress);
		_priority = priority;
	}

	if (width <= 0 || height <= 0)
	{
		qCDebug(proto_server_client_flow) << "Received image command with invalid width and/or height from client" << QString("PROTO1@%2").arg( _clientAddress);
		sendErrorReply("Size of image data does not match with the width and height");
		return;
	}

	// check consistency of the size of the received data
	int channelCount = (int)imageData.size()/(width*height);
	if (channelCount != 3 && channelCount != 4)
	{
		qCDebug(proto_server_client_flow) << "Received image command with invalid size of image data from client" << QString("PROTO1@%2").arg( _clientAddress);
		sendErrorReply("Size of image data does not match with the width and height");
		return;
	}

	// create ImageRgb
	Image<ColorRgb> imageRGB(width, height);
	if (channelCount == 3)
	{
		memmove(imageRGB.memptr(), imageData.c_str(), imageData.size());
	}

	if (channelCount == 4)
	{
		for (int source=0, destination=0; source < width * height * static_cast<int>(sizeof(ColorRgb)); source+=sizeof(ColorRgb), destination+=sizeof(ColorRgba))
		{
			memmove((uint8_t*)imageRGB.memptr() + source, imageData.c_str() + destination, sizeof(ColorRgb));
		}
	}

	qCDebug(proto_server_client_cmd) << "Received image from client" << QString("PROTO1@%2").arg(_clientAddress)
									  << "with priority" << _priority
									  << "with size" << imageRGB.width() << "x" << imageRGB.height()
									  << "and duration" << duration << "ms";

	emit setGlobalInputImage(_priority, imageRGB, duration);
	emit setBufferImage("ProtoBuffer", imageRGB);

	// send reply
	sendSuccessReply();
}


void ProtoClientConnection::handleClearCommand(const proto::ClearRequest &message)
{
	// extract parameters
	int priority = message.priority();

	qCDebug(proto_server_client_cmd) << "Received clear command from client" << QString("PROTO1@%2").arg( _clientAddress) << "with priority" << priority;

	// clear priority
	emit clearGlobalInput(priority);
	// send reply
	sendSuccessReply();
}

void ProtoClientConnection::handleClearallCommand()
{
	qCDebug(proto_server_client_cmd) << "Received clearall command from client" << QString("PROTO1@%2").arg( _clientAddress);
	// clear all priority
	emit clearGlobalInput(-1);

	// send reply
	sendSuccessReply();
}


void ProtoClientConnection::handleNotImplemented()
{
	qCDebug(proto_server_client_flow) << "Received not implemented command from client" << QString("PROTO1@%2").arg( _clientAddress);
	sendErrorReply("Command not implemented");
}

void ProtoClientConnection::sendMessage(const google::protobuf::Message &message)
{
	std::string serializedReply = message.SerializeAsString();
	auto size = static_cast<uint32_t>(serializedReply.size());
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
