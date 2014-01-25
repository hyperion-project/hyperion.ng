// stl includes
#include <stdexcept>

// Qt includes
#include <QRgb>

// hyperion-v4l2 includes
#include "ProtoConnection.h"

ProtoConnection::ProtoConnection(const std::string & a) :
	_socket()
{
	QString address(a.c_str());
	QStringList parts = address.split(":");
	if (parts.size() != 2)
	{
		throw std::runtime_error(QString("Wrong address: unable to parse address (%1)").arg(address).toStdString());
	}

	bool ok;
	uint16_t port = parts[1].toUShort(&ok);
	if (!ok)
	{
		throw std::runtime_error(QString("Wrong address: Unable to parse the port number (%1)").arg(parts[1]).toStdString());
	}

	_socket.connectToHost(parts[0], port);
	if (!_socket.waitForConnected())
	{
		throw std::runtime_error("Unable to connect to host");
	}

	std::cout << "Connected to " << a << std::endl;
}

ProtoConnection::~ProtoConnection()
{
	_socket.close();
}

void ProtoConnection::setColor(const ColorRgb & color, int priority, int duration)
{
	proto::HyperionRequest request;
	request.set_command(proto::HyperionRequest::COLOR);
	proto::ColorRequest * colorRequest = request.MutableExtension(proto::ColorRequest::colorRequest);
	colorRequest->set_rgbcolor((color.red << 16) | (color.green << 8) | color.blue);
	colorRequest->set_priority(priority);
	colorRequest->set_duration(duration);

	// send command message
	proto::HyperionReply reply = sendMessage(request);

	// parse reply message
	parseReply(reply);
}

void ProtoConnection::setImage(const Image<ColorRgb> &image, int priority, int duration)
{
	proto::HyperionRequest request;
	request.set_command(proto::HyperionRequest::IMAGE);
	proto::ImageRequest * imageRequest = request.MutableExtension(proto::ImageRequest::imageRequest);
	imageRequest->set_imagedata(image.memptr(), image.width() * image.height() * 3);
	imageRequest->set_imagewidth(image.width());
	imageRequest->set_imageheight(image.height());
	imageRequest->set_priority(priority);
	imageRequest->set_duration(duration);

	// send command message
	proto::HyperionReply reply = sendMessage(request);

	// parse reply message
//	parseReply(reply);
}

void ProtoConnection::clear(int priority)
{
	proto::HyperionRequest request;
	request.set_command(proto::HyperionRequest::CLEAR);
	proto::ClearRequest * clearRequest = request.MutableExtension(proto::ClearRequest::clearRequest);
	clearRequest->set_priority(priority);

	// send command message
	proto::HyperionReply reply = sendMessage(request);

	// parse reply message
	parseReply(reply);
}

void ProtoConnection::clearAll()
{
	proto::HyperionRequest request;
	request.set_command(proto::HyperionRequest::CLEARALL);

	// send command message
	proto::HyperionReply reply = sendMessage(request);

	// parse reply message
	parseReply(reply);
}

proto::HyperionReply ProtoConnection::sendMessage(const proto::HyperionRequest &message)
{
	// serialize message (FastWriter already appends a newline)
	std::string serializedMessage = message.SerializeAsString();

	int length = serializedMessage.size();
	const uint8_t header[] = {
		uint8_t((length >> 24) & 0xFF),
		uint8_t((length >> 16) & 0xFF),
		uint8_t((length >>  8) & 0xFF),
		uint8_t((length      ) & 0xFF)};

	// write message
	int count = 0;
	count += _socket.write(reinterpret_cast<const char *>(header), 4);
	count += _socket.write(reinterpret_cast<const char *>(serializedMessage.data()), length);
	if (!_socket.waitForBytesWritten())
	{
		throw std::runtime_error("Error while writing data to host");
	}

	/*
	// read reply data
	QByteArray serializedReply;
	length = -1;
	while (serializedReply.size() != length)
	{
		std::cout << length << std::endl;
		// receive reply
		if (!_socket.waitForReadyRead())
		{
			throw std::runtime_error("Error while reading data from host");
		}

		serializedReply += _socket.readAll();

		if (length < 0 && serializedReply.size() >= 4)
		{
			std::cout << (int) serializedReply[3] << std::endl;
			std::cout << (int) serializedReply[2] << std::endl;
			std::cout << (int) serializedReply[1] << std::endl;
			std::cout << (int) serializedReply[0] << std::endl;

			length = (uint8_t(serializedReply[0]) << 24) | (uint8_t(serializedReply[1]) << 16) | (uint8_t(serializedReply[2]) << 8) | uint8_t(serializedReply[3]) ;
		}
	}

	std::cout << length << std::endl;
*/
	// parse reply data
	proto::HyperionReply reply;
//	reply.ParseFromArray(serializedReply.constData()+4, length);
	return reply;
}

bool ProtoConnection::parseReply(const proto::HyperionReply &reply)
{
	bool success = false;

	if (!reply.success())
	{
		if (reply.has_error())
		{
			throw std::runtime_error("Error: " + reply.error());
		}
		else
		{
			throw std::runtime_error("Error: No error info");
		}
	}

	return success;
}
