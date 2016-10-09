// stl includes
#include <stdexcept>
#include <cassert>

// Qt includes
#include <QRgb>

// hyperion-remote includes
#include "JsonConnection.h"

JsonConnection::JsonConnection(const QString & address, bool printJson)
	: _printJson(printJson)
	, _socket()
{
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

    qDebug() << "Connected to:" << address;
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

void JsonConnection::setImage(QImage &image, int priority, int duration)
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
	command["imagedata"] = base64Image.data();
	if (duration > 0)
	{
		command["duration"] = duration;
	}

	// send command message
	Json::Value reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

void JsonConnection::setEffect(const QString &effectName, const QString & effectArgs, int priority, int duration)
{
    qDebug() << "Start effect " << effectName;

	// create command
	Json::Value command;
	command["command"] = "effect";
	command["priority"] = priority;
	Json::Value & effect = command["effect"];
	effect["name"] = effectName.toStdString();
	if (effectArgs.size() > 0)
	{
		Json::Reader reader;
		if (!reader.parse(effectArgs.toStdString(), effect["args"], false))
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

void JsonConnection::setComponentState(const QString & component, const bool state)
{
	qDebug() << (state ? "Enable" : "Disable") << "Component" << component;

	// create command
	Json::Value command;
	command["command"] = "componentstate";
	Json::Value & parameter = command["componentstate"];
	parameter["component"] = component.toStdString();
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

QString JsonConnection::getConfig(std::string type)
{
	assert( type == "schema" || type == "config" );
	std::cout << "Get configuration file from Hyperion Server" << std::endl;

	// create command
	Json::Value command;
	command["command"] = "config";
	command["subcommand"] = (type == "schema")? "getschema" : "getconfig";

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

void JsonConnection::setConfig(const QString &jsonString)
{
	// create command
	Json::Value command;
	command["command"] = "config";
	command["subcommand"] = "setconfig";
	
	Json::Value & config = command["config"];
	if (jsonString.size() > 0)
	{
		Json::Reader reader;
		if (!reader.parse(jsonString.toStdString(), config, false))
		{
			throw std::runtime_error("Error in configset arguments: " + reader.getFormattedErrorMessages());
		}
	}

	// send command message
	Json::Value reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

void JsonConnection::setTransform(const QString &transformId,
								  double *saturation,
								  double *value,
								  double *saturationL,
								  double *luminance,
								  double *luminanceMin,
								  QColor threshold,
								  QColor gamma,
								  QColor blacklevel,
								  QColor whitelevel)
{
	std::cout << "Set color transforms" << std::endl;

	// create command
	Json::Value command;
	command["command"] = "transform";
	Json::Value & transform = command["transform"];

	if (!transformId.isNull())
	{
		transform["id"] = transformId.toStdString();
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
	
	if (threshold.isValid())
	{
		Json::Value & v = transform["threshold"];
		v.append(threshold.red());
		v.append(threshold.green());
		v.append(threshold.blue());
	}

	if (gamma.isValid())
	{
		Json::Value & v = transform["gamma"];
		v.append(gamma.red());
		v.append(gamma.green());
		v.append(gamma.blue());
	}

	if (blacklevel.isValid())
	{
		Json::Value & v = transform["blacklevel"];
		v.append(blacklevel.red());
		v.append(blacklevel.green());
		v.append(blacklevel.blue());
	}

	if (whitelevel.isValid())
	{
		Json::Value & v = transform["whitelevel"];
		v.append(whitelevel.red());
		v.append(whitelevel.green());
		v.append(whitelevel.blue());
	}

	// send command message
	Json::Value reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

void JsonConnection::setCorrection(QString &correctionId, const QColor & correction)
{
	std::cout << "Set color corrections" << std::endl;

	// create command
	Json::Value command;
	command["command"] = "correction";
	Json::Value & correct = command["correction"];
	
	if (!correctionId.isNull())
	{
		correct["id"] = correctionId.toStdString();
	}

	if (correction.isValid())
	{
		Json::Value & v = correct["correctionValues"];
		v.append(correction.red());
		v.append(correction.green());
		v.append(correction.blue());
	}

	// send command message
	Json::Value reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

void JsonConnection::setTemperature(const QString &temperatureId, const QColor & temperature)
{
	std::cout << "Set color temperature corrections" << std::endl;

	// create command
	Json::Value command;
	command["command"] = "temperature";
	Json::Value & temp = command["temperature"];

	if (!temperatureId.isNull())
	{
		temp["id"] = temperatureId.toStdString();
	}

	if (temperature.isValid())
	{
		Json::Value & v = temp["correctionValues"];
		v.append(temperature.red());
		v.append(temperature.green());
		v.append(temperature.blue());
	}

	// send command message
	Json::Value reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

void JsonConnection::setAdjustment(const QString &adjustmentId,
								   const QColor & redAdjustment,
								   const QColor & greenAdjustment,
								   const QColor & blueAdjustment)
{
	std::cout << "Set color adjustments" << std::endl;

	// create command
	Json::Value command;
	command["command"] = "adjustment";
	Json::Value & adjust = command["adjustment"];

	if (!adjustmentId.isNull())
	{
		adjust["id"] = adjustmentId.toStdString();
	}
	
	if (redAdjustment.isValid())
	{
		Json::Value & v = adjust["redAdjust"];
		v.append(redAdjustment.red());
		v.append(redAdjustment.green());
		v.append(redAdjustment.blue());
	}

	if (greenAdjustment.isValid())
	{
		Json::Value & v = adjust["greenAdjust"];
		v.append(greenAdjustment.red());
		v.append(greenAdjustment.green());
		v.append(greenAdjustment.blue());
	}

	if (blueAdjustment.isValid())
	{
		Json::Value & v = adjust["blueAdjust"];
		v.append(blueAdjustment.red());
		v.append(blueAdjustment.green());
		v.append(blueAdjustment.blue());
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
