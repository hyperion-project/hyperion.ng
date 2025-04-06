// stl includes
#include <stdexcept>
#include <cassert>
#include <iostream>

// Qt includes
#include <QRgb>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QHostInfo>
#include <QUrl>
#include <QTcpSocket>
#include <QEventLoop>
#include <QTimer>

#include "HyperionConfig.h"

// hyperion-remote includes
#include "JsonConnection.h"

// util includes
#include <utils/JsonUtils.h>

JsonConnection::JsonConnection(const QHostAddress& host, bool printJson , quint16 port)
	: _log(Logger::getInstance("JSONAPICONN"))
	, _host(host)
	, _port(port)
	, _printJson(printJson)
{
	_socket.reset(new QTcpSocket(this));
	QObject::connect(_socket.get(), &QTcpSocket::connected, this, &JsonConnection::onConnected);
	QObject::connect(_socket.get(), &QTcpSocket::readyRead, this, &JsonConnection::onReadyRead);
	QObject::connect(_socket.get(), &QTcpSocket::disconnected, this, &JsonConnection::onDisconnected);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
	QObject::connect(_socket.get(), &QTcpSocket::errorOccurred, this, &JsonConnection::onErrorOccured);
#else
	QObject::connect(_socket.get(),
					 QOverload<QTcpSocket::SocketError>::of(&QTcpSocket::error),
					 this,
					 &JsonConnection::onErrorOccured);
#endif

	// init connect
	connectToRemoteHost();
}

JsonConnection::~JsonConnection()
{
}

void JsonConnection::connectToRemoteHost()
{
	Debug(_log, "Connecting to target host: %s, port [%u]", QSTRING_CSTR(_host.toString()), _port);
	_socket->connectToHost(_host, _port);
}

void JsonConnection::close()
{
	_socket->close();
}

void JsonConnection::onErrorOccured()
{
	QString errorText = QString("Unable to connect to host: %1, port: %2, error: %3").arg(_host.toString()).arg(_port).arg(_socket->errorString());
	emit errorOccured(errorText);
}

void JsonConnection::onDisconnected()
{
	Debug(_log, "Disconnected from target host: %s, port [%u]", QSTRING_CSTR(_host.toString()), _port);
	emit isDisconnected();
}

void JsonConnection::onConnected()
{
	Debug(_log, "Connected to target host: %s, port [%u]", QSTRING_CSTR(_host.toString()), _port);
	emit isReadyToSend();
}

void JsonConnection::setColor(const QList<QColor>& colors, int priority, int duration)
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

	sendMessageSync(command);
}

