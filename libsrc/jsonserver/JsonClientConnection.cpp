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
#include <QImage>
#include <QBuffer>
#include <QByteArray>
#include <QIODevice>
#include <QDateTime>
#include <QHostInfo>

// hyperion util includes
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>
#include <hyperion/MessageForwarder.h>
#include <hyperion/ColorAdjustment.h>
#include <utils/ColorSys.h>
#include <utils/ColorRgb.h>
#include <leddevice/LedDevice.h>
#include <hyperion/GrabberWrapper.h>
#include <HyperionConfig.h>
#include <utils/jsonschema/QJsonFactory.h>
#include <utils/Process.h>
#include <utils/SysInfo.h>

// project includes
#include "JsonClientConnection.h"

using namespace hyperion;

int _connectionCounter = 0;
std::map<hyperion::Components, bool> JsonClientConnection::_componentsPrevState;

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
	, _image_stream_timeout(0)
	, _clientAddress(socket->peerAddress())
{
	// connect internal signals and slots
	connect(_socket, SIGNAL(disconnected()), this, SLOT(socketClosed()));
	connect(_socket, SIGNAL(readyRead()), this, SLOT(readData()));
	connect(_hyperion, SIGNAL(componentStateChanged(hyperion::Components,bool)), this, SLOT(componentStateChanged(hyperion::Components,bool)));

	_timer_ledcolors.setSingleShot(false);
	connect(&_timer_ledcolors, SIGNAL(timeout()), this, SLOT(streamLedcolorsUpdate()));
	_image_stream_mutex.unlock();
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
	}
	else
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
				QString message(QByteArray(_receiveBuffer.data(), bytes));

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
	if ((_receiveBuffer.at(0) & BHB0_FIN) == BHB0_FIN)
	{
		// final bit found, frame complete
		quint8 * maskKey = NULL;
		quint8 opCode = _receiveBuffer.at(0) & BHB0_OPCODE;
		bool isMasked = (_receiveBuffer.at(1) & BHB0_FIN) == BHB0_FIN;
		quint64 payloadLength = _receiveBuffer.at(1) & BHB1_PAYLOAD;
		quint32 index = 2;
		
		switch (payloadLength)
		{
			case payload_size_code_16bit:
				payloadLength = ((_receiveBuffer.at(2) << 8) & 0xFF00) | (_receiveBuffer.at(3) & 0xFF);
				index += 2;
				break;
			case payload_size_code_64bit:
				payloadLength = 0;
				for (uint i=0; i < 8; i++)
				{
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
			case OPCODE::TEXT:
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
		case OPCODE::CLOSE:
			{
				// close request, confirm
				quint8 close[] = {0x88, 0};
				_socket->write((const char*)close, 2);
				_socket->flush();
				_socket->close();
			}
			break;
		case OPCODE::PING:
			{
				// ping received, send pong
				quint8 pong[] = {OPCODE::PONG, 0};
				_socket->write((const char*)pong, 2);
				_socket->flush();
			}
			break;
		}
	}
	else
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
	QByteArray value = _receiveBuffer.mid(start, _receiveBuffer.indexOf("\r\n", start) - start);
	_receiveBuffer.clear();

	// must be always appended
	value += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

	// generate sha1 hash
	QByteArray hash = QCryptographicHash::hash(value, QCryptographicHash::Sha1);

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
		if      (command == "color")          handleColorCommand         (message, command, tan);
		else if (command == "image")          handleImageCommand         (message, command, tan);
		else if (command == "effect")         handleEffectCommand        (message, command, tan);
		else if (command == "create-effect")  handleCreateEffectCommand  (message, command, tan);
		else if (command == "delete-effect")  handleDeleteEffectCommand  (message, command, tan);
		else if (command == "serverinfo")     handleServerInfoCommand    (message, command, tan);
		else if (command == "sysinfo")        handleSysInfoCommand       (message, command, tan);
		else if (command == "clear")          handleClearCommand         (message, command, tan);
		else if (command == "clearall")       handleClearallCommand      (message, command, tan);
		else if (command == "adjustment")     handleAdjustmentCommand    (message, command, tan);
		else if (command == "sourceselect")   handleSourceSelectCommand  (message, command, tan);
		else if (command == "config")         handleConfigCommand        (message, command, tan);
		else if (command == "componentstate") handleComponentStateCommand(message, command, tan);
		else if (command == "ledcolors")      handleLedColorsCommand     (message, command, tan);
		else if (command == "logging")        handleLoggingCommand       (message, command, tan);
		else if (command == "processing")     handleProcessingCommand    (message, command, tan);
		else                                  handleNotImplemented       ();
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
	QString origin = message["origin"].toString() + "@"+QHostInfo::fromName(_clientAddress.toString()).hostName();

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
	_hyperion->setColors(priority, colorData, duration, true, hyperion::COMP_COLOR, origin);

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
	QString pythonScript = message["pythonScript"].toString();
	QString origin = message["origin"].toString() + "@"+_clientAddress.toString();
	const QJsonObject & effect = message["effect"].toObject();
	const QString & effectName = effect["name"].toString();

	// set output
	if (effect.contains("args"))
	{
		_hyperion->setEffect(effectName, effect["args"].toObject(), priority, duration, pythonScript, origin);
	}
	else
	{
		_hyperion->setEffect(effectName, priority, duration, origin);
	}

	// send reply
	sendSuccessReply(command, tan);
}

