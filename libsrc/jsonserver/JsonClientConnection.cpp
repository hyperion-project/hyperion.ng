// system includes
#include <stdexcept>
#include <cassert>
#include <iomanip>

// stl includes
#include <iostream>
#include <sstream>
#include <iterator>

// Qt includes
#include <QResource>
#include <QDateTime>
#include <QCryptographicHash>
#include <QHostInfo>
#include <QString>
#include <QFile>

// hyperion util includes
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>
#include <hyperion/MessageForwarder.h>
#include <hyperion/ColorTransform.h>
#include <hyperion/ColorCorrection.h>
#include <hyperion/ColorAdjustment.h>
#include <utils/ColorRgb.h>
#include <leddevice/LedDevice.h>
#include <HyperionConfig.h>
#include <utils/jsonschema/JsonFactory.h>

// project includes
#include "JsonClientConnection.h"

using namespace hyperion;

JsonClientConnection::JsonClientConnection(QTcpSocket *socket)
	: QObject()
	, _socket(socket)
	, _imageProcessor(ImageProcessorFactory::getInstance().newImageProcessor())
	, _hyperion(Hyperion::getInstance())
	, _receiveBuffer()
	, _webSocketHandshakeDone(false)
	, _log(Logger::getInstance("JSONCLIENTCONNECTION"))
	, _forwarder_enabled(true)
{
	// connect internal signals and slots
	connect(_socket, SIGNAL(disconnected()), this, SLOT(socketClosed()));
	connect(_socket, SIGNAL(readyRead()), this, SLOT(readData()));
	connect(_hyperion, SIGNAL(componentStateChanged(hyperion::Components,bool)), this, SLOT(componentStateChanged(hyperion::Components,bool)));
	
	_timer_ledcolors.setSingleShot(false);
	connect(&_timer_ledcolors, SIGNAL(timeout()), this, SLOT(streamLedcolorsUpdate()));
}


JsonClientConnection::~JsonClientConnection()
{
	delete _socket;
}

