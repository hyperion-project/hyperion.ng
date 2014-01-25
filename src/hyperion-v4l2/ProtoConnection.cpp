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

void ProtoConnection::setColor(std::vector<QColor> colors, int priority, int duration)
{
	// create command
	Json::Value command;
	command["command"] = "color";
	command["priority"] = priority;
	Json::Value & rgbValue = command["color"];
	for (const QColor & color : colors)
	{
		rgbValue.append(color.red());
		rgbValue.append(color.green());
		rgbValue.append(color.blue());
	}
	if (duration > 0)
	{
		command["duration"] = duration;
	}

	// send command message
	Json::Value reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

void ProtoConnection::setImage(const Image<ColorRgb> &image, int priority, int duration)
{
	// ensure the image has RGB888 format
	QByteArray binaryImage = QByteArray::fromRawData(reinterpret_cast<const char *>(image.memptr()), image.width() * image.height() * 3);
	const QByteArray base64Image = binaryImage.toBase64();

	// create command
	Json::Value command;
	command["command"] = "image";
	command["priority"] = priority;
	command["imagewidth"] = image.width();
	command["imageheight"] = image.height();
	command["imagedata"] = std::string(base64Image.data(), base64Image.size());
	if (duration > 0)
	{
		command["duration"] = duration;
	}

	// send command message
	Json::Value reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

void ProtoConnection::clear(int priority)
{
	std::cout << "Clear priority channel " << priority << std::endl;

	// create command
	Json::Value command;
	command["command"] = "clear";
	command["priority"] = priority;

	// send command message
	Json::Value reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

void ProtoConnection::clearAll()
{
	std::cout << "Clear all priority channels" << std::endl;

	// create command
	Json::Value command;
	command["command"] = "clearall";

	// send command message
	Json::Value reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

Json::Value ProtoConnection::sendMessage(const Json::Value & message)
{
	// serialize message (FastWriter already appends a newline)
	std::string serializedMessage = Json::FastWriter().write(message);

	// write message
	_socket.write(serializedMessage.c_str());
	if (!_socket.waitForBytesWritten())
	{
		throw std::runtime_error("Error while writing data to host");
	}

	// read reply data
	QByteArray serializedReply;
	while (!serializedReply.contains('\n'))
	{
		// receive reply
		if (!_socket.waitForReadyRead())
		{
			throw std::runtime_error("Error while reading data from host");
		}

		serializedReply += _socket.readAll();
	}
	int bytes = serializedReply.indexOf('\n') + 1;     // Find the end of message

	// parse reply data
	Json::Reader jsonReader;
	Json::Value reply;
	if (!jsonReader.parse(serializedReply.constData(), serializedReply.constData() + bytes, reply))
	{
		throw std::runtime_error("Error while parsing reply: invalid json");
	}

	return reply;
}

bool ProtoConnection::parseReply(const Json::Value &reply)
{
	bool success = false;
	std::string reason = "No error info";

	try
	{
		success = reply.get("success", false).asBool();
		if (!success)
			reason = reply.get("error", reason).asString();
	}
	catch (const std::runtime_error &)
	{
		// Some json parsing error: ignore and set parsing error
	}

	if (!success)
	{
		throw std::runtime_error("Error: " + reason);
	}

	return success;
}
