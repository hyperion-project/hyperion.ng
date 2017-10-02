// project includes
#include <utils/JsonProcessor.h>

// stl includes
#include <iostream>
#include <sstream>
#include <iterator>
#include <iomanip>

// Qt includes
#include <QResource>
#include <QDateTime>
#include <QCryptographicHash>
#include <QImage>
#include <QBuffer>
#include <QByteArray>
#include <QFileInfo>
#include <QDir>
#include <QIODevice>
#include <QDateTime>

// hyperion includes
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>
#include <utils/jsonschema/QJsonFactory.h>
#include <utils/SysInfo.h>
#include <HyperionConfig.h>
#include <utils/ColorSys.h>
#include <leddevice/LedDevice.h>
#include <hyperion/GrabberWrapper.h>
#include <utils/Process.h>
#include <utils/JsonUtils.h>

using namespace hyperion;

std::map<hyperion::Components, bool> JsonProcessor::_componentsPrevState;

JsonProcessor::JsonProcessor(QString peerAddress, Logger* log, bool noListener)
	: QObject()
	, _peerAddress(peerAddress)
	, _log(log)
	, _hyperion(Hyperion::getInstance())
	, _imageProcessor(ImageProcessorFactory::getInstance().newImageProcessor())
	, _streaming_logging_activated(false)
	, _image_stream_timeout(0)
{
	// notify hyperion about a jsonMessageForward
	connect(this, &JsonProcessor::forwardJsonMessage, _hyperion, &Hyperion::forwardJsonMessage);
	// notify hyperion about a push emit TODO: Remove! Make sure that the target of the commands trigger this (less error margin) instead this instance
	connect(this, &JsonProcessor::pushReq, _hyperion, &Hyperion::hyperionStateChanged);

	if(!noListener)
	{
		// listen for sendServerInfo pushes from hyperion
		connect(_hyperion, &Hyperion::sendServerInfo, this, &JsonProcessor::forceServerInfo);
	}

	// led color stream update timer
	_timer_ledcolors.setSingleShot(false);
	connect(&_timer_ledcolors, SIGNAL(timeout()), this, SLOT(streamLedcolorsUpdate()));
	_image_stream_mutex.unlock();
}

JsonProcessor::~JsonProcessor()
{

}

void JsonProcessor::handleMessage(const QString& messageString, const QString peerAddress)
{
	if(!peerAddress.isNull())
		_peerAddress = peerAddress;

	const QString ident = "JsonRpc@"+_peerAddress;
	Q_INIT_RESOURCE(JSONRPC_schemas);
	QJsonObject message;
	// parse the message
	if(!JsonUtils::parse(ident, messageString, message, _log))
	{
		sendErrorReply("Errors during message parsing, please consult the Hyperion Log. Data:"+messageString);
		return;
	}

	// check basic message
	if(!JsonUtils::validate(ident, message, ":schema", _log))
	{
		sendErrorReply("Errors during message validation, please consult the Hyperion Log.");
		return;
	}

	// check specific message
	const QString command = message["command"].toString();
	if(!JsonUtils::validate(ident, message, QString(":schema-%1").arg(command), _log))
	{
		sendErrorReply("Errors during specific message validation, please consult the Hyperion Log");
		return;
	}

	int tan = message["tan"].toInt(0);
	// switch over all possible commands and handle them
	if      (command == "color")          handleColorCommand         (message, command, tan);
	else if (command == "image")          handleImageCommand         (message, command, tan);
	else if (command == "effect")         handleEffectCommand        (message, command, tan);
	else if (command == "create-effect")  handleCreateEffectCommand  (message, command, tan);
	else if (command == "delete-effect")  handleDeleteEffectCommand  (message, command, tan);
	else if (command == "sysinfo")        handleSysInfoCommand       (message, command, tan);
	else if (command == "serverinfo")     handleServerInfoCommand    (message, command, tan);
	else if (command == "clear")          handleClearCommand         (message, command, tan);
	else if (command == "clearall")       handleClearallCommand      (message, command, tan);
	else if (command == "adjustment")     handleAdjustmentCommand    (message, command, tan);
	else if (command == "sourceselect")   handleSourceSelectCommand  (message, command, tan);
	else if (command == "config")         handleConfigCommand        (message, command, tan);
	else if (command == "componentstate") handleComponentStateCommand(message, command, tan);
	else if (command == "ledcolors")      handleLedColorsCommand     (message, command, tan);
	else if (command == "logging")        handleLoggingCommand       (message, command, tan);
	else if (command == "processing")     handleProcessingCommand    (message, command, tan);
	else if (command == "videomode")      handleVideoModeCommand     (message, command, tan);
	else                                  handleNotImplemented       ();
}