void JsonClientConnection::readData()
{
	_receiveBuffer += _socket->readAll();
	
	if (_webSocketHandshakeDone)
	{
		// websocket mode, data frame
		handleWebSocketFrame();
	} else
	{
		// might be a handshake request or raw socket data
		if(_receiveBuffer.contains("Upgrade: websocket"))
		{
			doWebSocketHandshake();
		} else 
		{
			// raw socket data, handling as usual
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
	}
}

void JsonClientConnection::handleWebSocketFrame()
{
	if ((_receiveBuffer.at(0) & 0x80) == 0x80)
	{
		// final bit found, frame complete
		quint8 * maskKey = NULL;
		quint8 opCode = _receiveBuffer.at(0) & 0x0F;
		bool isMasked = (_receiveBuffer.at(1) & 0x80) == 0x80;
		quint64 payloadLength = _receiveBuffer.at(1) & 0x7F;
		quint32 index = 2;
		
		switch (payloadLength)
		{
			case 126:
				payloadLength = ((_receiveBuffer.at(2) << 8) & 0xFF00) | (_receiveBuffer.at(3) & 0xFF);
				index += 2;
				break;
			case 127:
				payloadLength = 0;
				for (uint i=0; i < 8; i++)						{
					payloadLength |= ((quint64)(_receiveBuffer.at(index+i) & 0xFF)) << (8*(7-i));
				}
				index += 8;
				break;
			default:
				break;
		}
		
		if (isMasked)
		{
			// if the data is masked we need to get the key for unmasking
			maskKey = new quint8[4];
			for (uint i=0; i < 4; i++)
			{
				maskKey[i] = _receiveBuffer.at(index + i);	
			}
			index += 4;
		}
		
		// check the type of data frame
		switch (opCode)
		{
		case 0x01:
			{
				// frame contains text, extract it
				QByteArray result = _receiveBuffer.mid(index, payloadLength);
				_receiveBuffer.clear();
			
				// unmask data if necessary
				if (isMasked)
				{
					for (uint i=0; i < payloadLength; i++)
					{
						result[i] = (result[i] ^ maskKey[i % 4]);
					}
					if (maskKey != NULL)
					{
						delete[] maskKey;
						maskKey = NULL;
					}
				}
								
				handleMessage(QString(result).toStdString());
			}
			break;
		case 0x08:
			{
				// close request, confirm
				quint8 close[] = {0x88, 0};				
				_socket->write((const char*)close, 2);
				_socket->flush();
				_socket->close();
			}
			break;
		case 0x09:
			{
				// ping received, send pong
				quint8 pong[] = {0x0A, 0};				
				_socket->write((const char*)pong, 2);
				_socket->flush();
			}
			break;
		}
	} else
	{
		Error(_log, "Someone is sending very big messages over several frames... it's not supported yet");
		quint8 close[] = {0x88, 0};				
		_socket->write((const char*)close, 2);
		_socket->flush();
		_socket->close();
	}
}

void JsonClientConnection::doWebSocketHandshake()
{
	// http header, might not be a very reliable check...
	Debug(_log, "Websocket handshake");

	// get the key to prepare an answer
	int start = _receiveBuffer.indexOf("Sec-WebSocket-Key") + 19;
	std::string value(_receiveBuffer.mid(start, _receiveBuffer.indexOf("\r\n", start) - start).data());
	_receiveBuffer.clear();

	// must be always appended
	value += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

	// generate sha1 hash
	QByteArray hash = QCryptographicHash::hash(value.c_str(), QCryptographicHash::Sha1);

	// prepare an answer
	std::ostringstream h;
	h << "HTTP/1.1 101 Switching Protocols\r\n" <<
	"Upgrade: websocket\r\n" <<
	"Connection: Upgrade\r\n" << 
	"Sec-WebSocket-Accept: " << QString(hash.toBase64()).toStdString() << "\r\n\r\n";

	_socket->write(h.str().c_str());
	_socket->flush();
	// we are in WebSocket mode, data frames should follow next
	_webSocketHandshakeDone = true;
}

void JsonClientConnection::socketClosed()
{
	_webSocketHandshakeDone = false;
	emit connectionClosed(this);
}

void JsonClientConnection::handleMessage(const std::string &messageString)
{
	Json::Reader reader;
	Json::Value message;
	std::string errors;
 	try
 	{
		if (!reader.parse(messageString, message, false))
		{
			sendErrorReply("Error while parsing json: " + reader.getFormattedErrorMessages());
			return;
		}

		// check basic message
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

		int tan = message.get("tan",0).asInt();
		// switch over all possible commands and handle them
		if (command == "color")
			handleColorCommand(message, command, tan);
		else if (command == "image")
			handleImageCommand(message, command, tan);
		else if (command == "effect")
			handleEffectCommand(message, command, tan);
		else if (command == "serverinfo")
			handleServerInfoCommand(message, command, tan);
		else if (command == "clear")
			handleClearCommand(message, command, tan);
		else if (command == "clearall")
			handleClearallCommand(message, command, tan);
		else if (command == "transform")
			handleTransformCommand(message, command, tan);
		else if (command == "temperature")
			handleTemperatureCommand(message, command, tan);
		else if (command == "adjustment")
			handleAdjustmentCommand(message, command, tan);
		else if (command == "sourceselect")
			handleSourceSelectCommand(message, command, tan);
		else if (command == "config")
			handleConfigCommand(message, command, tan);
		else if (command == "componentstate")
			handleComponentStateCommand(message, command, tan);
		else if (command == "ledcolors")
			handleLedColorsCommand(message, command, tan);
		else
			handleNotImplemented();
 	}
 	catch (std::exception& e)
 	{
 		sendErrorReply("Error while processing incoming json message: " + std::string(e.what()) + " " + errors );
 		Warning(_log, "Error while processing incoming json message: %s (%s)", e.what(), errors.c_str());
 	}

}

void JsonClientConnection::componentStateChanged(const hyperion::Components component, bool enable)
{
	if (component == COMP_FORWARDER && _forwarder_enabled != enable)
	{
		_forwarder_enabled = enable;
		Info(_log, "forwarder change state to %s", (enable ? "enabled" : "disabled") );
	}
}

void JsonClientConnection::forwardJsonMessage(const Json::Value & message)
{
	if (_forwarder_enabled)
	{
		QTcpSocket client;
		QList<MessageForwarder::JsonSlaveAddress> list = _hyperion->getForwarder()->getJsonSlaves();

		for ( int i=0; i<list.size(); i++ )
		{
			client.connectToHost(list.at(i).addr, list.at(i).port);
			if ( client.waitForConnected(500) )
			{
				sendMessage(message,&client);
				client.close();
			}
		}
	}
}

void JsonClientConnection::handleColorCommand(const Json::Value &message, const std::string &command, const int tan)
{
	forwardJsonMessage(message);

	// extract parameters
	int priority = message["priority"].asInt();
	int duration = message.get("duration", -1).asInt();

	std::vector<ColorRgb> colorData(_hyperion->getLedCount());
	const Json::Value & jsonColor = message["color"];
	Json::UInt i = 0;
	for (; i < jsonColor.size()/3 && i < _hyperion->getLedCount(); ++i)
	{
		colorData[i].red = uint8_t(message["color"][3u*i].asInt());
		colorData[i].green = uint8_t(message["color"][3u*i+1u].asInt());
		colorData[i].blue = uint8_t(message["color"][3u*i+2u].asInt());
	}

	// copy full blocks of led colors
	unsigned size = i;
	while (i + size < _hyperion->getLedCount())
	{
		memcpy(&(colorData[i]), colorData.data(), size * sizeof(ColorRgb));
		i += size;
	}

	// copy remaining block of led colors
	if (i < _hyperion->getLedCount())
	{
		memcpy(&(colorData[i]), colorData.data(), (_hyperion->getLedCount()-i) * sizeof(ColorRgb));
	}

	// set output
	_hyperion->setColors(priority, colorData, duration);

	// send reply
	sendSuccessReply(command, tan);
}

void JsonClientConnection::handleImageCommand(const Json::Value &message, const std::string &command, const int tan)
{
	forwardJsonMessage(message);

	// extract parameters
	int priority = message["priority"].asInt();
	int duration = message.get("duration", -1).asInt();
	int width = message["imagewidth"].asInt();
	int height = message["imageheight"].asInt();
	QByteArray data = QByteArray::fromBase64(QByteArray(message["imagedata"].asCString()));

	// check consistency of the size of the received data
	if (data.size() != width*height*3)
	{
		sendErrorReply("Size of image data does not match with the width and height", command, tan);
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
	sendSuccessReply(command, tan);
}

void JsonClientConnection::handleEffectCommand(const Json::Value &message, const std::string &command, const int tan)
{
	forwardJsonMessage(message);

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
	sendSuccessReply(command, tan);
}

void JsonClientConnection::handleServerInfoCommand(const Json::Value &, const std::string &command, const int tan)
{
	// create result
	Json::Value result;
	result["success"] = true;
	result["command"] = command;
	result["tan"] = tan;
	Json::Value & info = result["info"];
	
	// add host name for remote clients
	info["hostname"] = QHostInfo::localHostName().toStdString();

	// collect priority information
	Json::Value & priorities = info["priorities"] = Json::Value(Json::arrayValue);
	uint64_t now = QDateTime::currentMSecsSinceEpoch();
	QList<int> activePriorities = _hyperion->getActivePriorities();
	Hyperion::PriorityRegister priorityRegister = _hyperion->getPriorityRegister();
	int currentPriority = _hyperion->getCurrentPriority();
	foreach (int priority, activePriorities) {
		const Hyperion::InputInfo & priorityInfo = _hyperion->getPriorityInfo(priority);
		Json::Value & item = priorities[priorities.size()];
		item["priority"] = priority;
		if (priorityInfo.timeoutTime_ms != -1)
		{
			item["duration_ms"] = Json::Value::UInt(priorityInfo.timeoutTime_ms - now);
		}
		
		item["owner"] = "unknown";
		item["active"] = true;
		item["visible"] = (priority == currentPriority);
		foreach(auto const &entry, priorityRegister)
		{
			if (entry.second == priority)
			{
				item["owner"] = entry.first;
				priorityRegister.erase(entry.first);
				break;
			}
		}
	}
	foreach(auto const &entry, priorityRegister)
	{
		Json::Value & item = priorities[priorities.size()];
		item["priority"] = entry.second;
		item["active"] = false;
		item["visible"] = false;
		item["owner"] = entry.first;
	}
	
	// collect temperature correction information
	Json::Value & temperatureArray = info["temperature"];
	for (const std::string& tempId : _hyperion->getTemperatureIds())
	{
		const ColorCorrection * colorTemp = _hyperion->getTemperature(tempId);
		if (colorTemp == nullptr)
		{
			Error(_log, "Incorrect color temperature correction id: %s", tempId.c_str());
			continue;
		}

		Json::Value & temperature = temperatureArray.append(Json::Value());
		temperature["id"] = tempId;
		
		Json::Value & tempValues = temperature["correctionValues"];
		tempValues.append(colorTemp->_rgbCorrection.getAdjustmentR());
		tempValues.append(colorTemp->_rgbCorrection.getAdjustmentG());
		tempValues.append(colorTemp->_rgbCorrection.getAdjustmentB());
	}


	// collect transform information
	Json::Value & transformArray = info["transform"];
	for (const std::string& transformId : _hyperion->getTransformIds())
	{
		const ColorTransform * colorTransform = _hyperion->getTransform(transformId);
		if (colorTransform == nullptr)
		{
			Error(_log, "Incorrect color transform id: %s", transformId.c_str());
			continue;
		}

		Json::Value & transform = transformArray.append(Json::Value());
		transform["id"] = transformId;

		transform["saturationGain"] = colorTransform->_hsvTransform.getSaturationGain();
		transform["valueGain"]      = colorTransform->_hsvTransform.getValueGain();
		transform["saturationLGain"] = colorTransform->_hslTransform.getSaturationGain();
		transform["luminanceGain"]   = colorTransform->_hslTransform.getLuminanceGain();
		transform["luminanceMinimum"]   = colorTransform->_hslTransform.getLuminanceMinimum();

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
	
	// collect adjustment information
	Json::Value & adjustmentArray = info["adjustment"];
	for (const std::string& adjustmentId : _hyperion->getAdjustmentIds())
	{
		const ColorAdjustment * colorAdjustment = _hyperion->getAdjustment(adjustmentId);
		if (colorAdjustment == nullptr)
		{
			Error(_log, "Incorrect color adjustment id: %s", adjustmentId.c_str());
			continue;
		}

		Json::Value & adjustment = adjustmentArray.append(Json::Value());
		adjustment["id"] = adjustmentId;

		Json::Value & redAdjust = adjustment["redAdjust"];
		redAdjust.append(colorAdjustment->_rgbRedAdjustment.getAdjustmentR());
		redAdjust.append(colorAdjustment->_rgbRedAdjustment.getAdjustmentG());
		redAdjust.append(colorAdjustment->_rgbRedAdjustment.getAdjustmentB());
		Json::Value & greenAdjust = adjustment["greenAdjust"];
		greenAdjust.append(colorAdjustment->_rgbGreenAdjustment.getAdjustmentR());
		greenAdjust.append(colorAdjustment->_rgbGreenAdjustment.getAdjustmentG());
		greenAdjust.append(colorAdjustment->_rgbGreenAdjustment.getAdjustmentB());
		Json::Value & blueAdjust = adjustment["blueAdjust"];
		blueAdjust.append(colorAdjustment->_rgbBlueAdjustment.getAdjustmentR());
		blueAdjust.append(colorAdjustment->_rgbBlueAdjustment.getAdjustmentG());
		blueAdjust.append(colorAdjustment->_rgbBlueAdjustment.getAdjustmentB());
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
	
	// collect active effect info
	Json::Value & activeEffects = info["activeEffects"] = Json::Value(Json::arrayValue);
	const std::list<ActiveEffectDefinition> & activeEffectsDefinitions = _hyperion->getActiveEffects();
	for (const ActiveEffectDefinition & activeEffectDefinition : activeEffectsDefinitions)
	{
		if (activeEffectDefinition.priority != PriorityMuxer::LOWEST_PRIORITY -1)
		{
			Json::Value activeEffect;
			activeEffect["script"] = activeEffectDefinition.script;
			activeEffect["priority"] = activeEffectDefinition.priority;
			activeEffect["timeout"] = activeEffectDefinition.timeout;
			activeEffect["args"] = activeEffectDefinition.args;
	
			activeEffects.append(activeEffect);
		}
	}
	
	////////////////////////////////////
	// collect active static led color//
	////////////////////////////////////
	
	// create New JSON Array Value "activeLEDColor"
	Json::Value & activeLedColors = info["activeLedColor"] = Json::Value(Json::arrayValue);
	// get current Priority from Hyperion Muxer
	const Hyperion::InputInfo & priorityInfo = _hyperion->getPriorityInfo(_hyperion->getCurrentPriority());
	// check if current Priority exist
	if (priorityInfo.priority != std::numeric_limits<int>::max())
	{
		Json::Value LEDcolor;
		// check if all LEDs has the same Color
		if (std::all_of(priorityInfo.ledColors.begin(), priorityInfo.ledColors.end(), [&](ColorRgb color)
		{
			return ((color.red == priorityInfo.ledColors.begin()->red) &&
				(color.green == priorityInfo.ledColors.begin()->green) &&
				(color.blue == priorityInfo.ledColors.begin()->blue));
		} ))
	    {
		// check if LED Color not Black (0,0,0)
		if ((priorityInfo.ledColors.begin()->red +
		priorityInfo.ledColors.begin()->green +
		priorityInfo.ledColors.begin()->blue != 0))
		{
			// add RGB Value to Array
			LEDcolor["RGB Value"].append(priorityInfo.ledColors.begin()->red);
			LEDcolor["RGB Value"].append(priorityInfo.ledColors.begin()->green);
			LEDcolor["RGB Value"].append(priorityInfo.ledColors.begin()->blue);

			uint16_t Hue;
			float Saturation, Luminace;
		    
			// add HSL Value to Array
			HslTransform::rgb2hsl(priorityInfo.ledColors.begin()->red,
					priorityInfo.ledColors.begin()->green,
					priorityInfo.ledColors.begin()->blue,
					Hue, Saturation, Luminace);

			LEDcolor["HSL Value"].append(Hue);
			LEDcolor["HSL Value"].append(Saturation);
			LEDcolor["HSL Value"].append(Luminace);

			// add HEX Value to Array
			std::stringstream hex;
				hex << "0x"
				<< std::uppercase << std::setw(2) << std::setfill('0')
				<< std::hex << unsigned(priorityInfo.ledColors.begin()->red)
				<< std::uppercase << std::setw(2) << std::setfill('0')
				<< std::hex << unsigned(priorityInfo.ledColors.begin()->green)
				<< std::uppercase << std::setw(2) << std::setfill('0')
				<< std::hex << unsigned(priorityInfo.ledColors.begin()->blue);

			LEDcolor["HEX Value"].append(hex.str());

			activeLedColors.append(LEDcolor);
			}
		}
	}

	// get available led devices
	info["ledDevices"]["active"]    = LedDevice::activeDevice();
	info["ledDevices"]["available"] = Json::Value(Json::arrayValue);
	for ( auto dev: LedDevice::getDeviceMap())
	{
		info["ledDevices"]["available"].append(dev.first);
	}

	
	// Add Hyperion Version, build time
	//Json::Value & version = 
	info["hyperion"] = Json::Value(Json::arrayValue);
	Json::Value ver;
	ver["jsonrpc_version"] = HYPERION_JSON_VERSION;
	ver["version"] = HYPERION_VERSION;
	ver["build"]   = HYPERION_BUILD_ID;
	ver["time"]    = __DATE__ " " __TIME__;

	info["hyperion"].append(ver);

	// send the result
	sendMessage(result);
}

void JsonClientConnection::handleClearCommand(const Json::Value &message, const std::string &command, const int tan)
{
	forwardJsonMessage(message);

	// extract parameters
	int priority = message["priority"].asInt();

	// clear priority
	_hyperion->clear(priority);

	// send reply
	sendSuccessReply(command, tan);
}

void JsonClientConnection::handleClearallCommand(const Json::Value & message, const std::string &command, const int tan)
{
	forwardJsonMessage(message);

	// clear priority
	_hyperion->clearall();

	// send reply
	sendSuccessReply(command, tan);
}

void JsonClientConnection::handleTransformCommand(const Json::Value &message, const std::string &command, const int tan)
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
	
	if (transform.isMember("saturationLGain"))
	{
		colorTransform->_hslTransform.setSaturationGain(transform["saturationLGain"].asDouble());
	}

	if (transform.isMember("luminanceGain"))
	{
		colorTransform->_hslTransform.setLuminanceGain(transform["luminanceGain"].asDouble());
	}

	if (transform.isMember("luminanceMinimum"))
	{
		colorTransform->_hslTransform.setLuminanceMinimum(transform["luminanceMinimum"].asDouble());
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

	sendSuccessReply(command, tan);
}


void JsonClientConnection::handleTemperatureCommand(const Json::Value &message, const std::string &command, const int tan)
{
	const Json::Value & temperature = message["temperature"];

	const std::string tempId = temperature.get("id", _hyperion->getTemperatureIds().front()).asString();
	ColorCorrection * colorTemperature = _hyperion->getTemperature(tempId);
	if (colorTemperature == nullptr)
	{
		//sendErrorReply(std::string("Incorrect temperature identifier: ") + tempId);
		return;
	}

	if (temperature.isMember("correctionValues"))
	{
		const Json::Value & values = temperature["correctionValues"];
		colorTemperature->_rgbCorrection.setAdjustmentR(values[0u].asInt());
		colorTemperature->_rgbCorrection.setAdjustmentG(values[1u].asInt());
		colorTemperature->_rgbCorrection.setAdjustmentB(values[2u].asInt());
	}
	
	// commit the changes
	_hyperion->temperaturesUpdated();

	sendSuccessReply(command, tan);
}

void JsonClientConnection::handleAdjustmentCommand(const Json::Value &message, const std::string &command, const int tan)
{
	const Json::Value & adjustment = message["adjustment"];

	const std::string adjustmentId = adjustment.get("id", _hyperion->getAdjustmentIds().front()).asString();
	ColorAdjustment * colorAdjustment = _hyperion->getAdjustment(adjustmentId);
	if (colorAdjustment == nullptr)
	{
		//sendErrorReply(std::string("Incorrect transform identifier: ") + transformId);
		return;
	}
		
	if (adjustment.isMember("redAdjust"))
	{
		const Json::Value & values = adjustment["redAdjust"];
		colorAdjustment->_rgbRedAdjustment.setAdjustmentR(values[0u].asInt());
		colorAdjustment->_rgbRedAdjustment.setAdjustmentG(values[1u].asInt());
		colorAdjustment->_rgbRedAdjustment.setAdjustmentB(values[2u].asInt());
	}

	if (adjustment.isMember("greenAdjust"))
	{
		const Json::Value & values = adjustment["greenAdjust"];
		colorAdjustment->_rgbGreenAdjustment.setAdjustmentR(values[0u].asInt());
		colorAdjustment->_rgbGreenAdjustment.setAdjustmentG(values[1u].asInt());
		colorAdjustment->_rgbGreenAdjustment.setAdjustmentB(values[2u].asInt());
	}

	if (adjustment.isMember("blueAdjust"))
	{
		const Json::Value & values = adjustment["blueAdjust"];
		colorAdjustment->_rgbBlueAdjustment.setAdjustmentR(values[0u].asInt());
		colorAdjustment->_rgbBlueAdjustment.setAdjustmentG(values[1u].asInt());
		colorAdjustment->_rgbBlueAdjustment.setAdjustmentB(values[2u].asInt());
	}	
	// commit the changes
	_hyperion->adjustmentsUpdated();

	sendSuccessReply(command, tan);
}

void JsonClientConnection::handleSourceSelectCommand(const Json::Value & message, const std::string &command, const int tan)
{
	bool success = false;
	if (message.get("auto",false).asBool())
	{
		_hyperion->setSourceAutoSelectEnabled(true);
		success = true;
	}
	else if (message.isMember("priority"))
	{
		success = _hyperion->setCurrentSourcePriority(message["priority"].asInt());
	}

	if (success)
	{
		sendSuccessReply(command, tan);
	}
	else
	{
		sendErrorReply("setting current priority failed", command, tan);
	}
}

void JsonClientConnection::handleConfigCommand(const Json::Value & message, const std::string &command, const int tan)
{
	std::string subcommand = message.get("subcommand","").asString();
	std::string full_command = command + "-" + subcommand;
	if (subcommand == "getschema")
	{
		handleSchemaGetCommand(message, full_command, tan);
	}
	else if (subcommand == "getconfig")
	{
		handleConfigGetCommand(message, full_command, tan);
	}
	else if (subcommand == "setconfig")
	{
		handleConfigSetCommand(message, full_command, tan);
	} 
	else
	{
		sendErrorReply("unknown or missing subcommand", full_command, tan);
	}
}

void JsonClientConnection::handleConfigGetCommand(const Json::Value & message, const std::string &command, const int tan)
{
	// create result
	Json::Value result;
	result["success"] = true;
	result["command"] = command;
	result["tan"] = tan;
	Json::Value & config = result["result"];
	config = _hyperion->getJsonConfig();
	
	// send the result
	sendMessage(result);
}

void JsonClientConnection::handleSchemaGetCommand(const Json::Value & message, const std::string &command, const int tan)
{
	// create result
	Json::Value result;
	result["success"] = true;
	result["command"] = command;
	result["tan"] = tan;
	Json::Value & schemaJson = result["result"];
	
	// make sure the resources are loaded (they may be left out after static linking)
	Q_INIT_RESOURCE(resource);

	// read the json schema from the resource
	QResource schemaData(":/hyperion-schema");
	assert(schemaData.isValid());

	Json::Reader jsonReader;
	if (!jsonReader.parse(reinterpret_cast<const char *>(schemaData.data()), reinterpret_cast<const char *>(schemaData.data()) + schemaData.size(), schemaJson, false))
	{
		throw std::runtime_error("ERROR: Json schema wrong: " + jsonReader.getFormattedErrorMessages())	;
	}

	// send the result
	sendMessage(result);
}

void JsonClientConnection::handleConfigSetCommand(const Json::Value &message, const std::string &command, const int tan)
{
	struct nested
	{
		static void configSetCommand(const Json::Value& message, Json::Value& config, bool& create)
		{
			if (!config.isObject() || !message.isObject())
				return;

			for (const auto& key : message.getMemberNames()) {
				if ((config.isObject() && config.isMember(key)) || create)
				{
					if (config[key].type() == Json::objectValue && message[key].type() == Json::objectValue)
					{
						configSetCommand(message[key], config[key], create);
					}
					else
						if ( !config[key].empty() || create)
							config[key] = message[key];
				}
			}
		}
	};

	if(message.size() > 0)
	{
		if (message.isObject() && message.isMember("config"))
		{	
			std::string errors;
			if (!checkJson(message["config"], ":/hyperion-schema", errors, true))
			{
				sendErrorReply("Error while validating json: " + errors, command, tan);
				return;
			}
			
			bool createKey = message["create"].asBool();
			Json::Value hyperionConfig;
			message["overwrite"].asBool() ? createKey = true : hyperionConfig = _hyperion->getJsonConfig();
			nested::configSetCommand(message["config"], hyperionConfig, createKey);
			
			JsonFactory::writeJson(_hyperion->getConfigFileName(), hyperionConfig);
			
			sendSuccessReply(command, tan);
		}
	} else
		sendErrorReply("Error while parsing json: Message size " + std::to_string(message.size()), command, tan);
}

void JsonClientConnection::handleComponentStateCommand(const Json::Value& message, const std::string &command, const int tan)
{
	const Json::Value & componentState = message["componentstate"];
	Components component = stringToComponent(QString::fromStdString(componentState.get("component", "invalid").asString()));
	
	if (component != COMP_INVALID)
	{
		_hyperion->setComponentState(component, componentState.get("state", true).asBool());
		sendSuccessReply(command, tan);
	}
	else
	{
		sendErrorReply("invalid component name", command, tan);
	}
}

void JsonClientConnection::handleLedColorsCommand(const Json::Value& message, const std::string &command, const int tan)
{
	// create result
	std::string subcommand = message.get("subcommand","").asString();
	_streaming_leds_reply["success"] = true;
	_streaming_leds_reply["command"] = command;
	_streaming_leds_reply["tan"] = tan;
	
	if (subcommand == "ledstream-start")
	{
		_streaming_leds_reply["command"] = command+"-ledstream-update";
		_timer_ledcolors.start(250);
	}
	else if (subcommand == "ledstream-stop")
	{
		_timer_ledcolors.stop();
	}
	else
	{
		sendErrorReply("unknown subcommand",command,tan);
		return;
	}
	
	sendSuccessReply(command+"-"+subcommand,tan);
}

void JsonClientConnection::handleNotImplemented()
{
	sendErrorReply("Command not implemented");
}

void JsonClientConnection::sendMessage(const Json::Value &message)
{
	Json::FastWriter writer;
	std::string serializedReply = writer.write(message);
	
	if (!_webSocketHandshakeDone)
	{
		// raw tcp socket mode
		_socket->write(serializedReply.data(), serializedReply.length());
	} else
	{
		// websocket mode
		quint32 size = serializedReply.length();
	
		// prepare data frame
		QByteArray response;
		response.append(0x81);
		if (size > 125)
		{
			response.append(0x7E);
			response.append((size >> 8) & 0xFF);
			response.append(size & 0xFF);
		} else {
			response.append(size);
		}
	
		response.append(serializedReply.c_str(), serializedReply.length());
	
		_socket->write(response.data(), response.length());
	}
}


void JsonClientConnection::sendMessage(const Json::Value & message, QTcpSocket * socket)
{
	// serialize message (FastWriter already appends a newline)
	std::string serializedMessage = Json::FastWriter().write(message);

	// write message
	socket->write(serializedMessage.c_str());
	if (!socket->waitForBytesWritten())
	{
		Debug(_log, "Error while writing data to host");
		return;
	}

	// read reply data
	QByteArray serializedReply;
	while (!serializedReply.contains('\n'))
	{
		// receive reply
		if (!socket->waitForReadyRead())
		{
			Debug(_log, "Error while writing data from host");
			return;
		}

		serializedReply += socket->readAll();
	}
	int bytes = serializedReply.indexOf('\n') + 1;     // Find the end of message

	// parse reply data
	Json::Reader jsonReader;
	Json::Value reply;
	if (!jsonReader.parse(serializedReply.constData(), serializedReply.constData() + bytes, reply))
	{
		Error(_log, "Error while parsing reply: invalid json");
		return;
	}

}

void JsonClientConnection::sendSuccessReply(const std::string &command, const int tan)
{
	// create reply
	Json::Value reply;
	reply["success"] = true;
	reply["command"] = command;
	reply["tan"] = tan;

	// send reply
	sendMessage(reply);
}

void JsonClientConnection::sendErrorReply(const std::string &error, const std::string &command, const int tan)
{
	// create reply
	Json::Value reply;
	reply["success"] = false;
	reply["error"] = error;
	reply["command"] = command;
	reply["tan"] = tan;

	// send reply
	sendMessage(reply);
}

bool JsonClientConnection::checkJson(const Json::Value & message, const QString & schemaResource, std::string & errorMessage, bool ignoreRequired)
{
	// read the json schema from the resource
	QResource schemaData(schemaResource);
	assert(schemaData.isValid());
	Json::Reader jsonReader;
	Json::Value schemaJson;
	if (!jsonReader.parse(reinterpret_cast<const char *>(schemaData.data()), reinterpret_cast<const char *>(schemaData.data()) + schemaData.size(), schemaJson, false))
	{
		errorMessage = "Schema error: " + jsonReader.getFormattedErrorMessages();
		return false;
	}

	// create schema checker
	JsonSchemaChecker schema;
	schema.setSchema(schemaJson);

	// check the message
	if (!schema.validate(message, ignoreRequired))
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


void JsonClientConnection::streamLedcolorsUpdate()
{
	Json::Value & leds = _streaming_leds_reply["result"] = Json::Value(Json::arrayValue);

	const PriorityMuxer::InputInfo & priorityInfo = _hyperion->getPriorityInfo(_hyperion->getCurrentPriority());
	std::vector<ColorRgb> ledBuffer =  priorityInfo.ledColors;

	for (ColorRgb& color : ledBuffer)
	{
		int idx = leds.size();
		Json::Value & item = leds[idx];
		item["index"] = idx;
		item["red"]   = color.red;
		item["green"] = color.green;
		item["blue"]  = color.blue;
	}

	// send the result
	sendMessage(_streaming_leds_reply);

}

