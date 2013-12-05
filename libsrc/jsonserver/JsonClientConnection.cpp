// system includes
#include <stdexcept>
#include <cassert>

// stl includes
#include <iostream>
#include <sstream>
#include <iterator>

// Qt includes
#include <QResource>
#include <QDateTime>

// hyperion util includes
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>
#include <hyperion/ColorTransform.h>
#include <utils/ColorRgb.h>

// project includes
#include "JsonClientConnection.h"

JsonClientConnection::JsonClientConnection(QTcpSocket *socket, Hyperion * hyperion) :
	QObject(),
	_socket(socket),
	_imageProcessor(ImageProcessorFactory::getInstance().newImageProcessor()),
	_hyperion(hyperion),
	_receiveBuffer()
{
	// connect internal signals and slots
	connect(_socket, SIGNAL(disconnected()), this, SLOT(socketClosed()));
	connect(_socket, SIGNAL(readyRead()), this, SLOT(readData()));
}


JsonClientConnection::~JsonClientConnection()
{
	delete _socket;
}

void JsonClientConnection::readData()
{
	_receiveBuffer += _socket->readAll();

	int bytes = _receiveBuffer.indexOf('\n') + 1;
	while(bytes > 0)
	{
		// create message string
		std::string message(_receiveBuffer.data(), bytes);

		// remove message data from buffer
		_receiveBuffer = _receiveBuffer.mid(bytes);

		// handle message
		handleMessage(message);

		// try too look up '\n' again
		bytes = _receiveBuffer.indexOf('\n') + 1;
	}
}

void JsonClientConnection::socketClosed()
{
	emit connectionClosed(this);
}

void JsonClientConnection::handleMessage(const std::string &messageString)
{
	Json::Reader reader;
	Json::Value message;
	if (!reader.parse(messageString, message, false))
	{
		sendErrorReply("Error while parsing json: " + reader.getFormattedErrorMessages());
		return;
	}

	// check basic message
	std::string errors;
	if (!checkJson(message, ":schema", errors))
	{
		sendErrorReply("Error while validating json: " + errors);
		return;
	}

	// check specific message
	const std::string command = message["command"].asString();
	if (!checkJson(message, QString(":schema-%1").arg(QString::fromStdString(command)), errors))
	{
		sendErrorReply("Error while validating json: " + errors);
		return;
	}

	// switch over all possible commands and handle them
	if (command == "color")
		handleColorCommand(message);
	else if (command == "image")
		handleImageCommand(message);
	else if (command == "effect")
		handleEffectCommand(message);
	else if (command == "serverinfo")
		handleServerInfoCommand(message);
	else if (command == "clear")
		handleClearCommand(message);
	else if (command == "clearall")
		handleClearallCommand(message);
	else if (command == "transform")
		handleTransformCommand(message);
	else
		handleNotImplemented();
}

void JsonClientConnection::handleColorCommand(const Json::Value &message)
{
	// extract parameters
	int priority = message["priority"].asInt();
	int duration = message.get("duration", -1).asInt();
	ColorRgb color = {uint8_t(message["color"][0u].asInt()), uint8_t(message["color"][1u].asInt()), uint8_t(message["color"][2u].asInt())};

	// set output
	_hyperion->setColor(priority, color, duration);

	// send reply
	sendSuccessReply();
}

void JsonClientConnection::handleImageCommand(const Json::Value &message)
{
	// extract parameters
	int priority = message["priority"].asInt();
	int duration = message.get("duration", -1).asInt();
	int width = message["imagewidth"].asInt();
	int height = message["imageheight"].asInt();
	QByteArray data = QByteArray::fromBase64(QByteArray(message["imagedata"].asCString()));

	// check consistency of the size of the received data
	if (data.size() != width*height*3)
	{
		sendErrorReply("Size of image data does not match with the width and height");
		return;
	}

	// set width and height of the image processor
	_imageProcessor->setSize(width, height);

	// create ImageRgb
	Image<ColorRgb> image(width, height);
	memcpy(image.memptr(), data.data(), data.size());

	// process the image
	std::vector<ColorRgb> ledColors = _imageProcessor->process(image);
	_hyperion->setColors(priority, ledColors, duration);

	// send reply
	sendSuccessReply();
}