void JsonProcessor::handleColorCommand(const QJsonObject& message, const QString& command, const int tan)
{
	emit forwardJsonMessage(message);

	// extract parameters
	int priority = message["priority"].toInt();
	int duration = message["duration"].toInt(-1);
	QString origin = message["origin"].toString() + "@"+_peerAddress;

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

void JsonProcessor::handleImageCommand(const QJsonObject& message, const QString& command, const int tan)
{
	emit forwardJsonMessage(message);

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

void JsonProcessor::handleEffectCommand(const QJsonObject& message, const QString& command, const int tan)
{
	emit forwardJsonMessage(message);

	// extract parameters
	int priority = message["priority"].toInt();
	int duration = message["duration"].toInt(-1);
	QString pythonScript = message["pythonScript"].toString();
	QString origin = message["origin"].toString() + "@"+_peerAddress;
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

void JsonProcessor::handleCreateEffectCommand(const QJsonObject& message, const QString &command, const int tan)
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
			if(!JsonUtils::validate("JsonRpc@"+_peerAddress, message["args"].toObject(), it->schemaFile, _log))
			{
				sendErrorReply("Error during arg validation against schema, please consult the Hyperion Log", command, tan);
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
					QString f = FileUtils::convertPath(effectArray[0].toString() + "/" + message["name"].toString().replace(QString(" "), QString("")) + QString(".json"));
					newFileName.setFile(f);
				}

				if(!JsonUtils::write(newFileName.absoluteFilePath(), effectJson, _log))
				{
					sendErrorReply("Error while saving effect, please check the Hyperion Log", command, tan);
					return;
				}

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
}

void JsonProcessor::handleDeleteEffectCommand(const QJsonObject& message, const QString& command, const int tan)
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
}

void JsonProcessor::handleSysInfoCommand(const QJsonObject&, const QString& command, const int tan)
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
	hyperion["id"              ] = _hyperion->id;
	info["hyperion"] = hyperion;

	// send the result
	result["info" ] = info;
	emit callbackMessage(result);
}

void JsonProcessor::handleServerInfoCommand(const QJsonObject&, const QString& command, const int tan)
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
	ledDevices["active"] = LedDevice::activeDevice();
	QJsonArray availableLedDevices;
	for (auto dev: LedDevice::getDeviceMap())
	{
		availableLedDevices.append(dev.first);
	}

	ledDevices["available"] = availableLedDevices;
	info["ledDevices"] = ledDevices;

	QJsonObject grabbers;
	QJsonArray availableGrabbers;
#if defined(ENABLE_DISPMANX) || defined(ENABLE_V4L2) || defined(ENABLE_FB) || defined(ENABLE_AMLOGIC) || defined(ENABLE_OSX) || defined(ENABLE_X11)
	// get available grabbers
	//grabbers["active"] = ????;
	for (auto grabber: GrabberWrapper::availableGrabbers())
	{
		availableGrabbers.append(grabber);
	}
#endif
	grabbers["available"] = availableGrabbers;
	grabbers["videomode"] = QString(videoMode2String(_hyperion->getCurrentVideoMode()));
	info["grabbers"]      = grabbers;

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
	emit callbackMessage(result);
}

void JsonProcessor::handleClearCommand(const QJsonObject& message, const QString& command, const int tan)
{
	emit forwardJsonMessage(message);

	// extract parameters
	int priority = message["priority"].toInt();

	// clear priority
	_hyperion->clear(priority);

	// send reply
	sendSuccessReply(command, tan);
}

void JsonProcessor::handleClearallCommand(const QJsonObject& message, const QString& command, const int tan)
{
	emit forwardJsonMessage(message);

	// clear priority
	_hyperion->clearall();

	// send reply
	sendSuccessReply(command, tan);
}

void JsonProcessor::handleAdjustmentCommand(const QJsonObject& message, const QString& command, const int tan)
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

void JsonProcessor::handleSourceSelectCommand(const QJsonObject& message, const QString& command, const int tan)
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

void JsonProcessor::handleConfigCommand(const QJsonObject& message, const QString& command, const int tan)
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

