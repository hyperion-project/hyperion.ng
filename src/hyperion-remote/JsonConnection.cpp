// stl includes
#include <stdexcept>

// Qt includes
#include <QRgb>

// hyperion-remote includes
#include "JsonConnection.h"

JsonConnection::JsonConnection(const std::string & a, bool printJson)
	: _printJson(printJson)
	, _socket()
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

JsonConnection::~JsonConnection()
{
	_socket.close();
}

void JsonConnection::setColor(std::vector<QColor> colors, int priority, int duration)
{
	std::cout << "Set color to " << colors[0].red() << " " << colors[0].green() << " " << colors[0].blue() << (colors.size() > 1 ? " + ..." : "") << std::endl;

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

void JsonConnection::setImage(QImage image, int priority, int duration)
{
	std::cout << "Set image has size: " << image.width() << "x" << image.height() << std::endl;

	// ensure the image has RGB888 format
	image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
	QByteArray binaryImage;
	binaryImage.reserve(image.width() * image.height() * 3);
	for (int i = 0; i < image.height(); ++i)
	{
		const QRgb * scanline = reinterpret_cast<const QRgb *>(image.scanLine(i));
		for (int j = 0; j < image.width(); ++j)
		{
			binaryImage.append((char) qRed(scanline[j]));
			binaryImage.append((char) qGreen(scanline[j]));
			binaryImage.append((char) qBlue(scanline[j]));
		}
	}
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

void JsonConnection::setEffect(const std::string &effectName, const std::string & effectArgs, int priority, int duration)
{
	std::cout << "Start effect " << effectName << std::endl;

	// create command
	Json::Value command;
	command["command"] = "effect";
	command["priority"] = priority;
	Json::Value & effect = command["effect"];
	effect["name"] = effectName;
	if (effectArgs.size() > 0)
	{
		Json::Reader reader;
		if (!reader.parse(effectArgs, effect["args"], false))
		{
			throw std::runtime_error("Error in effect arguments: " + reader.getFormattedErrorMessages());
		}
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

QString JsonConnection::getServerInfo()
{
	std::cout << "Get server info" << std::endl;

	// create command
	Json::Value command;
	command["command"] = "serverinfo";

	// send command message
	Json::Value reply = sendMessage(command);

	// parse reply message
	if (parseReply(reply))
	{
		if (!reply.isMember("info") || !reply["info"].isObject())
		{
			throw std::runtime_error("No info available in result");
		}

		const Json::Value & info = reply["info"];
		return QString(info.toStyledString().c_str());
	}

	return QString();
}

void JsonConnection::clear(int priority)
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

void JsonConnection::clearAll()
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

void JsonConnection::setComponentState(const std::string& component, const bool state)
{
	state ? std::cout << "Enable Component " : std::cout << "Disable Component ";
	std::cout << component << std::endl;

	// create command
	Json::Value command;
	command["command"] = "componentstate";
	Json::Value & parameter = command["componentstate"];
	parameter["component"] = component;
	parameter["state"] = state;

	// send command message
	Json::Value reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

void JsonConnection::setSource(int priority)
{
	// create command
	Json::Value command;
	command["command"] = "sourceselect";
	command["priority"] = priority;

	// send command message
	Json::Value reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

void JsonConnection::setSourceAutoSelect()
{
	// create command
	Json::Value command;
	command["command"] = "sourceselect";
	command["auto"] = true;

	// send command message
	Json::Value reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

QString JsonConnection::getConfigFile()
{
	std::cout << "Get configuration file from Hyperion Server" << std::endl;

	// create command
	Json::Value command;
	command["command"] = "configget";

	// send command message
	Json::Value reply = sendMessage(command);

	// parse reply message
	if (parseReply(reply))
	{
		if (!reply.isMember("result") || !reply["result"].isObject())
		{
			throw std::runtime_error("No configuration file available in result");
		}

		const Json::Value & config = reply["result"];
		return QString(config.toStyledString().c_str());
	}

	return QString();
}

void JsonConnection::setConfigFile(const std::string &jsonString, bool create)
{
	// create command
	Json::Value command;
	command["command"] = "configset";
	command["create"] = create;
	Json::Value & config = command["configset"];
	if (jsonString.size() > 0)
	{
		Json::Reader reader;
		if (!reader.parse(jsonString, config, false))
		{
			throw std::runtime_error("Error in configset arguments: " + reader.getFormattedErrorMessages());
		}
	}

	// send command message
	Json::Value reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

void JsonConnection::setTransform(std::string * transformId, double * saturation, double * value, double * saturationL, double * luminance, double * luminanceMin, ColorTransformValues *threshold, ColorTransformValues *gamma, ColorTransformValues *blacklevel, ColorTransformValues *whitelevel)
{
	std::cout << "Set color transforms" << std::endl;

	// create command
	Json::Value command;
	command["command"] = "transform";
	Json::Value & transform = command["transform"];

	if (transformId != nullptr)
	{
		transform["id"] = *transformId;
	}

	if (saturation != nullptr)
	{
		transform["saturationGain"] = *saturation;
	}

	if (value != nullptr)
	{
		transform["valueGain"] = *value;
	}
	
	if (saturationL != nullptr)
	{
		transform["saturationLGain"] = *saturationL;
	}

	if (luminance != nullptr)
	{
		transform["luminanceGain"] = *luminance;
	}
	
	if (luminanceMin != nullptr)
	{
		transform["luminanceMinimum"] = *luminanceMin;
	}
	
	if (threshold != nullptr)
	{
		Json::Value & v = transform["threshold"];
		v.append(threshold->valueRed);
		v.append(threshold->valueGreen);
		v.append(threshold->valueBlue);
	}

	if (gamma != nullptr)
	{
		Json::Value & v = transform["gamma"];
		v.append(gamma->valueRed);
		v.append(gamma->valueGreen);
		v.append(gamma->valueBlue);
	}

	if (blacklevel != nullptr)
	{
		Json::Value & v = transform["blacklevel"];
		v.append(blacklevel->valueRed);
		v.append(blacklevel->valueGreen);
		v.append(blacklevel->valueBlue);
	}

	if (whitelevel != nullptr)
	{
		Json::Value & v = transform["whitelevel"];
		v.append(whitelevel->valueRed);
		v.append(whitelevel->valueGreen);
		v.append(whitelevel->valueBlue);
	}

	// send command message
	Json::Value reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

void JsonConnection::setCorrection(std::string * correctionId, ColorCorrectionValues *correction)
{
	std::cout << "Set color corrections" << std::endl;

	// create command
	Json::Value command;
	command["command"] = "correction";
	Json::Value & correct = command["correction"];
	
	if (correctionId != nullptr)
	{
		correct["id"] = *correctionId;
	}

	if (correction != nullptr)
	{
		Json::Value & v = correct["correctionValues"];
		v.append(correction->valueRed);
		v.append(correction->valueGreen);
		v.append(correction->valueBlue);
	}

	// send command message
	Json::Value reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

void JsonConnection::setTemperature(std::string * temperatureId, ColorCorrectionValues *temperature)
{
	std::cout << "Set color temperature corrections" << std::endl;

	// create command
	Json::Value command;
	command["command"] = "temperature";
	Json::Value & temp = command["temperature"];

	if (temperatureId != nullptr)
	{
		temp["id"] = *temperatureId;
	}

	if (temperature != nullptr)
	{
		Json::Value & v = temp["correctionValues"];
		v.append(temperature->valueRed);
		v.append(temperature->valueGreen);
		v.append(temperature->valueBlue);
	}

	// send command message
	Json::Value reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

void JsonConnection::setAdjustment(std::string * adjustmentId, ColorAdjustmentValues * redAdjustment, ColorAdjustmentValues * greenAdjustment, ColorAdjustmentValues * blueAdjustment)
{
	std::cout << "Set color adjustments" << std::endl;

	// create command
	Json::Value command;
	command["command"] = "adjustment";
	Json::Value & adjust = command["adjustment"];

	if (adjustmentId != nullptr)
	{
		adjust["id"] = *adjustmentId;
	}
	
	if (redAdjustment != nullptr)
	{
		Json::Value & v = adjust["redAdjust"];
		v.append(redAdjustment->valueRed);
		v.append(redAdjustment->valueGreen);
		v.append(redAdjustment->valueBlue);
	}

	if (greenAdjustment != nullptr)
	{
		Json::Value & v = adjust["greenAdjust"];
		v.append(greenAdjustment->valueRed);
		v.append(greenAdjustment->valueGreen);
		v.append(greenAdjustment->valueBlue);
	}

	if (blueAdjustment != nullptr)
	{
		Json::Value & v = adjust["blueAdjust"];
		v.append(blueAdjustment->valueRed);
		v.append(blueAdjustment->valueGreen);
		v.append(blueAdjustment->valueBlue);
	}

	// send command message
	Json::Value reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

Json::Value JsonConnection::sendMessage(const Json::Value & message)
{
	// serialize message (FastWriter already appends a newline)
	std::string serializedMessage = Json::FastWriter().write(message);

	// print command if requested
	if (_printJson)
	{
		std::cout << "Command: " << serializedMessage;
	}

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

	// print reply if requested
	if (_printJson)
	{
		std::cout << "Reply: " << std::string(serializedReply.data(), bytes);
	}

	// parse reply data
	Json::Reader jsonReader;
	Json::Value reply;
	if (!jsonReader.parse(serializedReply.constData(), serializedReply.constData() + bytes, reply))
	{
		throw std::runtime_error("Error while parsing reply: invalid json");
	}

	return reply;
}

bool JsonConnection::parseReply(const Json::Value &reply)
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