void JsonClientConnection::handleEffectCommand(const Json::Value &message)
{
	// extract parameters
	int priority = message["priority"].asInt();
	int duration = message.get("duration", -1).asInt();
	const Json::Value & effect = message["effect"];
	const std::string & effectName = effect["name"].asString();

	// set output
	if (effect.isMember("args"))
	{
		_hyperion->setEffect(effectName, effect["args"], priority, duration);
	}
	else
	{
		_hyperion->setEffect(effectName, priority, duration);
	}

	// send reply
	sendSuccessReply();
}

void JsonClientConnection::handleServerInfoCommand(const Json::Value &)
{
	// create result
	Json::Value result;
	result["success"] = true;
	Json::Value & info = result["info"];

	// collect priority information
	Json::Value & priorities = info["priorities"] = Json::Value(Json::arrayValue);
	uint64_t now = QDateTime::currentMSecsSinceEpoch();
	QList<int> activePriorities = _hyperion->getActivePriorities();
	foreach (int priority, activePriorities) {
		const Hyperion::InputInfo & priorityInfo = _hyperion->getPriorityInfo(priority);
		Json::Value & item = priorities[priorities.size()];
		item["priority"] = priority;
		if (priorityInfo.timeoutTime_ms != -1)
		{
			item["duration_ms"] = Json::Value::UInt(priorityInfo.timeoutTime_ms - now);
		}
	}

	// collect transform information
	Json::Value & transformArray = info["transform"];
	for (const std::string& transformId : _hyperion->getTransformIds())
	{
		const ColorTransform * colorTransform = _hyperion->getTransform(transformId);
		if (colorTransform == nullptr)
		{
			std::cerr << "Incorrect color transform id: " << transformId << std::endl;
			continue;
		}

		Json::Value & transform = transformArray.append(Json::Value());
		transform["id"] = transformId;

		transform["saturationGain"] = colorTransform->_hsvTransform.getSaturationGain();
		transform["valueGain"]      = colorTransform->_hsvTransform.getValueGain();

		Json::Value & threshold = transform["threshold"];
		threshold.append(colorTransform->_rgbRedTransform.getThreshold());
		threshold.append(colorTransform->_rgbGreenTransform.getThreshold());
		threshold.append(colorTransform->_rgbBlueTransform.getThreshold());
		Json::Value & gamma = transform["gamma"];
		gamma.append(colorTransform->_rgbRedTransform.getGamma());
		gamma.append(colorTransform->_rgbGreenTransform.getGamma());
		gamma.append(colorTransform->_rgbBlueTransform.getGamma());
		Json::Value & blacklevel = transform["blacklevel"];
		blacklevel.append(colorTransform->_rgbRedTransform.getBlacklevel());
		blacklevel.append(colorTransform->_rgbGreenTransform.getBlacklevel());
		blacklevel.append(colorTransform->_rgbBlueTransform.getBlacklevel());
		Json::Value & whitelevel = transform["whitelevel"];
		whitelevel.append(colorTransform->_rgbRedTransform.getWhitelevel());
		whitelevel.append(colorTransform->_rgbGreenTransform.getWhitelevel());
		whitelevel.append(colorTransform->_rgbBlueTransform.getWhitelevel());
	}

	// collect effect info
	Json::Value & effects = info["effects"] = Json::Value(Json::arrayValue);
	const std::list<EffectDefinition> & effectsDefinitions = _hyperion->getEffects();
	for (const EffectDefinition & effectDefinition : effectsDefinitions)
	{
		Json::Value effect;
		effect["name"] = effectDefinition.name;
		effect["script"] = effectDefinition.script;
		effect["args"] = effectDefinition.args;

		effects.append(effect);
	}

	// send the result
	sendMessage(result);
}

void JsonClientConnection::handleClearCommand(const Json::Value &message)
{
	// extract parameters
	int priority = message["priority"].asInt();

	// clear priority
	_hyperion->clear(priority);

	// send reply
	sendSuccessReply();
}

void JsonClientConnection::handleClearallCommand(const Json::Value &)
{
	// clear priority
	_hyperion->clearall();

	// send reply
	sendSuccessReply();
}

