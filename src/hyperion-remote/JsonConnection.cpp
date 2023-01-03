// stl includes
#include <stdexcept>
#include <cassert>
#include <sstream>
#include <iostream>

// Qt includes
#include <QRgb>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QHostInfo>
#include <QUrl>

#include "HyperionConfig.h"

// hyperion-remote includes
#include "JsonConnection.h"

// util includes
#include <utils/JsonUtils.h>

JsonConnection::JsonConnection(const QString & host, bool printJson , quint16 port)
	: _printJson(printJson)
	, _log(Logger::getInstance("REMOTE"))
{
	_socket.connectToHost(host, port);
	if (!_socket.waitForConnected())
	{
		throw std::runtime_error(QString("Unable to connect to host (%1), port (%2)").arg(host).arg(port).toStdString());
	}

	Debug(_log, "Connected to: %s, port: %u", QSTRING_CSTR(host), port);
}

JsonConnection::~JsonConnection()
{
	_socket.close();
}

void JsonConnection::setColor(std::vector<QColor> colors, int priority, int duration)
{
	Debug(_log, "Set color to [%d,%d,%d] %s", colors[0].red(), colors[0].green(), colors[0].blue(), (colors.size() > 1 ? " + ..." : ""));

	// create command
	QJsonObject command;
	command["command"] = QString("color");
	command["origin"] = QString("hyperion-remote");
	command["priority"] = priority;
	QJsonArray rgbValue;
	for (const QColor & color : colors)
	{
		rgbValue.append(color.red());
		rgbValue.append(color.green());
		rgbValue.append(color.blue());
	}
	command["color"] = rgbValue;

	if (duration > 0)
	{
		command["duration"] = duration;
	}

	// send command message
	QJsonObject reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

void JsonConnection::setImage(QImage &image, int priority, int duration, const QString& name)
{
	Debug(_log, "Set image has size: %dx%d", image.width(), image.height());

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
	QJsonObject command;
	command["command"] = QString("image");
	command["priority"] = priority;
	command["origin"] = QString("hyperion-remote");
	if (!name.isEmpty())
		command["name"] = name;
	command["imagewidth"] = image.width();
	command["imageheight"] = image.height();
	command["imagedata"] = QString(base64Image.data());
	if (duration > 0)
	{
		command["duration"] = duration;
	}

	// send command message
	QJsonObject reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

#if defined(ENABLE_EFFECTENGINE)
void JsonConnection::setEffect(const QString &effectName, const QString & effectArgs, int priority, int duration)
{
	Debug(_log, "Start effect: %s", QSTRING_CSTR(effectName));

	// create command
	QJsonObject command, effect;
	command["command"] = QString("effect");
	command["origin"] = QString("hyperion-remote");
	command["priority"] = priority;
	effect["name"] = effectName;

	if (effectArgs.size() > 0)
	{
		QJsonObject effObj;
		if(!JsonUtils::parse("hyperion-remote-args", effectArgs, effObj, _log))
		{
			throw std::runtime_error("Error in effect arguments, abort");
		}

		effect["args"] = effObj;
	}

	command["effect"] = effect;

	if (duration > 0)
	{
		command["duration"] = duration;
	}

	// send command message
	QJsonObject reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

void JsonConnection::createEffect(const QString &effectName, const QString &effectScript, const QString & effectArgs)
{
	Debug(_log, "Create effect: %s", QSTRING_CSTR(effectName));

	// create command
	QJsonObject effect;
	effect["command"] = QString("create-effect");
	effect["name"] = effectName;
	effect["script"] = effectScript;

	if (effectArgs.size() > 0)
	{
		QJsonObject effObj;
		if(!JsonUtils::parse("hyperion-remote-args", effectScript, effObj, _log))
		{
			throw std::runtime_error("Error in effect arguments, abort");
		}

		effect["args"] = effObj;
	}

	// send command message
	QJsonObject reply = sendMessage(effect);

	// parse reply message
	parseReply(reply);
}

void JsonConnection::deleteEffect(const QString &effectName)
{
	Debug(_log, "Delete effect configuration: %s", QSTRING_CSTR(effectName));

	// create command
	QJsonObject effect;
	effect["command"] = QString("delete-effect");
	effect["name"] = effectName;

	// send command message
	QJsonObject reply = sendMessage(effect);

	// parse reply message
	parseReply(reply);
}
#endif

QString JsonConnection::getServerInfoString()
{
	QJsonDocument doc(getServerInfo());
	QString info(doc.toJson(QJsonDocument::Indented));
	return info;
}

QJsonObject JsonConnection::getServerInfo()
{
	Debug(_log, "Get server info");

	// create command
	QJsonObject command;
	command["command"] = QString("serverinfo");

	// send command message
	QJsonObject reply = sendMessage(command);

	// parse reply message
	if (parseReply(reply))
	{
		if (!reply.contains("info") || !reply["info"].isObject())
		{
			throw std::runtime_error("No info available in reply");
		}

		return reply["info"].toObject();
	}

	return QJsonObject();
}

QString JsonConnection::getSysInfo()
{
	Debug(_log, "Get system info");

	// create command
	QJsonObject command;
	command["command"] = QString("sysinfo");

	// send command message
	QJsonObject reply = sendMessage(command);

	// parse reply message
	if (parseReply(reply))
	{
		if (!reply.contains("info") || !reply["info"].isObject())
		{
			throw std::runtime_error("No info available in reply");
		}

		QJsonDocument doc(reply["info"].toObject());
		QString info(doc.toJson(QJsonDocument::Indented));
		return info;
	}

	return QString();
}

void JsonConnection::suspend()
{
	Info(_log, "Suspend Hyperion. Stop all instances and components");
	QJsonObject command;
	command["command"] = QString("system");
	command["subcommand"] = QString("suspend");

	QJsonObject reply = sendMessage(command);

	parseReply(reply);
}

void JsonConnection::resume()
{
	Info(_log, "Resume Hyperion. Start all instances and components");
	QJsonObject command;
	command["command"] = QString("system");
	command["subcommand"] = QString("resume");

	QJsonObject reply = sendMessage(command);

	parseReply(reply);
}

void JsonConnection::toggleSuspend()
{
	Info(_log, "Toggle between Suspend and Resume");
	QJsonObject command;
	command["command"] = QString("system");
	command["subcommand"] = QString("toggleSuspend");

	QJsonObject reply = sendMessage(command);

	parseReply(reply);
}

void JsonConnection::idle()
{
	Info(_log, "Put Hyperion in Idle mode.");
	QJsonObject command;
	command["command"] = QString("system");
	command["subcommand"] = QString("idle");

	QJsonObject reply = sendMessage(command);

	parseReply(reply);
}

void JsonConnection::toggleIdle()
{
	Info(_log, "Toggle between Idle and Working mode");
	QJsonObject command;
	command["command"] = QString("system");
	command["subcommand"] = QString("toggleIdle");

	QJsonObject reply = sendMessage(command);

	parseReply(reply);
}

void JsonConnection::restart()
{
	Info(_log, "Restart Hyperion...");
	QJsonObject command;
	command["command"] = QString("system");
	command["subcommand"] = QString("restart");

	QJsonObject reply = sendMessage(command);

	parseReply(reply);
}

void JsonConnection::clear(int priority)
{
	Debug(_log, "Clear priority channel [%d]", priority);

	// create command
	QJsonObject command;
	command["command"] = QString("clear");
	command["priority"] = priority;

	// send command message
	QJsonObject reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

void JsonConnection::clearAll()
{
	Debug(_log, "Clear all priority channels");

	// create command
	QJsonObject command;
	command["command"] = QString("clear");
	command["priority"] = -1;

	// send command message
	QJsonObject reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

void JsonConnection::setComponentState(const QString & component, bool state)
{
	Debug(_log, "%s Component: %s", (state ? "Enable" : "Disable"), QSTRING_CSTR(component));

	// create command
	QJsonObject command, parameter;
	command["command"] = QString("componentstate");
	parameter["component"] = component;
	parameter["state"] = state;
	command["componentstate"] = parameter;

	// send command message
	QJsonObject reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

void JsonConnection::setSource(int priority)
{
	// create command
	QJsonObject command;
	command["command"] = QString("sourceselect");
	command["priority"] = priority;

	// send command message
	QJsonObject reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

void JsonConnection::setSourceAutoSelect()
{
	// create command
	QJsonObject command;
	command["command"] = QString("sourceselect");
	command["auto"] = true;

	// send command message
	QJsonObject reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

QString JsonConnection::getConfig(std::string type)
{
	assert( type == "schema" || type == "config" );
	Debug(_log, "Get configuration file from Hyperion Server");

	// create command
	QJsonObject command;
	command["command"] = QString("config");
	command["subcommand"] = (type == "schema") ? QString("getschema") : QString("getconfig");

	// send command message
	QJsonObject reply = sendMessage(command);

	// parse reply message
	if (parseReply(reply))
	{
		if (!reply.contains("info") || !reply["info"].isObject())
		{
			throw std::runtime_error("No configuration file available in reply");
		}

		QJsonDocument doc(reply["info"].toObject());
		QString result(doc.toJson(QJsonDocument::Indented));
		return result;
	}

	return QString();
}

void JsonConnection::setConfig(const QString &jsonString)
{
	// create command
	QJsonObject command;
	command["command"] = QString("config");
	command["subcommand"] = QString("setconfig");

	if (jsonString.size() > 0)
	{
		QJsonObject configObj;
		if(!JsonUtils::parse("hyperion-remote-args", jsonString, configObj, _log))
		{
			throw std::runtime_error("Error in configSet arguments, abort");
		}

		command["config"] = configObj;
	}

	// send command message
	QJsonObject reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

void JsonConnection::setAdjustment(
		const QString & adjustmentId,
		const QColor & redAdjustment,
		const QColor & greenAdjustment,
		const QColor & blueAdjustment,
		const QColor & cyanAdjustment,
		const QColor & magentaAdjustment,
		const QColor & yellowAdjustment,
		const QColor & whiteAdjustment,
		const QColor & blackAdjustment,
		double *gammaR,
		double *gammaG,
		double *gammaB,
		int    *backlightThreshold,
		int    *backlightColored,
		int    *brightness,
		int    *brightnessC)
{
	Debug(_log, "Set color adjustments");

	// create command
	QJsonObject command, adjust;
	command["command"] = QString("adjustment");

	if (!adjustmentId.isNull())
	{
		adjust["id"] = adjustmentId;
	}

	const auto addAdjustmentIfValid = [&] (const QString & name, const QColor & adjustment) {
		if (adjustment.isValid())
		{
			QJsonArray color;
			color.append(adjustment.red());
			color.append(adjustment.green());
			color.append(adjustment.blue());
			adjust[name] = color;
		}
	};

	addAdjustmentIfValid("red",     redAdjustment);
	addAdjustmentIfValid("green",   greenAdjustment);
	addAdjustmentIfValid("blue",    blueAdjustment);
	addAdjustmentIfValid("cyan",    cyanAdjustment);
	addAdjustmentIfValid("magenta", magentaAdjustment);
	addAdjustmentIfValid("yellow",  yellowAdjustment);
	addAdjustmentIfValid("white",   whiteAdjustment);
	addAdjustmentIfValid("black",   blackAdjustment);

	if (backlightThreshold != nullptr)
	{
		adjust["backlightThreshold"] = *backlightThreshold;
	}
	if (backlightColored != nullptr)
	{
		adjust["backlightColored"] = (*backlightColored == 0)? false : true;
	}
	if (brightness != nullptr)
	{
		adjust["brightness"] = *brightness;
	}
	if (brightnessC != nullptr)
	{
		adjust["brightnessCompensation"] = *brightnessC;
	}
	if (gammaR != nullptr)
	{
		adjust["gammaRed"] = *gammaR;
	}
	if (gammaG != nullptr)
	{
		adjust["gammaGreen"] = *gammaG;
	}
	if (gammaB != nullptr)
	{
		adjust["gammaBlue"] = *gammaB;
	}

	command["adjustment"] = adjust;

	// send command message
	QJsonObject reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}


void JsonConnection::setLedMapping(QString mappingType)
{
	QJsonObject command;
	command["command"] = QString("processing");
	command["mappingType"] = mappingType;

	QJsonObject reply = sendMessage(command);
	parseReply(reply);
}

void JsonConnection::setVideoMode(QString videoMode)
{
	QJsonObject command;
	command["command"] = QString("videomode");
	command["videoMode"] = videoMode.toUpper();

	QJsonObject reply = sendMessage(command);
	parseReply(reply);
}

void JsonConnection::setToken(const QString &token)
{
	// create command
	QJsonObject command;
	command["command"] = QString("authorize");
	command["subcommand"] = QString("login");

	if (token.size() < 36)
		throw std::runtime_error("The given token length is too short.");

	command["token"] = token;

	// send command message
	QJsonObject reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

void JsonConnection::setInstance(int instance)
{
	// create command
	QJsonObject command;
	command["command"] = QString("instance");
	command["subcommand"] = QString("switchTo");
	command["instance"] = instance;

	// send command message
	QJsonObject reply = sendMessage(command);

	// parse reply message
	parseReply(reply);
}

QJsonObject JsonConnection::sendMessage(const QJsonObject & message)
{
	// serialize message
	QJsonDocument writer(message);
	QByteArray serializedMessage = writer.toJson(QJsonDocument::Compact) + "\n";

	// print command if requested
	if (_printJson)
	{
		std::cout << "Command: " << serializedMessage.constData();
	}

	// write message
	_socket.write(serializedMessage);
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
	QJsonParseError error;
	QJsonDocument reply = QJsonDocument::fromJson(serializedReply ,&error);
	if (error.error != QJsonParseError::NoError)
	{
		throw std::runtime_error(
			std::string("Error while parsing json reply: ")
			+ error.errorString().toStdString() );
	}

	return reply.object();
}

bool JsonConnection::parseReply(const QJsonObject &reply)
{
	bool success = false;
	QString reason = "No error info";

	try
	{
		success = reply["success"].toBool(false);
		if (!success)
			reason = reply["error"].toString(reason);
	}
	catch (const std::runtime_error &)
	{
		// Some json parsing error: ignore and set parsing error
	}

	if (!success)
	{
		throw std::runtime_error("Error: " + reason.toStdString());
	}

	return success;
}