void JsonClientConnection::handleCreateEffectCommand(const QJsonObject& message, const QString &command, const int tan)
{
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
					if (message["name"].toString().trimmed().isEmpty() || message["name"].toString().trimmed().startsWith("."))
					{
						sendErrorReply("Can't save new effect. Effect name is empty or begins with a dot.", command, tan);
						return;
					}
					
					effectJson["name"] = message["name"].toString();
					effectJson["script"] = message["script"].toString();
					effectJson["args"] = message["args"].toObject();
					
					std::list<EffectDefinition> availableEffects = _hyperion->getEffects();
					std::list<EffectDefinition>::iterator iter = std::find_if(availableEffects.begin(), availableEffects.end(), find_effect(message["name"].toString()));
					
					QFileInfo newFileName;
					if (iter != availableEffects.end())
					{
						newFileName.setFile(iter->file);
						if (newFileName.absoluteFilePath().mid(0, 1)  == ":")
						{
							sendErrorReply("The effect name '" + message["name"].toString() + "' is assigned to an internal effect. Please rename your effekt.", command, tan);
							return;
						}
					} else
					{
						newFileName.setFile(effectArray[0].toString() + QDir::separator() + message["name"].toString().replace(QString(" "), QString("")) + QString(".json"));
						
						while(newFileName.exists())
						{
							newFileName.setFile(effectArray[0].toString() + QDir::separator() + newFileName.baseName() + QString::number(qrand() % ((10) - 0) + 0) + QString(".json"));
						}
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


void JsonClientConnection::handleSysInfoCommand(const QJsonObject&, const QString& command, const int tan)
{
	// create result
	QJsonObject result;
	QJsonObject info;
	result["success"] = true;
	result["command"] = command;
	result["tan"] = tan;
	
	SysInfo::HyperionSysInfo data = SysInfo::get();
	QJsonObject system;
	system["kernelType"    ] = data.kernelType;
	system["kernelVersion" ] = data.kernelVersion;
	system["architecture"  ] = data.architecture;
	system["wordSize"      ] = data.wordSize;
	system["productType"   ] = data.productType;
	system["productVersion"] = data.productVersion;
	system["prettyName"    ] = data.prettyName;
	system["hostName"      ] = data.hostName;
	system["domainName"    ] = data.domainName;
	info["system"] = system;

	QJsonObject hyperion;
	hyperion["jsonrpc_version" ] = QString(HYPERION_JSON_VERSION);
	hyperion["version"         ] = QString(HYPERION_VERSION);
	hyperion["build"           ] = QString(HYPERION_BUILD_ID);
	hyperion["time"            ] = QString(__DATE__ " " __TIME__);
	info["hyperion"] = hyperion;

	// send the result
	result["info" ] = info;
	sendMessage(result);
}


void JsonClientConnection::handleServerInfoCommand(const QJsonObject&, const QString& command, const int tan)
{
	// create result
	QJsonObject result;
	result["success"] = true;
	result["command"] = command;
	result["tan"] = tan;
	
	QJsonObject info;

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
		if (priorityInfo.timeoutTime_ms != -1 )
		{
			item["duration_ms"] = int(priorityInfo.timeoutTime_ms - now);
		}

		item["owner"]       = QString(hyperion::componentToIdString(priorityInfo.componentId));
		item["componentId"] = QString(hyperion::componentToIdString(priorityInfo.componentId));
		item["origin"] = priorityInfo.origin;
		item["active"] = true;
		item["visible"] = (priority == currentPriority);

		// remove item from prio register, because we have more valuable information via active priority
		QList<QString> prios = priorityRegister.keys(priority);
		if (! prios.empty())
		{
			item["owner"] = prios[0];
			priorityRegister.remove(prios[0]);
		}
		
		if(priorityInfo.componentId == hyperion::COMP_COLOR)
		{
			QJsonObject LEDcolor;
			
			// add RGB Value to Array
			QJsonArray RGBValue;
			RGBValue.append(priorityInfo.ledColors.begin()->red);
			RGBValue.append(priorityInfo.ledColors.begin()->green);
			RGBValue.append(priorityInfo.ledColors.begin()->blue);
			LEDcolor.insert("RGB", RGBValue);

			uint16_t Hue;
			float Saturation, Luminace;

			// add HSL Value to Array
			QJsonArray HSLValue;
			ColorSys::rgb2hsl(priorityInfo.ledColors.begin()->red,
					priorityInfo.ledColors.begin()->green,
					priorityInfo.ledColors.begin()->blue,
					Hue, Saturation, Luminace);

			HSLValue.append(Hue);
			HSLValue.append(Saturation);
			HSLValue.append(Luminace);
			LEDcolor.insert("HSL", HSLValue);

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
			LEDcolor.insert("HEX", HEXValue);
			
			item["value"] = LEDcolor;
		}
		// priorities[priorities.size()] = item;
		priorities.append(item);
	}

	// append left over priorities
	for(auto key : priorityRegister.keys())
	{
		QJsonObject item;
		item["priority"] = priorityRegister[key];
		item["active"]   = false;
		item["visible"]  = false;
		item["owner"]    = key;
		priorities.append(item);
	}

	info["priorities"] = priorities;
	info["priorities_autoselect"] = _hyperion->sourceAutoSelectEnabled();

	// collect adjustment information
	QJsonArray adjustmentArray;
	for (const QString& adjustmentId : _hyperion->getAdjustmentIds())
	{
		const ColorAdjustment * colorAdjustment = _hyperion->getAdjustment(adjustmentId);
		if (colorAdjustment == nullptr)
		{
			Error(_log, "Incorrect color adjustment id: %s", QSTRING_CSTR(adjustmentId));
			continue;
		}

		QJsonObject adjustment;
		adjustment["id"] = adjustmentId;

		QJsonArray blackAdjust;
		blackAdjust.append(colorAdjustment->_rgbBlackAdjustment.getAdjustmentR());
		blackAdjust.append(colorAdjustment->_rgbBlackAdjustment.getAdjustmentG());
		blackAdjust.append(colorAdjustment->_rgbBlackAdjustment.getAdjustmentB());
		adjustment.insert("black", blackAdjust);
		
		QJsonArray whiteAdjust;
		whiteAdjust.append(colorAdjustment->_rgbWhiteAdjustment.getAdjustmentR());
		whiteAdjust.append(colorAdjustment->_rgbWhiteAdjustment.getAdjustmentG());
		whiteAdjust.append(colorAdjustment->_rgbWhiteAdjustment.getAdjustmentB());
		adjustment.insert("white", whiteAdjust);
		
		QJsonArray redAdjust;
		redAdjust.append(colorAdjustment->_rgbRedAdjustment.getAdjustmentR());
		redAdjust.append(colorAdjustment->_rgbRedAdjustment.getAdjustmentG());
		redAdjust.append(colorAdjustment->_rgbRedAdjustment.getAdjustmentB());
		adjustment.insert("red", redAdjust);

		QJsonArray greenAdjust;
		greenAdjust.append(colorAdjustment->_rgbGreenAdjustment.getAdjustmentR());
		greenAdjust.append(colorAdjustment->_rgbGreenAdjustment.getAdjustmentG());
		greenAdjust.append(colorAdjustment->_rgbGreenAdjustment.getAdjustmentB());
		adjustment.insert("green", greenAdjust);

		QJsonArray blueAdjust;
		blueAdjust.append(colorAdjustment->_rgbBlueAdjustment.getAdjustmentR());
		blueAdjust.append(colorAdjustment->_rgbBlueAdjustment.getAdjustmentG());
		blueAdjust.append(colorAdjustment->_rgbBlueAdjustment.getAdjustmentB());
		adjustment.insert("blue", blueAdjust);
		
		QJsonArray cyanAdjust;
		cyanAdjust.append(colorAdjustment->_rgbCyanAdjustment.getAdjustmentR());
		cyanAdjust.append(colorAdjustment->_rgbCyanAdjustment.getAdjustmentG());
		cyanAdjust.append(colorAdjustment->_rgbCyanAdjustment.getAdjustmentB());
		adjustment.insert("cyan", cyanAdjust);
		
		QJsonArray magentaAdjust;
		magentaAdjust.append(colorAdjustment->_rgbMagentaAdjustment.getAdjustmentR());
		magentaAdjust.append(colorAdjustment->_rgbMagentaAdjustment.getAdjustmentG());
		magentaAdjust.append(colorAdjustment->_rgbMagentaAdjustment.getAdjustmentB());
		adjustment.insert("magenta", magentaAdjust);

		QJsonArray yellowAdjust;
		yellowAdjust.append(colorAdjustment->_rgbYellowAdjustment.getAdjustmentR());
		yellowAdjust.append(colorAdjustment->_rgbYellowAdjustment.getAdjustmentG());
		yellowAdjust.append(colorAdjustment->_rgbYellowAdjustment.getAdjustmentB());
		adjustment.insert("yellow", yellowAdjust);

		adjustment["backlightThreshold"] = colorAdjustment->_rgbTransform.getBacklightThreshold();
		adjustment["backlightColored"]   = colorAdjustment->_rgbTransform.getBacklightColored();
		adjustment["brightness"] = colorAdjustment->_rgbTransform.getBrightness();
		adjustment["brightnessCompensation"] = colorAdjustment->_rgbTransform.getBrightnessCompensation();
		adjustment["gammaRed"]   = colorAdjustment->_rgbTransform.getGammaR();
		adjustment["gammaGreen"] = colorAdjustment->_rgbTransform.getGammaG();
		adjustment["gammaBlue"]  = colorAdjustment->_rgbTransform.getGammaB();

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

	// get available led devices
	QJsonObject ledDevices;
	ledDevices["active"] =LedDevice::activeDevice();
	QJsonArray availableLedDevices;
	for (auto dev: LedDevice::getDeviceMap())
	{
		availableLedDevices.append(dev.first);
	}
	
	ledDevices["available"] = availableLedDevices;
	info["ledDevices"] = ledDevices;

	// get available grabbers
	QJsonObject grabbers;
	//grabbers["active"] = ????;
	QJsonArray availableGrabbers;
	for (auto grabber: GrabberWrapper::availableGrabbers())
	{
		availableGrabbers.append(grabber);
	}
	
	grabbers["available"] = availableGrabbers;
	info["grabbers"] = grabbers;

	// get available components
	QJsonArray component;
	std::map<hyperion::Components, bool> components = _hyperion->getComponentRegister().getRegister();
	for(auto comp : components)
	{
		QJsonObject item;
		item["name"] = QString::fromStdString(hyperion::componentToIdString(comp.first));
		item["enabled"] = comp.second;
		
		component.append(item);
	}
	
	info["components"] = component;
	info["ledMAppingType"] = ImageProcessor::mappingTypeToStr(_hyperion->getLedMappingType());
	
	// Add Hyperion 
	QJsonObject hyperion;
	hyperion["config_modified" ] = _hyperion->configModified();
	hyperion["config_writeable"] = _hyperion->configWriteable();
	hyperion["off"] = hyperionIsActive()? false : true;

	// sessions
	QJsonArray sessions;
	for (auto session: _hyperion->getHyperionSessions())
	{
		if (session.port<0) continue;
		QJsonObject item;
		item["name"]   = session.serviceName;
		item["type"]   = session.registeredType;
		item["domain"] = session.replyDomain;
		item["host"]   = session.hostName;
		item["address"]= session.address;
		item["port"]   = session.port;
		sessions.append(item);
	}
	hyperion["sessions"] = sessions;

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

void JsonClientConnection::handleAdjustmentCommand(const QJsonObject& message, const QString& command, const int tan)
{
	const QJsonObject & adjustment = message["adjustment"].toObject();

	const QString adjustmentId = adjustment["id"].toString(_hyperion->getAdjustmentIds().first());
	ColorAdjustment * colorAdjustment = _hyperion->getAdjustment(adjustmentId);
	if (colorAdjustment == nullptr)
	{
		Warning(_log, "Incorrect adjustment identifier: %s", adjustmentId.toStdString().c_str());
		return;
	}
		
	if (adjustment.contains("red"))
	{
		const QJsonArray & values = adjustment["red"].toArray();
		colorAdjustment->_rgbRedAdjustment.setAdjustment(values[0u].toInt(), values[1u].toInt(), values[2u].toInt());
	}

	if (adjustment.contains("green"))
	{
		const QJsonArray & values = adjustment["green"].toArray();
		colorAdjustment->_rgbGreenAdjustment.setAdjustment(values[0u].toInt(), values[1u].toInt(), values[2u].toInt());
	}

	if (adjustment.contains("blue"))
	{
		const QJsonArray & values = adjustment["blue"].toArray();
		colorAdjustment->_rgbBlueAdjustment.setAdjustment(values[0u].toInt(), values[1u].toInt(), values[2u].toInt());
	}	
	if (adjustment.contains("cyan"))
	{
		const QJsonArray & values = adjustment["cyan"].toArray();
		colorAdjustment->_rgbCyanAdjustment.setAdjustment(values[0u].toInt(), values[1u].toInt(), values[2u].toInt());
	}	
	if (adjustment.contains("magenta"))
	{
		const QJsonArray & values = adjustment["magenta"].toArray();
		colorAdjustment->_rgbMagentaAdjustment.setAdjustment(values[0u].toInt(), values[1u].toInt(), values[2u].toInt());
	}	
	if (adjustment.contains("yellow"))
	{
		const QJsonArray & values = adjustment["yellow"].toArray();
		colorAdjustment->_rgbYellowAdjustment.setAdjustment(values[0u].toInt(), values[1u].toInt(), values[2u].toInt());
	}	
	if (adjustment.contains("black"))
	{
		const QJsonArray & values = adjustment["black"].toArray();
		colorAdjustment->_rgbBlackAdjustment.setAdjustment(values[0u].toInt(), values[1u].toInt(), values[2u].toInt());
	}	
	if (adjustment.contains("white"))
	{
		const QJsonArray & values = adjustment["white"].toArray();
		colorAdjustment->_rgbWhiteAdjustment.setAdjustment(values[0u].toInt(), values[1u].toInt(), values[2u].toInt());
	}	

	if (adjustment.contains("gammaRed"))
	{
		colorAdjustment->_rgbTransform.setGamma(adjustment["gammaRed"].toDouble(), colorAdjustment->_rgbTransform.getGammaG(), colorAdjustment->_rgbTransform.getGammaB());
	}
	if (adjustment.contains("gammaGreen"))
	{
		colorAdjustment->_rgbTransform.setGamma(colorAdjustment->_rgbTransform.getGammaR(), adjustment["gammaGreen"].toDouble(), colorAdjustment->_rgbTransform.getGammaB());
	}
	if (adjustment.contains("gammaBlue"))
	{
		colorAdjustment->_rgbTransform.setGamma(colorAdjustment->_rgbTransform.getGammaR(), colorAdjustment->_rgbTransform.getGammaG(), adjustment["gammaBlue"].toDouble());
	}

	if (adjustment.contains("backlightThreshold"))
	{
		colorAdjustment->_rgbTransform.setBacklightThreshold(adjustment["backlightThreshold"].toDouble());
	}	
	if (adjustment.contains("backlightColored"))
	{
		colorAdjustment->_rgbTransform.setBacklightColored(adjustment["backlightColored"].toBool());
	}	
	if (adjustment.contains("brightness"))
	{
		colorAdjustment->_rgbTransform.setBrightness(adjustment["brightness"].toInt());
	}	
	if (adjustment.contains("brightnessCompensation"))
	{
		colorAdjustment->_rgbTransform.setBrightnessCompensation(adjustment["brightnessCompensation"].toInt());
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
	else if (subcommand == "reload")
	{
		_hyperion->freeObjects(true);
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
	
	try
	{
		result["result"] = QJsonFactory::readConfig(_hyperion->getConfigFileName());
	}
	catch(...)
	{
		result["result"] = _hyperion->getQJsonConfig();
	}

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
	QFile schemaData(":/hyperion-schema-"+QString::number(_hyperion->getConfigVersionId()));

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

void JsonClientConnection::handleComponentStateCommand(const QJsonObject& message, const QString &command, const int tan)
{
	const QJsonObject & componentState = message["componentstate"].toObject();
	
	QString compStr   = componentState["component"].toString("invalid");
	bool    compState = componentState["state"].toBool(true);

	if (compStr == "ALL" )
	{
		if (hyperionIsActive() != compState)
		{
			std::map<hyperion::Components, bool> components = _hyperion->getComponentRegister().getRegister();

			if (!compState)
			{
				JsonClientConnection::_componentsPrevState = components;
			}

			for(auto comp : components)
			{
				_hyperion->setComponentState(comp.first, compState ? JsonClientConnection::_componentsPrevState[comp.first] : false);
			}

			if (compState)
			{
				JsonClientConnection::_componentsPrevState.clear();
			}
		}

		sendSuccessReply(command, tan);
		return;

	}
	else
	{
		Components component = stringToComponent(compStr);

		if (hyperionIsActive())
		{
			if (component != COMP_INVALID)
			{
				_hyperion->setComponentState(component, compState);
				sendSuccessReply(command, tan);
				return;
			}
			sendErrorReply("invalid component name", command, tan);
			return;
		}
		sendErrorReply("can't change component state when hyperion is off", command, tan);
	}
}

void JsonClientConnection::handleLedColorsCommand(const QJsonObject& message, const QString &command, const int tan)
{
	// create result
	QString subcommand = message["subcommand"].toString("");

	if (subcommand == "ledstream-start")
	{
		_streaming_leds_reply["success"] = true;
		_streaming_leds_reply["command"] = command+"-ledstream-update";
		_streaming_leds_reply["tan"]     = tan;
		_timer_ledcolors.start(125);
	}
	else if (subcommand == "ledstream-stop")
	{
		_timer_ledcolors.stop();
	}
	else if (subcommand == "imagestream-start")
	{
		_streaming_image_reply["success"] = true;
		_streaming_image_reply["command"] = command+"-imagestream-update";
		_streaming_image_reply["tan"]     = tan;
		connect(_hyperion, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), this, SLOT(setImage(int, const Image<ColorRgb>&, const int)) );
	}
	else if (subcommand == "imagestream-stop")
	{
		disconnect(_hyperion, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), this, 0 );
	}
	else
	{
		sendErrorReply("unknown subcommand \""+subcommand+"\"",command,tan);
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
	_streaming_logging_reply["tan"]     = tan;
	
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

void JsonClientConnection::handleProcessingCommand(const QJsonObject& message, const QString &command, const int tan)
{
	_hyperion->setLedMappingType(ImageProcessor::mappingTypeToInt( message["mappingType"].toString("multicolor_mean")) );

	sendSuccessReply(command, tan);
}

void JsonClientConnection::incommingLogMessage(Logger::T_LOG_MESSAGE msg)
{
	QJsonObject result, message;
	QJsonArray messageArray;

	if (!_streaming_logging_activated)
	{
		_streaming_logging_activated = true;
		QVector<Logger::T_LOG_MESSAGE>* logBuffer = LoggerManager::getInstance()->getLogMessageBuffer();
		for(int i=0; i<logBuffer->length(); i++)
		{
			message["appName"] = logBuffer->at(i).appName;
			message["loggerName"] = logBuffer->at(i).loggerName;
			message["function"] = logBuffer->at(i).function;
			message["line"] = QString::number(logBuffer->at(i).line);
			message["fileName"] = logBuffer->at(i).fileName;
			message["message"] = logBuffer->at(i).message;
			message["levelString"] = logBuffer->at(i).levelString;
			
			messageArray.append(message);
		}
	}
	else
	{
		message["appName"] = msg.appName;
		message["loggerName"] = msg.loggerName;
		message["function"] = msg.function;
		message["line"] = QString::number(msg.line);
		message["fileName"] = msg.fileName;
		message["message"] = msg.message;
		message["levelString"] = msg.levelString;
		
		messageArray.append(message);
	}

	result.insert("messages", messageArray);
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
		
		errorMessage = "Schema error: " + error.errorString() + " at Line: " + QString::number(errorLine) + ", Column: " + QString::number(errorColumn);
		return false;
	}
	
	QJsonSchemaChecker schemaChecker;
	schemaChecker.setSchema(schemaJson.object());

	// check the message
	if (!schemaChecker.validate(message, ignoreRequired))
	{
		const QStringList & errors = schemaChecker.getMessages();
		errorMessage = "{";
		foreach (auto & error, errors)
		{
			errorMessage += error + " ";
		}
		errorMessage += "}";
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

void JsonClientConnection::setImage(int priority, const Image<ColorRgb> & image, int duration_ms)
{
	if ( (_image_stream_timeout+250) < QDateTime::currentMSecsSinceEpoch() && _image_stream_mutex.tryLock(0) )
	{
		_image_stream_timeout = QDateTime::currentMSecsSinceEpoch();

		QImage jpgImage((const uint8_t *) image.memptr(), image.width(), image.height(), 3*image.width(), QImage::Format_RGB888);
		QByteArray ba;
		QBuffer buffer(&ba);
		buffer.open(QIODevice::WriteOnly);
		jpgImage.save(&buffer, "jpg");
		
		QJsonObject result;
		result["image"] = "data:image/jpg;base64,"+QString(ba.toBase64());
		_streaming_image_reply["result"] = result;
		sendMessage(_streaming_image_reply);

		_image_stream_mutex.unlock();
	}
}