void JsonProcessor::handleConfigGetCommand(const QJsonObject& message, const QString& command, const int tan)
{
	// create result
	QJsonObject result;
	result["success"] = true;
	result["command"] = command;
	result["tan"] = tan;

	result["result"] = _hyperion->getQJsonConfig();

	// send the result
	emit callbackMessage(result);
}

void JsonProcessor::handleSchemaGetCommand(const QJsonObject& message, const QString& command, const int tan)
{
	// create result
	QJsonObject result, schemaJson, alldevices, properties;
	result["success"] = true;
	result["command"] = command;
	result["tan"] = tan;

	// make sure the resources are loaded (they may be left out after static linking)
	Q_INIT_RESOURCE(resource);

	// read the hyperion json schema from the resource
	QString schemaFile = ":/hyperion-schema";

	try
	{
		schemaJson = QJsonFactory::readSchema(schemaFile);
	}
	catch(const std::runtime_error& error)
	{
		throw std::runtime_error(error.what());
	}

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
	emit callbackMessage(result);
}

void JsonProcessor::handleComponentStateCommand(const QJsonObject& message, const QString &command, const int tan)
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
				JsonProcessor::_componentsPrevState = components;
			}

			for(auto comp : components)
			{
				_hyperion->setComponentState(comp.first, compState ? JsonProcessor::_componentsPrevState[comp.first] : false);
			}

			if (compState)
			{
				JsonProcessor::_componentsPrevState.clear();
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

void JsonProcessor::handleLedColorsCommand(const QJsonObject& message, const QString &command, const int tan)
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

void JsonProcessor::handleLoggingCommand(const QJsonObject& message, const QString &command, const int tan)
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
			Debug(_log, "log streaming activated for client %s",_peerAddress.toStdString().c_str()); // needed to trigger log sending
		}
	}
	else if (subcommand == "stop")
	{
		if (_streaming_logging_activated)
		{
			disconnect(LoggerManager::getInstance(), SIGNAL(newLogMessage(Logger::T_LOG_MESSAGE)), this, 0);
			_streaming_logging_activated = false;
			Debug(_log, "log streaming deactivated for client  %s",_peerAddress.toStdString().c_str());

		}
	}
	else
	{
		sendErrorReply("unknown subcommand",command,tan);
		return;
	}

	sendSuccessReply(command+"-"+subcommand,tan);
}

void JsonProcessor::handleProcessingCommand(const QJsonObject& message, const QString &command, const int tan)
{
	_hyperion->setLedMappingType(ImageProcessor::mappingTypeToInt( message["mappingType"].toString("multicolor_mean")) );

	sendSuccessReply(command, tan);
}

void JsonProcessor::handleVideoModeCommand(const QJsonObject& message, const QString &command, const int tan)
{
	_hyperion->setVideoMode(parse3DMode(message["videoMode"].toString("2D")));

	sendSuccessReply(command, tan);
}

void JsonProcessor::handleNotImplemented()
{
	sendErrorReply("Command not implemented");
}

void JsonProcessor::sendSuccessReply(const QString &command, const int tan)
{
	// create reply
	QJsonObject reply;
	reply["success"] = true;
	reply["command"] = command;
	reply["tan"] = tan;

	// send reply
	emit callbackMessage(reply);

	// blacklisted commands for emitter
	QVector<QString> vector;
	vector << "ledcolors-imagestream-stop" << "ledcolors-imagestream-start" << "ledcolors-ledstream-stop" << "ledcolors-ledstream-start" << "logging-start" << "logging-stop";
	if(vector.indexOf(command) == -1)
	{
		emit pushReq();
	}
}

void JsonProcessor::sendErrorReply(const QString &error, const QString &command, const int tan)
{
	// create reply
	QJsonObject reply;
	reply["success"] = false;
	reply["error"] = error;
	reply["command"] = command;
	reply["tan"] = tan;

	// send reply
	emit callbackMessage(reply);
}


void JsonProcessor::streamLedcolorsUpdate()
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
	emit callbackMessage(_streaming_leds_reply);
}

void JsonProcessor::setImage(int priority, const Image<ColorRgb> & image, int duration_ms)
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
		emit callbackMessage(_streaming_image_reply);

		_image_stream_mutex.unlock();
	}
}

void JsonProcessor::incommingLogMessage(Logger::T_LOG_MESSAGE msg)
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
	emit callbackMessage(_streaming_logging_reply);
}

void JsonProcessor::forceServerInfo()
{
	const QString command("serverinfo");
	const int tan = 1;
	const QJsonObject obj;
	handleServerInfoCommand(obj,command,tan);
}
