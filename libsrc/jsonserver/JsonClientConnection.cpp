// system includes
#include <stdexcept>
#include <cassert>
#include <iomanip>
#include <unistd.h>

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
#include <QFileInfo>
#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonDocument>
#include <QVariantMap>
#include <QDir>

// hyperion util includes
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>
#include <hyperion/MessageForwarder.h>
#include <hyperion/ColorTransform.h>
#include <hyperion/ColorAdjustment.h>
#include <utils/ColorRgb.h>
#include <leddevice/LedDevice.h>
#include <HyperionConfig.h>
#include <utils/jsonschema/QJsonFactory.h>
#include <utils/Process.h>

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
	, _streaming_logging_activated(false)
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
				handleMessage(QString::fromStdString(message));

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
								
				handleMessage(QString(result));
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

void JsonClientConnection::handleMessage(const QString& messageString)
{
	QString errors;
	
 	try
 	{
		QJsonParseError error;
		QJsonDocument doc = QJsonDocument::fromJson(messageString.toUtf8(), &error);

		if (error.error != QJsonParseError::NoError)
		{
			// report to the user the failure and their locations in the document.
			int errorLine(0), errorColumn(0);
			
			for( int i=0, count=qMin( error.offset,messageString.size()); i<count; ++i )
			{
				++errorColumn;
				if(messageString.at(i) == '\n' )
				{
					errorColumn = 0;
					++errorLine;
				}
			}
			
			std::stringstream sstream;
			sstream << "Error while parsing json: " << error.errorString().toStdString() << " at Line: " << errorLine << ", Column: " << errorColumn;
			sendErrorReply(QString::fromStdString(sstream.str()));
			return;
		}
		
		const QJsonObject message = doc.object();

		// check basic message
		if (!checkJson(message, ":schema", errors))
		{
			sendErrorReply("Error while validating json: " + errors);
			return;
		}

		// check specific message
		const QString command = message["command"].toString();
		if (!checkJson(message, QString(":schema-%1").arg(command), errors))
		{
			sendErrorReply("Error while validating json: " + errors);
			return;
		}

		int tan = message["tan"].toInt(0);
		// switch over all possible commands and handle them
		if (command == "color")
			handleColorCommand(message, command, tan);
		else if (command == "image")
			handleImageCommand(message, command, tan);
		else if (command == "effect")
			handleEffectCommand(message, command, tan);
		else if (command == "create-effect")
			handleCreateEffectCommand(message, command, tan);
		else if (command == "delete-effect")
			handleDeleteEffectCommand(message, command, tan);
		else if (command == "serverinfo")
			handleServerInfoCommand(message, command, tan);
		else if (command == "clear")
			handleClearCommand(message, command, tan);
		else if (command == "clearall")
			handleClearallCommand(message, command, tan);
		else if (command == "transform")
			handleTransformCommand(message, command, tan);
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
		else if (command == "logging")
			handleLoggingCommand(message, command, tan);
		else
			handleNotImplemented();
 	}
 	catch (std::exception& e)
 	{
 		sendErrorReply("Error while processing incoming json message: " + QString(e.what()) + " " + errors );
 		Warning(_log, "Error while processing incoming json message: %s (%s)", e.what(), errors.toStdString().c_str());
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

void JsonClientConnection::forwardJsonMessage(const QJsonObject & message)
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

void JsonClientConnection::handleColorCommand(const QJsonObject& message, const QString& command, const int tan)
{
	forwardJsonMessage(message);

	// extract parameters
	int priority = message["priority"].toInt();
	int duration = message["duration"].toInt(-1);

	std::vector<ColorRgb> colorData(_hyperion->getLedCount());
	const QJsonArray & jsonColor = message["color"].toArray();
	unsigned int i = 0;
	for (; i < unsigned(jsonColor.size()/3) && i < _hyperion->getLedCount(); ++i)
	{
		colorData[i].red = uint8_t(jsonColor.at(3u*i).toInt());
		colorData[i].green = uint8_t(jsonColor.at(3u*i+1u).toInt());
		colorData[i].blue = uint8_t(jsonColor.at(3u*i+2u).toInt());
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

void JsonClientConnection::handleImageCommand(const QJsonObject& message, const QString& command, const int tan)
{
	forwardJsonMessage(message);

	// extract parameters
	int priority = message["priority"].toInt();
	int duration = message["duration"].toInt(-1);
	int width = message["imagewidth"].toInt();
	int height = message["imageheight"].toInt();
	QByteArray data = QByteArray::fromBase64(QByteArray(message["imagedata"].toString().toUtf8()));

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

void JsonClientConnection::handleEffectCommand(const QJsonObject& message, const QString& command, const int tan)
{
	forwardJsonMessage(message);

	// extract parameters
	int priority = message["priority"].toInt();
	int duration = message["duration"].toInt(-1);
	QString pythonScript = message["pythonScript"].toString("");
	const QJsonObject & effect = message["effect"].toObject();
	const QString & effectName = effect["name"].toString();

	// set output
	if (effect.contains("args"))
	{
		_hyperion->setEffect(effectName, effect["args"].toObject(), priority, duration, pythonScript);
	}
	else
	{
		_hyperion->setEffect(effectName, priority, duration);
	}

	// send reply
	sendSuccessReply(command, tan);
}

void JsonClientConnection::handleCreateEffectCommand(const QJsonObject& message, const QString &command, const int tan)
{
	struct find_schema: std::unary_function<EffectSchema, bool>
	{
		QString pyFile;
		find_schema(QString pyFile):pyFile(pyFile) { }
		bool operator()(EffectSchema const& schema) const
		{
			return schema.pyFile == pyFile;
		}
	};

	if(message.size() > 0)
	{
		if (!message["args"].toObject().isEmpty())
		{
			QString scriptName;
			(message["script"].toString().mid(0, 1)  == ":" )
				? scriptName = ":/effects//" + message["script"].toString().mid(1)
				: scriptName = message["script"].toString();
			
			std::list<EffectSchema> effectsSchemas = _hyperion->getEffectSchemas();
			std::list<EffectSchema>::iterator it = std::find_if(effectsSchemas.begin(), effectsSchemas.end(), find_schema(scriptName));
			
			if (it != effectsSchemas.end())
			{
				QString errors;
				
				if (!checkJson(message["args"].toObject(), it->schemaFile, errors))
				{
					sendErrorReply("Error while validating json: " + errors, command, tan);
					return;
				}
				
				QJsonObject effectJson;
				QJsonArray effectArray;
				effectArray = _hyperion->getQJsonConfig()["effects"].toObject()["paths"].toArray();
				
				if (effectArray.size() > 0)
				{
					effectJson["name"] = message["name"].toString();
					effectJson["script"] = message["script"].toString();
					effectJson["args"] = message["args"].toObject();
					
					QFileInfo newFileName(effectArray[0].toString() + QDir::separator() + message["name"].toString().replace(QString(" "), QString("")) + QString(".json"));
					
					while(newFileName.exists())
					{
						newFileName.setFile(effectArray[0].toString() + QDir::separator() + newFileName.baseName() + QString::number(qrand() % ((10) - 0) + 0) + QString(".json"));
					}
					
					QJsonFactory::writeJson(newFileName.absoluteFilePath(), effectJson);
					Info(_log, "Reload effect list");
					_hyperion->reloadEffects();
					sendSuccessReply(command, tan);
				} else
				{
					sendErrorReply("Can't save new effect. Effect path empty", command, tan);
					return;
				}
			} else
				sendErrorReply("Missing schema file for Python script " + message["script"].toString(), command, tan);
		} else
			sendErrorReply("Missing or empty Object 'args'", command, tan);
	} else
		sendErrorReply("Error while parsing json: Message size " + QString(message.size()), command, tan);
}

void JsonClientConnection::handleDeleteEffectCommand(const QJsonObject& message, const QString& command, const int tan)
{
	struct find_effect: std::unary_function<EffectDefinition, bool>
	{
		QString effectName;
		find_effect(QString effectName) :effectName(effectName) { }
		bool operator()(EffectDefinition const& effectDefinition) const
		{
			return effectDefinition.name == effectName;
		}
	};
	
	if(message.size() > 0)
	{
		QString effectName = message["name"].toString();
		std::list<EffectDefinition> effectsDefinition = _hyperion->getEffects();
		std::list<EffectDefinition>::iterator it = std::find_if(effectsDefinition.begin(), effectsDefinition.end(), find_effect(effectName));
		
		if (it != effectsDefinition.end())
		{
			QFileInfo effectConfigurationFile(it->file);
			if (effectConfigurationFile.absoluteFilePath().mid(0, 1)  != ":" )
			{
				if (effectConfigurationFile.exists())
				{
					bool result = QFile::remove(effectConfigurationFile.absoluteFilePath());
					if (result)
					{
						Info(_log, "Reload effect list");
						_hyperion->reloadEffects();
						sendSuccessReply(command, tan);
					} else
						sendErrorReply("Can't delete effect configuration file: " + effectConfigurationFile.absoluteFilePath() + ". Please check permissions", command, tan);
				} else
					sendErrorReply("Can't find effect configuration file: " + effectConfigurationFile.absoluteFilePath(), command, tan);
			} else
				sendErrorReply("Can't delete internal effect: " + message["name"].toString(), command, tan);
		} else
			sendErrorReply("Effect " + message["name"].toString() + " not found", command, tan);
	} else
		sendErrorReply("Error while parsing json: Message size " + QString(message.size()), command, tan);
}

void JsonClientConnection::handleServerInfoCommand(const QJsonObject&, const QString& command, const int tan)
{
	// create result
	QJsonObject result;
	result["success"] = true;
	result["command"] = command;
	result["tan"] = tan;
	
	QJsonObject info;

	// add host name for remote clients
	info["hostname"] = QHostInfo::localHostName();

	// collect priority information
	QJsonArray priorities;
	uint64_t now = QDateTime::currentMSecsSinceEpoch();
	QList<int> activePriorities = _hyperion->getActivePriorities();
	Hyperion::PriorityRegister priorityRegister = _hyperion->getPriorityRegister();
	int currentPriority = _hyperion->getCurrentPriority();
	
	foreach (int priority, activePriorities) {
		const Hyperion::InputInfo & priorityInfo = _hyperion->getPriorityInfo(priority);
		QJsonObject item;
		item["priority"] = priority;
		if (priorityInfo.timeoutTime_ms != -1)
		{
			item["duration_ms"] = int(priorityInfo.timeoutTime_ms - now);
		}
		
		item["owner"] = QString("unknown");
		item["active"] = true;
		item["visible"] = (priority == currentPriority);
		foreach(auto const &entry, priorityRegister)
		{
			if (entry.second == priority)
			{
				item["owner"] = QString::fromStdString(entry.first);
				priorityRegister.erase(entry.first);
				break;
			}
		}
		
		// priorities[priorities.size()] = item;
		priorities.append(item);
	}
	
	foreach(auto const &entry, priorityRegister)
	{
		QJsonObject item;
		item["priority"] = entry.second;
		item["active"] = false;
		item["visible"] = false;
		item["owner"] = QString::fromStdString(entry.first);
		priorities.append(item);
	}

	info["priorities"] = priorities;
	
	// collect transform information
	QJsonArray transformArray;
	for (const std::string& transformId : _hyperion->getTransformIds())
	{
		const ColorTransform * colorTransform = _hyperion->getTransform(transformId);
		if (colorTransform == nullptr)
		{
			Error(_log, "Incorrect color transform id: %s", transformId.c_str());
			continue;
		}

		QJsonObject transform;
		transform["id"] = QString::fromStdString(transformId);

		transform["saturationGain"] = colorTransform->_hsvTransform.getSaturationGain();
		transform["valueGain"]      = colorTransform->_hsvTransform.getValueGain();
		transform["saturationLGain"] = colorTransform->_hslTransform.getSaturationGain();
		transform["luminanceGain"]   = colorTransform->_hslTransform.getLuminanceGain();
		transform["luminanceMinimum"]   = colorTransform->_hslTransform.getLuminanceMinimum();
		

		QJsonArray threshold;
		threshold.append(colorTransform->_rgbRedTransform.getThreshold());
		threshold.append(colorTransform->_rgbGreenTransform.getThreshold());
		threshold.append(colorTransform->_rgbBlueTransform.getThreshold());
		transform.insert("threshold", threshold);

		QJsonArray gamma;
		gamma.append(colorTransform->_rgbRedTransform.getGamma());
		gamma.append(colorTransform->_rgbGreenTransform.getGamma());
		gamma.append(colorTransform->_rgbBlueTransform.getGamma());
		transform.insert("gamma", gamma);

		QJsonArray blacklevel;
		blacklevel.append(colorTransform->_rgbRedTransform.getBlacklevel());
		blacklevel.append(colorTransform->_rgbGreenTransform.getBlacklevel());
		blacklevel.append(colorTransform->_rgbBlueTransform.getBlacklevel());
		transform.insert("blacklevel", blacklevel);

		QJsonArray whitelevel;
		whitelevel.append(colorTransform->_rgbRedTransform.getWhitelevel());
		whitelevel.append(colorTransform->_rgbGreenTransform.getWhitelevel());
		whitelevel.append(colorTransform->_rgbBlueTransform.getWhitelevel());
		transform.insert("whitelevel", whitelevel);
		
		transformArray.append(transform);
	}
	
	info["transform"] = transformArray;
	
	// collect adjustment information
	QJsonArray adjustmentArray;
	for (const std::string& adjustmentId : _hyperion->getAdjustmentIds())
	{
		const ColorAdjustment * colorAdjustment = _hyperion->getAdjustment(adjustmentId);
		if (colorAdjustment == nullptr)
		{
			Error(_log, "Incorrect color adjustment id: %s", adjustmentId.c_str());
			continue;
		}

		QJsonObject adjustment;
		adjustment["id"] = QString::fromStdString(adjustmentId);

		QJsonArray redAdjust;
		redAdjust.append(colorAdjustment->_rgbRedAdjustment.getAdjustmentR());
		redAdjust.append(colorAdjustment->_rgbRedAdjustment.getAdjustmentG());
		redAdjust.append(colorAdjustment->_rgbRedAdjustment.getAdjustmentB());
		adjustment.insert("redAdjust", redAdjust);

		QJsonArray greenAdjust;
		greenAdjust.append(colorAdjustment->_rgbGreenAdjustment.getAdjustmentR());
		greenAdjust.append(colorAdjustment->_rgbGreenAdjustment.getAdjustmentG());
		greenAdjust.append(colorAdjustment->_rgbGreenAdjustment.getAdjustmentB());
		adjustment.insert("greenAdjust", greenAdjust);

		QJsonArray blueAdjust;
		blueAdjust.append(colorAdjustment->_rgbBlueAdjustment.getAdjustmentR());
		blueAdjust.append(colorAdjustment->_rgbBlueAdjustment.getAdjustmentG());
		blueAdjust.append(colorAdjustment->_rgbBlueAdjustment.getAdjustmentB());
		adjustment.insert("blueAdjust", blueAdjust);
		
		adjustmentArray.append(adjustment);
	}
	
	info["adjustment"] = adjustmentArray;

	// collect effect info
	QJsonArray effects;
	const std::list<EffectDefinition> & effectsDefinitions = _hyperion->getEffects();
	for (const EffectDefinition & effectDefinition : effectsDefinitions)
	{
		QJsonObject effect;
		effect["name"] = effectDefinition.name;
		effect["file"] = effectDefinition.file;
		effect["script"] = effectDefinition.script;
		effect["args"] = effectDefinition.args;
		effects.append(effect);
	}
	
	info["effects"] = effects;
	
	// collect active effect info
	QJsonArray activeEffects;
	const std::list<ActiveEffectDefinition> & activeEffectsDefinitions = _hyperion->getActiveEffects();
	for (const ActiveEffectDefinition & activeEffectDefinition : activeEffectsDefinitions)
	{
		if (activeEffectDefinition.priority != PriorityMuxer::LOWEST_PRIORITY -1)
		{
			QJsonObject activeEffect;
			activeEffect["script"] = activeEffectDefinition.script;
			activeEffect["name"] = activeEffectDefinition.name;
			activeEffect["priority"] = activeEffectDefinition.priority;
			activeEffect["timeout"] = activeEffectDefinition.timeout;
			activeEffect["args"] = activeEffectDefinition.args;
			activeEffects.append(activeEffect);
		}
	}
	
	info["activeEffects"] = activeEffects;

	// collect active static led color
	QJsonArray activeLedColors;
	const Hyperion::InputInfo & priorityInfo = _hyperion->getPriorityInfo(_hyperion->getCurrentPriority());
	if (priorityInfo.priority != std::numeric_limits<int>::max())
	{
		QJsonObject LEDcolor;
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
			QJsonArray RGBValue;
			RGBValue.append(priorityInfo.ledColors.begin()->red);
			RGBValue.append(priorityInfo.ledColors.begin()->green);
			RGBValue.append(priorityInfo.ledColors.begin()->blue);
			LEDcolor.insert("RGB Value", RGBValue);

			uint16_t Hue;
			float Saturation, Luminace;
		    
			// add HSL Value to Array
			QJsonArray HSLValue;
			HslTransform::rgb2hsl(priorityInfo.ledColors.begin()->red,
					priorityInfo.ledColors.begin()->green,
					priorityInfo.ledColors.begin()->blue,
					Hue, Saturation, Luminace);

			HSLValue.append(Hue);
			HSLValue.append(Saturation);
			HSLValue.append(Luminace);
			LEDcolor.insert("HSL Value", HSLValue);

			// add HEX Value to Array ["HEX Value"]
			QJsonArray HEXValue;
			std::stringstream hex;
				hex << "0x"
				<< std::uppercase << std::setw(2) << std::setfill('0')
				<< std::hex << unsigned(priorityInfo.ledColors.begin()->red)
				<< std::uppercase << std::setw(2) << std::setfill('0')
				<< std::hex << unsigned(priorityInfo.ledColors.begin()->green)
				<< std::uppercase << std::setw(2) << std::setfill('0')
				<< std::hex << unsigned(priorityInfo.ledColors.begin()->blue);

			HEXValue.append(QString::fromStdString(hex.str()));
			LEDcolor.insert("HEX Value", HEXValue);

			activeLedColors.append(LEDcolor);
			}
		}
	}
	
	info["activeLedColor"] = activeLedColors;

	// get available led devices
	QJsonObject ledDevices;
	ledDevices["active"] = QString::fromStdString(LedDevice::activeDevice());
	QJsonArray available;
	for (auto dev: LedDevice::getDeviceMap())
	{
		available.append(QString::fromStdString(dev.first));
	}
	
	ledDevices["available"] = available;
	info["ledDevices"] = ledDevices;

	// get available components
	QJsonArray component;
	std::map<hyperion::Components, bool> components = _hyperion->getComponentRegister().getRegister();
	for(auto comp : components)
	{
		QJsonObject item;
		item["id"] = comp.first;
		item["name"] = QString::fromStdString(hyperion::componentToIdString(comp.first));
		item["title"] = QString::fromStdString(hyperion::componentToString(comp.first));
		item["enabled"] = comp.second;
		
		component.append(item);
	}
	
	info["components"] = component;
	
	// Add Hyperion Version, build time
	QJsonArray hyperion;
	QJsonObject ver;
	ver["jsonrpc_version"] = QString(HYPERION_JSON_VERSION);
	ver["version"] = QString(HYPERION_VERSION);
	ver["build"]   = QString(HYPERION_BUILD_ID);
	ver["time"]    = QString(__DATE__ " " __TIME__);
	ver["config_modified"] = _hyperion->configModified();

	hyperion.append(ver);
	info["hyperion"] = hyperion;
	
	// send the result
	result["info"] = info;
	sendMessage(result);
}

void JsonClientConnection::handleClearCommand(const QJsonObject& message, const QString& command, const int tan)
{
	forwardJsonMessage(message);

	// extract parameters
	int priority = message["priority"].toInt();

	// clear priority
	_hyperion->clear(priority);

	// send reply
	sendSuccessReply(command, tan);
}

void JsonClientConnection::handleClearallCommand(const QJsonObject& message, const QString& command, const int tan)
{
	forwardJsonMessage(message);

	// clear priority
	_hyperion->clearall();

	// send reply
	sendSuccessReply(command, tan);
}

void JsonClientConnection::handleTransformCommand(const QJsonObject& message, const QString& command, const int tan)
{
	
	const QJsonObject & transform = message["transform"].toObject();

	const QString transformId = transform["id"].toString(QString::fromStdString(_hyperion->getTransformIds().front()));
	ColorTransform * colorTransform = _hyperion->getTransform(transformId.toStdString());
	if (colorTransform == nullptr)
	{
		Warning(_log, "Incorrect transform identifier: %s", transformId.toStdString().c_str());
		return;
	}
		
	if (transform.contains("saturationGain"))
	{
		colorTransform->_hsvTransform.setSaturationGain(transform["saturationGain"].toDouble());
	}

	if (transform.contains("valueGain"))
	{
		colorTransform->_hsvTransform.setValueGain(transform["valueGain"].toDouble());
	}
	
	if (transform.contains("saturationLGain"))
	{
		colorTransform->_hslTransform.setSaturationGain(transform["saturationLGain"].toDouble());
	}

	if (transform.contains("luminanceGain"))
	{
		colorTransform->_hslTransform.setLuminanceGain(transform["luminanceGain"].toDouble());
	}

	if (transform.contains("luminanceMinimum"))
	{
		colorTransform->_hslTransform.setLuminanceMinimum(transform["luminanceMinimum"].toDouble());
	}

	if (transform.contains("threshold"))
	{
		const QJsonArray & values = transform["threshold"].toArray();
		colorTransform->_rgbRedTransform  .setThreshold(values[0u].toDouble());
		colorTransform->_rgbGreenTransform.setThreshold(values[1u].toDouble());
		colorTransform->_rgbBlueTransform .setThreshold(values[2u].toDouble());
	}

	if (transform.contains("gamma"))
	{
		const QJsonArray & values = transform["gamma"].toArray();
		colorTransform->_rgbRedTransform  .setGamma(values[0u].toDouble());
		colorTransform->_rgbGreenTransform.setGamma(values[1u].toDouble());
		colorTransform->_rgbBlueTransform .setGamma(values[2u].toDouble());
	}

	if (transform.contains("blacklevel"))
	{
		const QJsonArray & values = transform["blacklevel"].toArray();
		colorTransform->_rgbRedTransform  .setBlacklevel(values[0u].toDouble());
		colorTransform->_rgbGreenTransform.setBlacklevel(values[1u].toDouble());
		colorTransform->_rgbBlueTransform .setBlacklevel(values[2u].toDouble());
	}

	if (transform.contains("whitelevel"))
	{
		const QJsonArray & values = transform["whitelevel"].toArray();
		colorTransform->_rgbRedTransform  .setWhitelevel(values[0u].toDouble());
		colorTransform->_rgbGreenTransform.setWhitelevel(values[1u].toDouble());
		colorTransform->_rgbBlueTransform .setWhitelevel(values[2u].toDouble());
	}
	
	// commit the changes
	_hyperion->transformsUpdated();

	sendSuccessReply(command, tan);
}


void JsonClientConnection::handleAdjustmentCommand(const QJsonObject& message, const QString& command, const int tan)
{
	const QJsonObject & adjustment = message["adjustment"].toObject();

	const QString adjustmentId = adjustment["id"].toString(QString::fromStdString(_hyperion->getAdjustmentIds().front()));
	ColorAdjustment * colorAdjustment = _hyperion->getAdjustment(adjustmentId.toStdString());
	if (colorAdjustment == nullptr)
	{
		Warning(_log, "Incorrect adjustment identifier: %s", adjustmentId.toStdString().c_str());
		return;
	}
		
	if (adjustment.contains("redAdjust"))
	{
		const QJsonArray & values = adjustment["redAdjust"].toArray();
		colorAdjustment->_rgbRedAdjustment.setAdjustmentR(values[0u].toInt());
		colorAdjustment->_rgbRedAdjustment.setAdjustmentG(values[1u].toInt());
		colorAdjustment->_rgbRedAdjustment.setAdjustmentB(values[2u].toInt());
	}

	if (adjustment.contains("greenAdjust"))
	{
		const QJsonArray & values = adjustment["greenAdjust"].toArray();
		colorAdjustment->_rgbGreenAdjustment.setAdjustmentR(values[0u].toInt());
		colorAdjustment->_rgbGreenAdjustment.setAdjustmentG(values[1u].toInt());
		colorAdjustment->_rgbGreenAdjustment.setAdjustmentB(values[2u].toInt());
	}

	if (adjustment.contains("blueAdjust"))
	{
		const QJsonArray & values = adjustment["blueAdjust"].toArray();
		colorAdjustment->_rgbBlueAdjustment.setAdjustmentR(values[0u].toInt());
		colorAdjustment->_rgbBlueAdjustment.setAdjustmentG(values[1u].toInt());
		colorAdjustment->_rgbBlueAdjustment.setAdjustmentB(values[2u].toInt());
	}	
	// commit the changes
	_hyperion->adjustmentsUpdated();

	sendSuccessReply(command, tan);
}

void JsonClientConnection::handleSourceSelectCommand(const QJsonObject& message, const QString& command, const int tan)
{
	bool success = false;
	if (message["auto"].toBool(false))
	{
		_hyperion->setSourceAutoSelectEnabled(true);
		success = true;
	}
	else if (message.contains("priority"))
	{
		success = _hyperion->setCurrentSourcePriority(message["priority"].toInt());
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

void JsonClientConnection::handleConfigCommand(const QJsonObject& message, const QString& command, const int tan)
{
	QString subcommand = message["subcommand"].toString("");
	QString full_command = command + "-" + subcommand;
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
	else if (subcommand == "reload")
	{
		_hyperion->freeObjects();
		Process::restartHyperion();
		sendErrorReply("failed to restart hyperion", full_command, tan);
	} 
	else
	{
		sendErrorReply("unknown or missing subcommand", full_command, tan);
	}
}

void JsonClientConnection::handleConfigGetCommand(const QJsonObject& message, const QString& command, const int tan)
{
	// create result
	QJsonObject result;
	result["success"] = true;
	result["command"] = command;
	result["tan"] = tan;
	const QJsonObject & config = _hyperion->getQJsonConfig();
	result["result"] = config;

	// send the result
	sendMessage(result);
}

void JsonClientConnection::handleSchemaGetCommand(const QJsonObject& message, const QString& command, const int tan)
{
	// create result
	QJsonObject result, schemaJson, alldevices, properties;
	result["success"] = true;
	result["command"] = command;
	result["tan"] = tan;
	
	// make sure the resources are loaded (they may be left out after static linking)
	Q_INIT_RESOURCE(resource);
	QJsonParseError error;

	// read the hyperion json schema from the resource
	QFile schemaData(":/hyperion-schema");
	
	if (!schemaData.open(QIODevice::ReadOnly))
	{
		std::stringstream error;
		error << "Schema not found: " << schemaData.errorString().toStdString();
		throw std::runtime_error(error.str());
	}
	
	QByteArray schema = schemaData.readAll();
	QJsonDocument doc = QJsonDocument::fromJson(schema, &error);
	schemaData.close();
	
	if (error.error != QJsonParseError::NoError)
	{
		// report to the user the failure and their locations in the document.
		int errorLine(0), errorColumn(0);
		
		for( int i=0, count=qMin( error.offset,schema.size()); i<count; ++i )
		{
			++errorColumn;
			if(schema.at(i) == '\n' )
			{
				errorColumn = 0;
				++errorLine;
			}
		}
		
		std::stringstream sstream;
		sstream << "ERROR: Json schema wrong: " << error.errorString().toStdString() << " at Line: " << errorLine << ", Column: " << errorColumn;

		throw std::runtime_error(sstream.str());
	}
	
	schemaJson = doc.object();
	
	// collect all LED Devices
	properties = schemaJson["properties"].toObject();
	alldevices = LedDevice::getLedDeviceSchemas();
	properties.insert("alldevices", alldevices);
	
	// collect all available effect schemas
	QJsonObject pyEffectSchemas, pyEffectSchema;
	QJsonArray in, ex;
	const std::list<EffectSchema> & effectsSchemas = _hyperion->getEffectSchemas();
	for (const EffectSchema & effectSchema : effectsSchemas)
	{
		if (effectSchema.pyFile.mid(0, 1)  == ":")
		{
			QJsonObject internal;
			internal.insert("script", effectSchema.pyFile);
			internal.insert("schemaLocation", effectSchema.schemaFile);
			internal.insert("schemaContent", effectSchema.pySchema);
			in.append(internal);
		}
		else
		{
			QJsonObject external;
			external.insert("script", effectSchema.pyFile);
			external.insert("schemaLocation", effectSchema.schemaFile);
			external.insert("schemaContent", effectSchema.pySchema);
			ex.append(external);
		}
	}
	
	if (!in.empty())
		pyEffectSchema.insert("internal", in);
	if (!ex.empty())
		pyEffectSchema.insert("external", ex);
	
	pyEffectSchemas = pyEffectSchema;
	properties.insert("effectSchemas", pyEffectSchemas);

	schemaJson.insert("properties", properties);
	
	result["result"] = schemaJson;

	// send the result
	sendMessage(result);
}

void JsonClientConnection::handleConfigSetCommand(const QJsonObject& message, const QString &command, const int tan)
{
	if(message.size() > 0)
	{
		if (message.contains("config"))
		{	
			QString errors;
			if (!checkJson(message["config"].toObject(), ":/hyperion-schema", errors, true))
			{
				sendErrorReply("Error while validating json: " + errors, command, tan);
				return;
			}
			
			QJsonObject hyperionConfig = message["config"].toObject();
			QJsonFactory::writeJson(QString::fromStdString(_hyperion->getConfigFileName()), hyperionConfig);
			
			sendSuccessReply(command, tan);
		}
	} else
		sendErrorReply("Error while parsing json: Message size " + QString(message.size()), command, tan);
}

void JsonClientConnection::handleComponentStateCommand(const QJsonObject& message, const QString &command, const int tan)
{
	const QJsonObject & componentState = message["componentstate"].toObject();
	Components component = stringToComponent(componentState["component"].toString("invalid"));
	
	if (component != COMP_INVALID)
	{
		_hyperion->setComponentState(component, componentState["state"].toBool(true));
		sendSuccessReply(command, tan);
	}
	else
	{
		sendErrorReply("invalid component name", command, tan);
	}
}

void JsonClientConnection::handleLedColorsCommand(const QJsonObject& message, const QString &command, const int tan)
{
	// create result
	QString subcommand = message["subcommand"].toString("");
	_streaming_leds_reply["success"] = true;
	_streaming_leds_reply["command"] = command;
	_streaming_leds_reply["tan"] = tan;
	
	if (subcommand == "ledstream-start")
	{
		_streaming_leds_reply["command"] = command+"-ledstream-update";
		_timer_ledcolors.start(125);
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

void JsonClientConnection::handleLoggingCommand(const QJsonObject& message, const QString &command, const int tan)
{
	// create result
	QString subcommand = message["subcommand"].toString("");
	_streaming_logging_reply["success"] = true;
	_streaming_logging_reply["command"] = command;
	_streaming_logging_reply["tan"] = tan;
	
	if (subcommand == "start")
	{
		if (!_streaming_logging_activated)
		{
			_streaming_logging_reply["command"] = command+"-update";
			connect(LoggerManager::getInstance(),SIGNAL(newLogMessage(Logger::T_LOG_MESSAGE)), this, SLOT(incommingLogMessage(Logger::T_LOG_MESSAGE)));
			Debug(_log, "log streaming activated"); // needed to trigger log sending
		}
	}
	else if (subcommand == "stop")
	{
		if (_streaming_logging_activated)
		{
			disconnect(LoggerManager::getInstance(), SIGNAL(newLogMessage(Logger::T_LOG_MESSAGE)), this, 0);
			_streaming_logging_activated = false;
			Debug(_log, "log streaming deactivated");

		}
	}
	else
	{
		sendErrorReply("unknown subcommand",command,tan);
		return;
	}
	
	sendSuccessReply(command+"-"+subcommand,tan);
}

void JsonClientConnection::incommingLogMessage(Logger::T_LOG_MESSAGE msg)
{
	QJsonObject result;
	QJsonArray messages;

	if (!_streaming_logging_activated)
	{
		_streaming_logging_activated = true;
		QVector<Logger::T_LOG_MESSAGE>* logBuffer = LoggerManager::getInstance()->getLogMessageBuffer();
		for(int i=0; i<logBuffer->length(); i++)
		{
			//std::cout << "------- " << logBuffer->at(i).message.toStdString() << std::endl;
			messages.append(logBuffer->at(i).message);
		}
	}
	else
	{
		//std::cout << "------- " << msg.message.toStdString() << std::endl;
		messages.append(msg.message);
	}

	result["messages"] = messages;
	_streaming_logging_reply["result"] = result;

	// send the result
	sendMessage(_streaming_logging_reply);
}


void JsonClientConnection::handleNotImplemented()
{
	sendErrorReply("Command not implemented");
}

void JsonClientConnection::sendMessage(const QJsonObject &message)
{
	QJsonDocument writer(message);
	QByteArray serializedReply = writer.toJson(QJsonDocument::Compact) + "\n";
	
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
	
		response.append(serializedReply, serializedReply.length());
	
		_socket->write(response.data(), response.length());
	}
}


void JsonClientConnection::sendMessage(const QJsonObject & message, QTcpSocket * socket)
{
	// serialize message
	QJsonDocument writer(message);
	QByteArray serializedMessage = writer.toJson(QJsonDocument::Compact) + "\n";

	// write message
	socket->write(serializedMessage);
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

	// parse reply data
	QJsonParseError error;
	QJsonDocument reply = QJsonDocument::fromJson(serializedReply ,&error);
	
	if (error.error != QJsonParseError::NoError)
	{
		Error(_log, "Error while parsing reply: invalid json");
		return;
	}

}

void JsonClientConnection::sendSuccessReply(const QString &command, const int tan)
{
	// create reply
	QJsonObject reply;
	reply["success"] = true;
	reply["command"] = command;
	reply["tan"] = tan;

	// send reply
	sendMessage(reply);
}

void JsonClientConnection::sendErrorReply(const QString &error, const QString &command, const int tan)
{
	// create reply
	QJsonObject reply;
	reply["success"] = false;
	reply["error"] = error;
	reply["command"] = command;
	reply["tan"] = tan;

	// send reply
	sendMessage(reply);
}

bool JsonClientConnection::checkJson(const QJsonObject& message, const QString& schemaResource, QString& errorMessage, bool ignoreRequired)
{
	// make sure the resources are loaded (they may be left out after static linking)
	Q_INIT_RESOURCE(JsonSchemas);
	QJsonParseError error;

	// read the json schema from the resource
	QFile schemaData(schemaResource);
	if (!schemaData.open(QIODevice::ReadOnly))
	{
		errorMessage = "Schema error: " + schemaData.errorString();
		return false;
	}

	// create schema checker
	QByteArray schema = schemaData.readAll();
	QJsonDocument schemaJson = QJsonDocument::fromJson(schema, &error);
	schemaData.close();
	
	if (error.error != QJsonParseError::NoError)
	{
		// report to the user the failure and their locations in the document.
		int errorLine(0), errorColumn(0);
		
		for( int i=0, count=qMin( error.offset,schema.size()); i<count; ++i )
		{
			++errorColumn;
			if(schema.at(i) == '\n' )
			{
				errorColumn = 0;
				++errorLine;
			}
		}
		
		std::stringstream sstream;
		sstream << "Schema error: " << error.errorString().toStdString() << " at Line: " << errorLine << ", Column: " << errorColumn;
		errorMessage = QString::fromStdString(sstream.str());
		return false;
	}
	
	QJsonSchemaChecker schemaChecker;
	schemaChecker.setSchema(schemaJson.object());

	// check the message
	if (!schemaChecker.validate(message, ignoreRequired))
	{
		const std::list<std::string> & errors = schemaChecker.getMessages();
		std::stringstream ss;
		ss << "{";
		foreach (const std::string & error, errors)
		{
			ss << error << " ";
		}
		ss << "}";
		errorMessage = QString::fromStdString(ss.str());
		return false;
	}

	return true;
}

void JsonClientConnection::streamLedcolorsUpdate()
{
	QJsonObject result;
	QJsonArray leds;
	
	const PriorityMuxer::InputInfo & priorityInfo = _hyperion->getPriorityInfo(_hyperion->getCurrentPriority());
	for(auto color = priorityInfo.ledColors.begin(); color != priorityInfo.ledColors.end(); ++color)
	{
		QJsonObject item;
		item["index"] = int(color - priorityInfo.ledColors.begin());
		item["red"]   = color->red;
		item["green"] = color->green;
		item["blue"]  = color->blue;
		leds.append(item);
	}
	
	result["leds"] = leds;
	_streaming_leds_reply["result"] = result;

	// send the result
	sendMessage(_streaming_leds_reply);

}