void JsonConnection::setImage(const QImage& image, int priority, int duration, const QString& name)
{
	Debug(_log, "Set image has size: %dx%d", image.width(), image.height());

	// ensure the image has RGB888 format
	QImage imageARGB32 = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
	QByteArray binaryImage;
	binaryImage.reserve(static_cast<qsizetype>(imageARGB32.width()) *
						static_cast<qsizetype>(imageARGB32.height()) * 3);
	for (int i = 0; i < imageARGB32.height(); ++i)
	{
		const QRgb * scanline = reinterpret_cast<const QRgb *>(imageARGB32.scanLine(i));
		for (int j = 0; j < imageARGB32.width(); ++j)
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
	command["imagewidth"] = imageARGB32.width();
	command["imageheight"] = imageARGB32.height();
	command["imagedata"] = QString(base64Image.data());
	if (duration > 0)
	{
		command["duration"] = duration;
	}

	sendMessageSync(command);
}

#if defined(ENABLE_EFFECTENGINE)
void JsonConnection::setEffect(const QString &effectName, const QString & effectArgs, int priority, int duration)
{
	Debug(_log, "Start effect: %s", QSTRING_CSTR(effectName));

	// create command
	QJsonObject command;
	QJsonObject effect;
	command["command"] = QString("effect");
	command["origin"] = QString("hyperion-remote");
	command["priority"] = priority;
	effect["name"] = effectName;

	if (effectArgs.size() > 0)
	{
		QJsonObject effObj;
		if(!JsonUtils::parse("hyperion-remote-args", effectArgs, effObj, _log).first)
		{
			emit errorOccured("Error in effect arguments, abort");
		}

		effect["args"] = effObj;
	}

	command["effect"] = effect;

	if (duration > 0)
	{
		command["duration"] = duration;
	}

	sendMessageSync(command);
}

void JsonConnection::createEffect(const QString &effectName, const QString &effectScript, const QString & effectArgs)
{
	Debug(_log, "Create effect: %s", QSTRING_CSTR(effectName));

	// create command
	QJsonObject effectCmd;
	effectCmd["command"] = QString("create-effect");
	effectCmd["name"] = effectName;
	effectCmd["script"] = effectScript;

	if (effectArgs.size() > 0)
	{
		QJsonObject effObj;
		if(!JsonUtils::parse("hyperion-remote-args", effectScript, effObj, _log).first)
		{
			emit errorOccured("Error in effect arguments, abort");
		}

		effectCmd["args"] = effObj;
	}

	sendMessageSync(effectCmd);
}

void JsonConnection::deleteEffect(const QString &effectName)
{
	Debug(_log, "Delete effect configuration: %s", QSTRING_CSTR(effectName));

	// create command
	QJsonObject effectCmd;
	effectCmd["command"] = QString("delete-effect");
	effectCmd["name"] = effectName;

	sendMessageSync(effectCmd);
}
#endif

QString JsonConnection::getServerInfoString()
{
	return JsonUtils::jsonValueToQString(getServerInfo(), QJsonDocument::Indented);
}

QJsonObject JsonConnection::getServerInfo()
{
	Debug(_log, "Get server info");

	// create command
	QJsonObject command;
	command["command"] = QString("serverinfo");


	QJsonObject reply = sendMessageSync(command);
	if (!reply.contains("info") || !reply["info"].isObject())
	{
		emit errorOccured("No info available in response to request");
	}

	return reply.value("info").toObject();
}

QString JsonConnection::getSysInfo()
{
	Debug(_log, "Get system info");

	// create command
	QJsonObject command;
	command["command"] = QString("sysinfo");

	QJsonObject reply = sendMessageSync(command);
	if (!reply.contains("info") || !reply["info"].isObject())
	{
		emit errorOccured("No info available in response to request");
	}

	return JsonUtils::jsonValueToQString(reply["info"], QJsonDocument::Indented);
}

void JsonConnection::suspend()
{
	Info(_log, "Suspend Hyperion. Stop all instances and components");
	QJsonObject command;
	command["command"] = QString("system");
	command["subcommand"] = QString("suspend");

	sendMessageSync(command);
}

void JsonConnection::resume()
{
	Info(_log, "Resume Hyperion. Start all instances and components");
	QJsonObject command;
	command["command"] = QString("system");
	command["subcommand"] = QString("resume");

	sendMessageSync(command);
}

void JsonConnection::toggleSuspend()
{
	Info(_log, "Toggle between Suspend and Resume");
	QJsonObject command;
	command["command"] = QString("system");
	command["subcommand"] = QString("toggleSuspend");

	sendMessageSync(command);
}

void JsonConnection::idle()
{
	Info(_log, "Put Hyperion in Idle mode.");
	QJsonObject command;
	command["command"] = QString("system");
	command["subcommand"] = QString("idle");

	sendMessageSync(command);
}

void JsonConnection::toggleIdle()
{
	Info(_log, "Toggle between Idle and Working mode");
	QJsonObject command;
	command["command"] = QString("system");
	command["subcommand"] = QString("toggleIdle");

	sendMessageSync(command);
}

void JsonConnection::restart()
{
	Info(_log, "Restart Hyperion...");
	QJsonObject command;
	command["command"] = QString("system");
	command["subcommand"] = QString("restart");

	sendMessageSync(command);
}

void JsonConnection::clear(int priority)
{
	Debug(_log, "Clear priority channel [%d]", priority);

	// create command
	QJsonObject command;
	command["command"] = QString("clear");
	command["priority"] = priority;

	sendMessageSync(command);
}

void JsonConnection::clearAll()
{
	Debug(_log, "Clear all priority channels");

	// create command
	QJsonObject command;
	command["command"] = QString("clear");
	command["priority"] = -1;

	sendMessageSync(command);
}

void JsonConnection::setComponentState(const QString & component, bool state)
{
	Debug(_log, "%s Component: %s", (state ? "Enable" : "Disable"), QSTRING_CSTR(component));

	// create command
	QJsonObject command;
	QJsonObject parameter;
	command["command"] = QString("componentstate");
	parameter["component"] = component;
	parameter["state"] = state;
	command["componentstate"] = parameter;

	sendMessageSync(command);
}

void JsonConnection::setSource(int priority)
{
	// create command
	QJsonObject command;
	command["command"] = QString("sourceselect");
	command["priority"] = priority;

	sendMessageSync(command);
}

void JsonConnection::setSourceAutoSelect()
{
	// create command
	QJsonObject command;
	command["command"] = QString("sourceselect");
	command["auto"] = true;

	sendMessageSync(command);
}

QString JsonConnection::getConfig(const QString& type)
{
	assert( type == "schema" || type == "config" );
	Debug(_log, "Get configuration file from Hyperion Server");

	// create command
	QJsonObject command;
	command["command"] = QString("config");
	command["subcommand"] = (type == "schema") ? "getschema" : "getconfig";

	QJsonObject reply = sendMessageSync(command);
	if (!reply.contains("info") || !reply["info"].isObject())
	{
		emit errorOccured("No configuration file available in reply");
	}

	return JsonUtils::jsonValueToQString(reply["info"], QJsonDocument::Indented);
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
		if(!JsonUtils::parse("hyperion-remote-args", jsonString, configObj, _log).first)
		{
			emit errorOccured("Error in configSet arguments, abort");
		}

		command["config"] = configObj;
	}

	sendMessageSync(command);
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
	QJsonObject command;
	QJsonObject adjust;
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
		adjust["backlightColored"] = *backlightColored != 0;
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

	sendMessageSync(command);
}