void JsonClientConnection::handleTransformCommand(const Json::Value &message)
{
	const Json::Value & transform = message["transform"];

	const std::string transformId = transform.get("id", _hyperion->getTransformIds().front()).asString();
	ColorTransform * colorTransform = _hyperion->getTransform(transformId);
	if (colorTransform == nullptr)
	{
		//sendErrorReply(std::string("Incorrect transform identifier: ") + transformId);
		return;
	}

	if (transform.isMember("saturationGain"))
	{
		colorTransform->_hsvTransform.setSaturationGain(transform["saturationGain"].asDouble());
	}

	if (transform.isMember("valueGain"))
	{
		colorTransform->_hsvTransform.setValueGain(transform["valueGain"].asDouble());
	}

	if (transform.isMember("threshold"))
	{
		const Json::Value & values = transform["threshold"];
		colorTransform->_rgbRedTransform  .setThreshold(values[0u].asDouble());
		colorTransform->_rgbGreenTransform.setThreshold(values[1u].asDouble());
		colorTransform->_rgbBlueTransform .setThreshold(values[2u].asDouble());
	}

	if (transform.isMember("gamma"))
	{
		const Json::Value & values = transform["gamma"];
		colorTransform->_rgbRedTransform  .setGamma(values[0u].asDouble());
		colorTransform->_rgbGreenTransform.setGamma(values[1u].asDouble());
		colorTransform->_rgbBlueTransform .setGamma(values[2u].asDouble());
	}

	if (transform.isMember("blacklevel"))
	{
		const Json::Value & values = transform["blacklevel"];
		colorTransform->_rgbRedTransform  .setBlacklevel(values[0u].asDouble());
		colorTransform->_rgbGreenTransform.setBlacklevel(values[1u].asDouble());
		colorTransform->_rgbBlueTransform .setBlacklevel(values[2u].asDouble());
	}

	if (transform.isMember("whitelevel"))
	{
		const Json::Value & values = transform["whitelevel"];
		colorTransform->_rgbRedTransform  .setWhitelevel(values[0u].asDouble());
		colorTransform->_rgbGreenTransform.setWhitelevel(values[1u].asDouble());
		colorTransform->_rgbBlueTransform .setWhitelevel(values[2u].asDouble());
	}

	// commit the changes
	_hyperion->transformsUpdated();

	sendSuccessReply();
}

void JsonClientConnection::handleNotImplemented()
{
	sendErrorReply("Command not implemented");
}

void JsonClientConnection::sendMessage(const Json::Value &message)
{
	Json::FastWriter writer;
	std::string serializedReply = writer.write(message);
	_socket->write(serializedReply.data(), serializedReply.length());
}

void JsonClientConnection::sendSuccessReply()
{
	// create reply
	Json::Value reply;
	reply["success"] = true;

	// send reply
	sendMessage(reply);
}

void JsonClientConnection::sendErrorReply(const std::string &error)
{
	// create reply
	Json::Value reply;
	reply["success"] = false;
	reply["error"] = error;

	// send reply
	sendMessage(reply);
}

bool JsonClientConnection::checkJson(const Json::Value & message, const QString & schemaResource, std::string & errorMessage)
{
	// read the json schema from the resource
	QResource schemaData(schemaResource);
	assert(schemaData.isValid());
	Json::Reader jsonReader;
	Json::Value schemaJson;
	if (!jsonReader.parse(reinterpret_cast<const char *>(schemaData.data()), reinterpret_cast<const char *>(schemaData.data()) + schemaData.size(), schemaJson, false))
	{
		throw std::runtime_error("Schema error: " + jsonReader.getFormattedErrorMessages())	;
	}

	// create schema checker
	JsonSchemaChecker schema;
	schema.setSchema(schemaJson);

	// check the message
	if (!schema.validate(message))
	{
		const std::list<std::string> & errors = schema.getMessages();
		std::stringstream ss;
		ss << "{";
		foreach (const std::string & error, errors) {
			ss << error << " ";
		}
		ss << "}";
		errorMessage = ss.str();
		return false;
	}

	return true;
}
