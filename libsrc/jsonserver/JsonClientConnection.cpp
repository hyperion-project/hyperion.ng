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

// hyperion util includes
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>
#include <hyperion/MessageForwarder.h>
#include <hyperion/ColorTransform.h>
#include <hyperion/ColorCorrection.h>
#include <hyperion/ColorAdjustment.h>
#include <utils/ColorRgb.h>
#include <HyperionConfig.h>

// project includes
#include "JsonClientConnection.h"

JsonClientConnection::JsonClientConnection(QTcpSocket *socket)
	: QObject()
	, _socket(socket)
	, _imageProcessor(ImageProcessorFactory::getInstance().newImageProcessor())
	, _hyperion(Hyperion::getInstance())
	, _receiveBuffer()
	, _webSocketHandshakeDone(false)
	, _log(Logger::getInstance("JSONCLIENTCONNECTION"))
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
	else if (command == "temperature")
		handleTemperatureCommand(message);
	else if (command == "adjustment")
		handleAdjustmentCommand(message);
	else if (command == "sourceselect")
		handleSourceSelectCommand(message);
	else
		handleNotImplemented();
}


void JsonClientConnection::forwardJsonMessage(const Json::Value & message)
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

void JsonClientConnection::handleColorCommand(const Json::Value &message)
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
	sendSuccessReply();
}

void JsonClientConnection::handleImageCommand(const Json::Value &message)
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
	sendSuccessReply();
}

void JsonClientConnection::handleServerInfoCommand(const Json::Value &)
{
	// create result
	Json::Value result;
	result["success"] = true;
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
		item["active"] = "false";
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
		Json::Value activeEffect;
		activeEffect["script"] = activeEffectDefinition.script;
		activeEffect["priority"] = activeEffectDefinition.priority;
		activeEffect["timeout"] = activeEffectDefinition.timeout;
		activeEffect["args"] = activeEffectDefinition.args;

		activeEffects.append(activeEffect);
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

	// Add Hyperion Version, build time
	Json::Value & version = info["hyperion"] = Json::Value(Json::arrayValue);
	Json::Value ver;
	ver["jsonrpc_version"] = HYPERION_JSON_VERSION;
	ver["version"] = HYPERION_VERSION;
	ver["build"] = HYPERION_BUILD_ID;
	ver["time"] = __DATE__ " " __TIME__;

	version.append(ver);

	// send the result
	sendMessage(result);
}

void JsonClientConnection::handleClearCommand(const Json::Value &message)
{
	forwardJsonMessage(message);

	// extract parameters
	int priority = message["priority"].asInt();

	// clear priority
	_hyperion->clear(priority);

	// send reply
	sendSuccessReply();
}

void JsonClientConnection::handleClearallCommand(const Json::Value & message)
{
	forwardJsonMessage(message);

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

	sendSuccessReply();
}


void JsonClientConnection::handleTemperatureCommand(const Json::Value &message)
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

	sendSuccessReply();
}

void JsonClientConnection::handleAdjustmentCommand(const Json::Value &message)
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

	sendSuccessReply();
}

void JsonClientConnection::handleSourceSelectCommand(const Json::Value & message)
{
	bool success = false;
	if (message.get("auto",false).asBool())
	{
		_hyperion->setSourceAutoSelectEnabled(true);
		success = true;
	}
	else if (message.isMember("priority"))
	{
		success =  _hyperion->setCurrentSourcePriority(message["priority"].asInt());
	}

	if (success)
	{
		sendSuccessReply();
	}
	else
	{
		sendErrorReply("setting current priority failed");
	}
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
		throw std::runtime_error("JSONCLIENT ERROR: Schema error: " + jsonReader.getFormattedErrorMessages());
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
