// stl includes
#include <stdexcept>

// hyperion-remote includes
#include "Connection.h"

Connection::Connection(const std::string & a, bool printJson) :
	_printJson(printJson),
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

Connection::~Connection()
{
	_socket.close();
}

bool Connection::setColor(QColor color, int priority, int duration)
{
	std::cout << "Set color to " << color.red() << " " << color.green() << " " << color.blue() << std::endl;

	// create command
	Json::Value command;
	command["command"] = "color";
	command["priority"] = priority;
	Json::Value & rgbValue = command["color"];
	rgbValue[0] = color.red();
	rgbValue[1] = color.green();
	rgbValue[2] = color.blue();
	if (duration > 0)
	{
		command["duration"] = duration;
	}

	// send command message
	Json::Value reply = sendMessage(command);

	// parse reply message
	return parseReply(reply);
}

bool Connection::setImage(QImage image, int priority, int duration)
{
	std::cout << "Set image has size: " << image.width() << "x" << image.height() << std::endl;

	// ensure the image has RGB888 format
	image = image.convertToFormat(QImage::Format_RGB888);
	QByteArray binaryImage;
	binaryImage.reserve(image.width() * image.height() * 3);
	for (int i = 0; i < image.height(); ++i)
	{
		binaryImage.append(reinterpret_cast<const char *>(image.constScanLine(i)), image.width() * 3);
	}
	const QByteArray base64Image = binaryImage.toBase64();

	// create command
	Json::Value command;
	command["command"] = "image";
	command["priority"] = priority;
	command["width"] = image.width();
	command["height"] = image.height();
	command["data"] = std::string(base64Image.data(), base64Image.size());
	if (duration > 0)
	{
		command["duration"] = duration;
	}

	// send command message
	Json::Value reply = sendMessage(command);

	// parse reply message
	return parseReply(reply);
}


bool Connection::listPriorities()
{
	std::cout << "List priority channels" << std::endl;
	return false;
}

bool Connection::clear(int priority)
{
	std::cout << "Clear priority channel " << priority << std::endl;
	return false;
}

bool Connection::clearAll()
{
	std::cout << "Clear all priority channels" << std::endl;
	return false;
}

bool Connection::setTransform(ColorTransformValues *threshold, ColorTransformValues *gamma, ColorTransformValues *blacklevel, ColorTransformValues *whitelevel)
{
	std::cout << "Set color transforms" << std::endl;
	return false;
}

Json::Value Connection::sendMessage(const Json::Value & message)
{
	// print command if requested
	if (_printJson)
	{
		std::cout << "Command: " << message << std::endl;
	}

	// serialize message
	Json::FastWriter jsonWriter;
	std::string serializedMessage = jsonWriter.write(message);

	// write message
	_socket.write(serializedMessage.c_str());
	if (!_socket.waitForBytesWritten())
	{
		throw std::runtime_error("Error while writing data to host");
	}

	// receive reply
	if (!_socket.waitForReadyRead())
	{
		throw std::runtime_error("Error while reading data from host");
	}
	char data[1024 * 100];
	uint64_t count = _socket.read(data, sizeof(data));
	std::string serializedReply(data, count);
	Json::Reader jsonReader;
	Json::Value reply;
	if (!jsonReader.parse(serializedReply, reply))
	{
		throw std::runtime_error("Error while parsing reply: invalid json");
	}

	// print reply if requested
	if (_printJson)
	{
		std::cout << "Reply:" << reply << std::endl;
	}

	return reply;
}

bool Connection::parseReply(const Json::Value &reply)
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
		// Some json paring error: ignore and set parsing error
	}

	if (!success)
	{
		throw std::runtime_error("Error while paring reply: " + reason);
	}

	return success;
}