void JsonConnection::setLedMapping(const QString& mappingType)
{
	QJsonObject command;
	command["command"] = QString("processing");
	command["mappingType"] = mappingType;

	sendMessageSync(command);
}

void JsonConnection::setVideoMode(const QString& videoMode)
{
	QJsonObject command;
	command["command"] = QString("videomode");
	command["videoMode"] = videoMode.toUpper();

	sendMessageSync(command);
}

bool JsonConnection::setToken(const QString& token)
{
	// create command
	QJsonObject command;
	command["command"] = QString("authorize");
	command["subcommand"] = QString("login");

	if (token.size() < 36)
	{
		emit errorOccured("The given token's length is too short.");
		return false;
	}

	command["token"] = token;

	QJsonObject const reply = sendMessageSync(command);
	return !reply.isEmpty();
}

bool JsonConnection::setInstance(int instance)
{
	// create command
	QJsonObject command;
	command["command"] = QString("instance");
	command["subcommand"] = QString("switchTo");
	command["instance"] = instance;

	QJsonObject const reply = sendMessageSync(command);
	return !reply.isEmpty();
}

void JsonConnection::onReadyRead()
{
	QByteArray const serializedReply = _socket->readAll();
	QList<QByteArray> replies = serializedReply.trimmed().split('\n');

	bool isParsingError {false};
	QList<QString> errorList;

	for (const QByteArray &reply : std::as_const(replies))
	{
		QJsonObject response;

		const QString ident = "RemoteTarget@" + _socket->peerAddress().toString();
		QPair<bool, QStringList> const parsingResult = JsonUtils::parse(ident, reply, response, _log);
		if (!parsingResult.first)
		{
			isParsingError = true;
			errorList.append(parsingResult.second);
		}
		else
		{
			if (!isParsingError)
			{
				emit isMessageReceived(response);
				break;
			}
		}
	}

	if (isParsingError)
	{
		QString const errorText = errorList.join(";");;
		emit errorOccured(errorText);
	}
}

QJsonObject JsonConnection::sendMessageSync(const QJsonObject &message, const QJsonArray& instanceIds)
{
	QEventLoop loop;
	QJsonObject response;
	bool received = false;

	// Ensure no duplicate connections by disconnecting previous ones
	QObject::disconnect(this, &JsonConnection::isMessageReceived, nullptr, nullptr);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
	QObject::disconnect(_socket.get(), &QTcpSocket::errorOccurred, nullptr, nullptr);
#else
	QObject::disconnect(_socket.get(),
					 QOverload<QTcpSocket::SocketError>::of(&QTcpSocket::error),
					 nullptr, nullptr);
#endif

	// Capture response when received
	QObject::connect(this, &JsonConnection::isMessageReceived, this, [&](const QJsonObject &reply) {
		response = reply;
		received = true;
		loop.quit(); // Exit event loop when response arrives
	});

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
	QObject::connect(_socket.get(), &QTcpSocket::errorOccurred, this, [&loop](QTcpSocket::SocketError  /*error*/) {
#else
	QObject::connect(_socket.get(), QOverload<QTcpSocket::SocketError>::of(&QTcpSocket::error),
					 [&loop](QTcpSocket::SocketError  /*error*/) {
#endif
		loop.quit();
	});

	// Send the message asynchronously
	sendMessage(message, instanceIds);

	// Timeout mechanism (prevents infinite blocking)
	QTimer timer;
	timer.setSingleShot(true);
	connect(&timer, &QTimer::timeout, this, [&]() {
		loop.quit();
	});

	timer.start(5000);
	loop.exec(); // Start event loop

	return received ? response : QJsonObject(); // Return response if received
}

void JsonConnection::sendMessage(const QJsonObject & message, const QJsonArray& instanceIds)
{
	// for hyperion classic compatibility
	QJsonObject jsonMessage = message;
	if (!instanceIds.empty())
	{
		jsonMessage["instance"] = instanceIds;
	}

	QJsonDocument const writer(jsonMessage);
	QByteArray const serializedMessage = writer.toJson(QJsonDocument::Compact) + "\n";

	// print command if requested
	if (_printJson)
	{
		std::cout << "Command: " << serializedMessage.constData();
	}

	// write message
	_socket->write(serializedMessage);
}
