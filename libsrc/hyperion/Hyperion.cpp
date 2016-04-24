
// STL includes
#include <cassert>

// QT includes
#include <QDateTime>
#include <QThread>
#include <QRegExp>
#include <QString>
#include <QStringList>

// JsonSchema include
#include <utils/jsonschema/JsonFactory.h>

// hyperion include
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ColorTransform.h>
#include <hyperion/ColorCorrection.h>
#include <hyperion/ColorAdjustment.h>

// Leddevice includes
#include <leddevice/LedDevice.h>
#include <leddevice/LedDeviceFactory.h>

#include "MultiColorTransform.h"
#include "MultiColorCorrection.h"
#include "MultiColorAdjustment.h"
#include "LinearColorSmoothing.h"

// effect engine includes
#include <effectengine/EffectEngine.h>


ColorOrder Hyperion::createColorOrder(const Json::Value &deviceConfig)
{
	// deprecated: force BGR when the deprecated flag is present and set to true
	if (deviceConfig.get("bgr-output", false).asBool())
	{
		return ORDER_BGR;
	}

	std::string order = deviceConfig.get("colorOrder", "rgb").asString();
	if (order == "rgb")
	{
		return ORDER_RGB;
	}
	else if (order == "bgr")
	{
		return ORDER_BGR;
	}
	else if (order == "rbg")
	{
		return ORDER_RBG;
	}
	else if (order == "brg")
	{
		return ORDER_BRG;
	}
	else if (order == "gbr")
	{
		return ORDER_GBR;
	}
	else if (order == "grb")
	{
		return ORDER_GRB;
	}
	else
	{
		std::cout << "HYPERION ERROR: Unknown color order defined (" << order << "). Using RGB." << std::endl;
	}

	return ORDER_RGB;
}

ColorTransform * Hyperion::createColorTransform(const Json::Value & transformConfig)
{
	const std::string id = transformConfig.get("id", "default").asString();

	RgbChannelTransform * redTransform   = createRgbChannelTransform(transformConfig["red"]);
	RgbChannelTransform * greenTransform = createRgbChannelTransform(transformConfig["green"]);
	RgbChannelTransform * blueTransform  = createRgbChannelTransform(transformConfig["blue"]);

	HsvTransform * hsvTransform = createHsvTransform(transformConfig["hsv"]);
	HslTransform * hslTransform = createHslTransform(transformConfig["hsl"]);

	ColorTransform * transform = new ColorTransform();
	transform->_id = id;
	transform->_rgbRedTransform   = *redTransform;
	transform->_rgbGreenTransform = *greenTransform;
	transform->_rgbBlueTransform  = *blueTransform;
	transform->_hsvTransform      = *hsvTransform;
	transform->_hslTransform      = *hslTransform;


	// Cleanup the allocated individual transforms
	delete redTransform;
	delete greenTransform;
	delete blueTransform;
	delete hsvTransform;
	delete hslTransform;

	return transform;
}


ColorCorrection * Hyperion::createColorCorrection(const Json::Value & correctionConfig)
{
	const std::string id = correctionConfig.get("id", "default").asString();

	RgbChannelCorrection * rgbCorrection   = createRgbChannelCorrection(correctionConfig["correctionValues"]);

	ColorCorrection * correction = new ColorCorrection();
	correction->_id = id;
	correction->_rgbCorrection   = *rgbCorrection;

	// Cleanup the allocated individual transforms
	delete rgbCorrection;

	return correction;
}


ColorAdjustment * Hyperion::createColorAdjustment(const Json::Value & adjustmentConfig)
{
	const std::string id = adjustmentConfig.get("id", "default").asString();

	RgbChannelAdjustment * redAdjustment   = createRgbChannelAdjustment(adjustmentConfig["pureRed"],RED);
	RgbChannelAdjustment * greenAdjustment = createRgbChannelAdjustment(adjustmentConfig["pureGreen"],GREEN);
	RgbChannelAdjustment * blueAdjustment  = createRgbChannelAdjustment(adjustmentConfig["pureBlue"],BLUE);

	ColorAdjustment * adjustment = new ColorAdjustment();
	adjustment->_id = id;
	adjustment->_rgbRedAdjustment   = *redAdjustment;
	adjustment->_rgbGreenAdjustment = *greenAdjustment;
	adjustment->_rgbBlueAdjustment  = *blueAdjustment;

	// Cleanup the allocated individual adjustments
	delete redAdjustment;
	delete greenAdjustment;
	delete blueAdjustment;

	return adjustment;
}


MultiColorTransform * Hyperion::createLedColorsTransform(const unsigned ledCnt, const Json::Value & colorConfig)
{
	// Create the result, the transforms are added to this
	MultiColorTransform * transform = new MultiColorTransform(ledCnt);

	const Json::Value transformConfig = colorConfig.get("transform", Json::nullValue);
	if (transformConfig.isNull())
	{
		// Old style color transformation config (just one for all leds)
		ColorTransform * colorTransform = createColorTransform(colorConfig);
		transform->addTransform(colorTransform);
		transform->setTransformForLed(colorTransform->_id, 0, ledCnt-1);
	}
	else if (!transformConfig.isArray())
	{
		ColorTransform * colorTransform = createColorTransform(transformConfig);
		transform->addTransform(colorTransform);
		transform->setTransformForLed(colorTransform->_id, 0, ledCnt-1);
	}
	else
	{
		const QRegExp overallExp("([0-9]+(\\-[0-9]+)?)(,[ ]*([0-9]+(\\-[0-9]+)?))*");

		for (Json::UInt i = 0; i < transformConfig.size(); ++i)
		{
			const Json::Value & config = transformConfig[i];
			ColorTransform * colorTransform = createColorTransform(config);
			transform->addTransform(colorTransform);

			const QString ledIndicesStr = QString(config.get("leds", "").asCString()).trimmed();
			if (ledIndicesStr.compare("*") == 0)
			{
				// Special case for indices '*' => all leds
				transform->setTransformForLed(colorTransform->_id, 0, ledCnt-1);
				std::cout << "HYPERION INFO: ColorTransform '" << colorTransform->_id << "' => [0; "<< ledCnt-1 << "]" << std::endl;
				continue;
			}

			if (!overallExp.exactMatch(ledIndicesStr))
			{
				std::cerr << "HYPERION ERROR: Given led indices " << i << " not correct format: " << ledIndicesStr.toStdString() << std::endl;
				continue;
			}

			std::cout << "HYPERION INFO: ColorTransform '" << colorTransform->_id << "' => [";

			const QStringList ledIndexList = ledIndicesStr.split(",");
			for (int i=0; i<ledIndexList.size(); ++i) {
				if (i > 0)
				{
					std::cout << ", ";
				}
				if (ledIndexList[i].contains("-"))
				{
					QStringList ledIndices = ledIndexList[i].split("-");
					int startInd = ledIndices[0].toInt();
					int endInd   = ledIndices[1].toInt();

					transform->setTransformForLed(colorTransform->_id, startInd, endInd);
					std::cout << startInd << "-" << endInd;
				}
				else
				{
					int index = ledIndexList[i].toInt();
					transform->setTransformForLed(colorTransform->_id, index, index);
					std::cout << index;
				}
			}
			std::cout << "]" << std::endl;
		}
	}
	return transform;
}

MultiColorCorrection * Hyperion::createLedColorsCorrection(const unsigned ledCnt, const Json::Value & colorConfig)
{
	// Create the result, the corrections are added to this
	MultiColorCorrection * correction = new MultiColorCorrection(ledCnt);

	const Json::Value correctionConfig = colorConfig.get("correction", Json::nullValue);
	if (correctionConfig.isNull())
	{
		// Old style color correction config (just one for all leds)
		ColorCorrection * colorCorrection = createColorCorrection(colorConfig);
		correction->addCorrection(colorCorrection);
		correction->setCorrectionForLed(colorCorrection->_id, 0, ledCnt-1);
	}
	else if (!correctionConfig.isArray())
	{
		ColorCorrection * colorCorrection = createColorCorrection(correctionConfig);
		correction->addCorrection(colorCorrection);
		correction->setCorrectionForLed(colorCorrection->_id, 0, ledCnt-1);
	}
	else
	{
		const QRegExp overallExp("([0-9]+(\\-[0-9]+)?)(,[ ]*([0-9]+(\\-[0-9]+)?))*");

		for (Json::UInt i = 0; i < correctionConfig.size(); ++i)
		{
			const Json::Value & config = correctionConfig[i];
			ColorCorrection * colorCorrection = createColorCorrection(config);
			correction->addCorrection(colorCorrection);

			const QString ledIndicesStr = QString(config.get("leds", "").asCString()).trimmed();
			if (ledIndicesStr.compare("*") == 0)
			{
				// Special case for indices '*' => all leds
				correction->setCorrectionForLed(colorCorrection->_id, 0, ledCnt-1);
				std::cout << "HYPERION INFO: ColorCorrection '" << colorCorrection->_id << "' => [0; "<< ledCnt-1 << "]" << std::endl;
				continue;
			}

			if (!overallExp.exactMatch(ledIndicesStr))
			{
				std::cerr << "HYPERION ERROR: Given led indices " << i << " not correct format: " << ledIndicesStr.toStdString() << std::endl;
				continue;
			}

			std::cout << "HYPERION INFO: ColorCorrection '" << colorCorrection->_id << "' => [";

			const QStringList ledIndexList = ledIndicesStr.split(",");
			for (int i=0; i<ledIndexList.size(); ++i) {
				if (i > 0)
				{
					std::cout << ", ";
				}
				if (ledIndexList[i].contains("-"))
				{
					QStringList ledIndices = ledIndexList[i].split("-");
					int startInd = ledIndices[0].toInt();
					int endInd   = ledIndices[1].toInt();

					correction->setCorrectionForLed(colorCorrection->_id, startInd, endInd);
					std::cout << startInd << "-" << endInd;
				}
				else
				{
					int index = ledIndexList[i].toInt();
					correction->setCorrectionForLed(colorCorrection->_id, index, index);
					std::cout << index;
				}
			}
			std::cout << "]" << std::endl;
		}
	}
	return correction;
}

MultiColorCorrection * Hyperion::createLedColorsTemperature(const unsigned ledCnt, const Json::Value & colorConfig)
{
	// Create the result, the corrections are added to this
	MultiColorCorrection * correction = new MultiColorCorrection(ledCnt);

	const Json::Value correctionConfig = colorConfig.get("temperature", Json::nullValue);
	if (correctionConfig.isNull())
	{
		// Old style color correction config (just one for all leds)
		ColorCorrection * colorCorrection = createColorCorrection(colorConfig);
		correction->addCorrection(colorCorrection);
		correction->setCorrectionForLed(colorCorrection->_id, 0, ledCnt-1);
	}
	else if (!correctionConfig.isArray())
	{
		ColorCorrection * colorCorrection = createColorCorrection(correctionConfig);
		correction->addCorrection(colorCorrection);
		correction->setCorrectionForLed(colorCorrection->_id, 0, ledCnt-1);
	}
	else
	{
		const QRegExp overallExp("([0-9]+(\\-[0-9]+)?)(,[ ]*([0-9]+(\\-[0-9]+)?))*");

		for (Json::UInt i = 0; i < correctionConfig.size(); ++i)
		{
			const Json::Value & config = correctionConfig[i];
			ColorCorrection * colorCorrection = createColorCorrection(config);
			correction->addCorrection(colorCorrection);

			const QString ledIndicesStr = QString(config.get("leds", "").asCString()).trimmed();
			if (ledIndicesStr.compare("*") == 0)
			{
				// Special case for indices '*' => all leds
				correction->setCorrectionForLed(colorCorrection->_id, 0, ledCnt-1);
				std::cout << "HYPERION INFO: ColorCorrection '" << colorCorrection->_id << "' => [0; "<< ledCnt-1 << "]" << std::endl;
				continue;
			}

			if (!overallExp.exactMatch(ledIndicesStr))
			{
				std::cerr << "HYPERION ERROR: Given led indices " << i << " not correct format: " << ledIndicesStr.toStdString() << std::endl;
				continue;
			}

			std::cout << "HYPERION INFO: ColorCorrection '" << colorCorrection->_id << "' => [";

			const QStringList ledIndexList = ledIndicesStr.split(",");
			for (int i=0; i<ledIndexList.size(); ++i) {
				if (i > 0)
				{
					std::cout << ", ";
				}
				if (ledIndexList[i].contains("-"))
				{
					QStringList ledIndices = ledIndexList[i].split("-");
					int startInd = ledIndices[0].toInt();
					int endInd   = ledIndices[1].toInt();

					correction->setCorrectionForLed(colorCorrection->_id, startInd, endInd);
					std::cout << startInd << "-" << endInd;
				}
				else
				{
					int index = ledIndexList[i].toInt();
					correction->setCorrectionForLed(colorCorrection->_id, index, index);
					std::cout << index;
				}
			}
			std::cout << "]" << std::endl;
		}
	}
	return correction;
}

MultiColorAdjustment * Hyperion::createLedColorsAdjustment(const unsigned ledCnt, const Json::Value & colorConfig)
{
	// Create the result, the transforms are added to this
	MultiColorAdjustment * adjustment = new MultiColorAdjustment(ledCnt);

	const Json::Value adjustmentConfig = colorConfig.get("channelAdjustment", Json::nullValue);
	if (adjustmentConfig.isNull())
	{
		// Old style color transformation config (just one for all leds)
		ColorAdjustment * colorAdjustment = createColorAdjustment(colorConfig);
		adjustment->addAdjustment(colorAdjustment);
		adjustment->setAdjustmentForLed(colorAdjustment->_id, 0, ledCnt-1);
	}
	else if (!adjustmentConfig.isArray())
	{
		ColorAdjustment * colorAdjustment = createColorAdjustment(adjustmentConfig);
		adjustment->addAdjustment(colorAdjustment);
		adjustment->setAdjustmentForLed(colorAdjustment->_id, 0, ledCnt-1);
	}
	else
	{
		const QRegExp overallExp("([0-9]+(\\-[0-9]+)?)(,[ ]*([0-9]+(\\-[0-9]+)?))*");

		for (Json::UInt i = 0; i < adjustmentConfig.size(); ++i)
		{
			const Json::Value & config = adjustmentConfig[i];
			ColorAdjustment * colorAdjustment = createColorAdjustment(config);
			adjustment->addAdjustment(colorAdjustment);

			const QString ledIndicesStr = QString(config.get("leds", "").asCString()).trimmed();
			if (ledIndicesStr.compare("*") == 0)
			{
				// Special case for indices '*' => all leds
				adjustment->setAdjustmentForLed(colorAdjustment->_id, 0, ledCnt-1);
				std::cout << "HYPERION INFO: ColorAdjustment '" << colorAdjustment->_id << "' => [0; "<< ledCnt-1 << "]" << std::endl;
				continue;
			}

			if (!overallExp.exactMatch(ledIndicesStr))
			{
				std::cerr << "HYPERION ERROR: Given led indices " << i << " not correct format: " << ledIndicesStr.toStdString() << std::endl;
				continue;
			}

			std::cout << "HYPERION INFO: ColorAdjustment '" << colorAdjustment->_id << "' => [";

			const QStringList ledIndexList = ledIndicesStr.split(",");
			for (int i=0; i<ledIndexList.size(); ++i) {
				if (i > 0)
				{
					std::cout << ", ";
				}
				if (ledIndexList[i].contains("-"))
				{
					QStringList ledIndices = ledIndexList[i].split("-");
					int startInd = ledIndices[0].toInt();
					int endInd   = ledIndices[1].toInt();

					adjustment->setAdjustmentForLed(colorAdjustment->_id, startInd, endInd);
					std::cout << startInd << "-" << endInd;
				}
				else
				{
					int index = ledIndexList[i].toInt();
					adjustment->setAdjustmentForLed(colorAdjustment->_id, index, index);
					std::cout << index;
				}
			}
			std::cout << "]" << std::endl;
		}
	}
	return adjustment;
}

HsvTransform * Hyperion::createHsvTransform(const Json::Value & hsvConfig)
{
	const double saturationGain = hsvConfig.get("saturationGain", 1.0).asDouble();
	const double valueGain      = hsvConfig.get("valueGain",      1.0).asDouble();

	return new HsvTransform(saturationGain, valueGain);
}

HslTransform * Hyperion::createHslTransform(const Json::Value & hslConfig)
{
	const double saturationGain = hslConfig.get("saturationGain", 1.0).asDouble();
	const double luminanceGain  = hslConfig.get("luminanceGain",  1.0).asDouble();

	return new HslTransform(saturationGain, luminanceGain);
}

RgbChannelTransform* Hyperion::createRgbChannelTransform(const Json::Value& colorConfig)
{
	const double threshold  = colorConfig.get("threshold", 0.0).asDouble();
	const double gamma      = colorConfig.get("gamma", 1.0).asDouble();
	const double blacklevel = colorConfig.get("blacklevel", 0.0).asDouble();
	const double whitelevel = colorConfig.get("whitelevel", 1.0).asDouble();

	RgbChannelTransform* transform = new RgbChannelTransform(threshold, gamma, blacklevel, whitelevel);
	return transform;
}

RgbChannelCorrection* Hyperion::createRgbChannelCorrection(const Json::Value& colorConfig)
{
	const int varR = colorConfig.get("red", 255).asInt();
	const int varG = colorConfig.get("green", 255).asInt();
	const int varB = colorConfig.get("blue", 255).asInt();

	RgbChannelCorrection* correction = new RgbChannelCorrection(varR, varG, varB);
	return correction;
}

RgbChannelAdjustment* Hyperion::createRgbChannelAdjustment(const Json::Value& colorConfig, const RgbChannel color)
{
	int varR, varG, varB;
	if (color == RED) 
	{
		varR = colorConfig.get("redChannel", 255).asInt();
		varG = colorConfig.get("greenChannel", 0).asInt();
		varB = colorConfig.get("blueChannel", 0).asInt();
	}
	else if (color == GREEN)
	{
		varR = colorConfig.get("redChannel", 0).asInt();
		varG = colorConfig.get("greenChannel", 255).asInt();
		varB = colorConfig.get("blueChannel", 0).asInt();
	}		
	else if (color == BLUE)
	{
		varR = colorConfig.get("redChannel", 0).asInt();
		varG = colorConfig.get("greenChannel", 0).asInt();
		varB = colorConfig.get("blueChannel", 255).asInt();
	}

	RgbChannelAdjustment* adjustment = new RgbChannelAdjustment(varR, varG, varB);
	return adjustment;
}

LedString Hyperion::createLedString(const Json::Value& ledsConfig, const ColorOrder deviceOrder)
{
	LedString ledString;

	const std::string deviceOrderStr = colorOrderToString(deviceOrder);
	for (const Json::Value& ledConfig : ledsConfig)
	{
		Led led;
		led.index = ledConfig["index"].asInt();

		const Json::Value& hscanConfig = ledConfig["hscan"];
		const Json::Value& vscanConfig = ledConfig["vscan"];
		led.minX_frac = std::max(0.0, std::min(1.0, hscanConfig["minimum"].asDouble()));
		led.maxX_frac = std::max(0.0, std::min(1.0, hscanConfig["maximum"].asDouble()));
		led.minY_frac = std::max(0.0, std::min(1.0, vscanConfig["minimum"].asDouble()));
		led.maxY_frac = std::max(0.0, std::min(1.0, vscanConfig["maximum"].asDouble()));

		// Fix if the user swapped min and max
		if (led.minX_frac > led.maxX_frac)
		{
			std::swap(led.minX_frac, led.maxX_frac);
		}
		if (led.minY_frac > led.maxY_frac)
		{
			std::swap(led.minY_frac, led.maxY_frac);
		}

		// Get the order of the rgb channels for this led (default is device order)
		const std::string ledOrderStr = ledConfig.get("colorOrder", deviceOrderStr).asString();
		led.colorOrder = stringToColorOrder(ledOrderStr);

		ledString.leds().push_back(led);
	}

	// Make sure the leds are sorted (on their indices)
	std::sort(ledString.leds().begin(), ledString.leds().end(), [](const Led& lhs, const Led& rhs){ return lhs.index < rhs.index; });

	return ledString;
}

LedDevice * Hyperion::createColorSmoothing(const Json::Value & smoothingConfig, LedDevice * ledDevice)
{
	std::string type = smoothingConfig.get("type", "none").asString();
	std::transform(type.begin(), type.end(), type.begin(), ::tolower);

	if (type == "none")
	{
		std::cout << "HYPERION INFO: Not creating any smoothing" << std::endl;
		return ledDevice;
	}
	else if (type == "linear")
	{
		if (!smoothingConfig.isMember("time_ms"))
		{
			std::cout << "HYPERION ERROR: Unable to create smoothing of type linear because of missing parameter 'time_ms'" << std::endl;
		}
		else if (!smoothingConfig.isMember("updateFrequency"))
		{
			std::cout << "HYPERION ERROR: Unable to create smoothing of type linear because of missing parameter 'updateFrequency'" << std::endl;
		}
		else
		{
			const unsigned updateDelay = smoothingConfig.get("updateDelay", Json::Value(0u)).asUInt();
			std::cout << "INFO: Creating linear smoothing" << std::endl;
			return new LinearColorSmoothing(
					ledDevice,
					smoothingConfig["updateFrequency"].asDouble(),
					smoothingConfig["time_ms"].asInt(),
					updateDelay);
		}
	}
	else
	{
		std::cout << "HYPERION ERROR: Unable to create smoothing of type " << type << std::endl;
	}

	return ledDevice;
}

MessageForwarder * Hyperion::createMessageForwarder(const Json::Value & forwarderConfig)
{
		MessageForwarder * forwarder = new MessageForwarder();
		if ( ! forwarderConfig.isNull() )
		{
			if ( ! forwarderConfig["json"].isNull() && forwarderConfig["json"].isArray() )
			{
				for (const Json::Value& addr : forwarderConfig["json"])
				{
					std::cout << "HYPERION INFO: Json forward to " << addr.asString() << std::endl;
					forwarder->addJsonSlave(addr.asString());
				}
			}

			if ( ! forwarderConfig["proto"].isNull() && forwarderConfig["proto"].isArray() )
			{
				for (const Json::Value& addr : forwarderConfig["proto"])
				{
					std::cout << "HYPERION INFO: Proto forward to " << addr.asString() << std::endl;
					forwarder->addProtoSlave(addr.asString());
				}
			}
		}

	return forwarder;
}

MessageForwarder * Hyperion::getForwarder()
{
	return _messageForwarder;
}

Hyperion::Hyperion(const Json::Value &jsonConfig) :
	_ledString(createLedString(jsonConfig["leds"], createColorOrder(jsonConfig["device"]))),
	_muxer(_ledString.leds().size()),
	_raw2ledAdjustment(createLedColorsAdjustment(_ledString.leds().size(), jsonConfig["color"])),
	_raw2ledCorrection(createLedColorsCorrection(_ledString.leds().size(), jsonConfig["color"])),
	_raw2ledTemperature(createLedColorsTemperature(_ledString.leds().size(), jsonConfig["color"])),
	_raw2ledTransform(createLedColorsTransform(_ledString.leds().size(), jsonConfig["color"])),
	_device(LedDeviceFactory::construct(jsonConfig["device"])),
	_effectEngine(nullptr),
	_messageForwarder(createMessageForwarder(jsonConfig["forwarder"])),
	_timer()
{
	if (!_raw2ledAdjustment->verifyAdjustments())
	{
		throw std::runtime_error("HYPERION ERROR: Color adjustment incorrectly set");
	}
	if (!_raw2ledCorrection->verifyCorrections())
	{
		throw std::runtime_error("HYPERION ERROR: Color correction incorrectly set");
	}
	if (!_raw2ledTemperature->verifyCorrections())
	{
		throw std::runtime_error("HYPERION ERROR: Color temperature incorrectly set");
	}
	if (!_raw2ledTransform->verifyTransforms())
	{
		throw std::runtime_error("HYPERION ERROR: Color transformation incorrectly set");
	}
	// initialize the image processor factory
	ImageProcessorFactory::getInstance().init(
				_ledString,
				jsonConfig["blackborderdetector"]
	);

	// initialize the color smoothing filter
	_device = createColorSmoothing(jsonConfig["color"]["smoothing"], _device);

	// setup the timer
	_timer.setSingleShot(true);
	QObject::connect(&_timer, SIGNAL(timeout()), this, SLOT(update()));

	// create the effect engine
	_effectEngine = new EffectEngine(this, jsonConfig["effects"]);

	// initialize the leds
	update();
}


Hyperion::~Hyperion()
{
	// switch off all leds
	clearall();
	_device->switchOff();

	// delete the effect engine
	delete _effectEngine;

	// Delete the Led-String
	delete _device;

	// delete the color transform
	delete _raw2ledTransform;
	
	// delete the color correction
	delete _raw2ledCorrection;

	// delete the color temperature correction
	delete _raw2ledTemperature;
	
	// delete the color adjustment
	delete _raw2ledAdjustment;

	// delete the message forwarder
	delete _messageForwarder;
}

unsigned Hyperion::getLedCount() const
{
	return _ledString.leds().size();
}

void Hyperion::setColor(int priority, const ColorRgb &color, const int timeout_ms, bool clearEffects)
{
	// create led output
	std::vector<ColorRgb> ledColors(_ledString.leds().size(), color);

	// set colors
	setColors(priority, ledColors, timeout_ms, clearEffects);
}

void Hyperion::setColors(int priority, const std::vector<ColorRgb>& ledColors, const int timeout_ms, bool clearEffects)
{
	// clear effects if this call does not come from an effect
	if (clearEffects)
	{
		_effectEngine->channelCleared(priority);
	}

	if (timeout_ms > 0)
	{
		const uint64_t timeoutTime = QDateTime::currentMSecsSinceEpoch() + timeout_ms;
		_muxer.setInput(priority, ledColors, timeoutTime);
	}
	else
	{
		_muxer.setInput(priority, ledColors);
	}

	if (priority == _muxer.getCurrentPriority())
	{
		update();
	}
}

const std::vector<std::string> & Hyperion::getTransformIds() const
{
	return _raw2ledTransform->getTransformIds();
}

const std::vector<std::string> & Hyperion::getCorrectionIds() const
{
	return _raw2ledCorrection->getCorrectionIds();
}

const std::vector<std::string> & Hyperion::getTemperatureIds() const
{
	return _raw2ledTemperature->getCorrectionIds();
}

const std::vector<std::string> & Hyperion::getAdjustmentIds() const
{
	return _raw2ledAdjustment->getAdjustmentIds();
}

ColorTransform * Hyperion::getTransform(const std::string& id)
{
	return _raw2ledTransform->getTransform(id);
}

ColorCorrection * Hyperion::getCorrection(const std::string& id)
{
	return _raw2ledCorrection->getCorrection(id);
}

ColorCorrection * Hyperion::getTemperature(const std::string& id)
{
	return _raw2ledTemperature->getCorrection(id);
}

ColorAdjustment * Hyperion::getAdjustment(const std::string& id)
{
	return _raw2ledAdjustment->getAdjustment(id);
}

void Hyperion::transformsUpdated()
{
	update();
}

void Hyperion::correctionsUpdated()
{
	update();
}

void Hyperion::temperaturesUpdated()
{
	update();
}

void Hyperion::adjustmentsUpdated()
{
	update();
}

void Hyperion::clear(int priority)
{
	if (_muxer.hasPriority(priority))
	{
		_muxer.clearInput(priority);

		// update leds if necessary
		if (priority < _muxer.getCurrentPriority())
		{
			update();
		}
	}

	// send clear signal to the effect engine
	// (outside the check so the effect gets cleared even when the effect is not sending colors)
	_effectEngine->channelCleared(priority);
}

void Hyperion::clearall()
{
	_muxer.clearAll();

	// update leds
	update();

	// send clearall signal to the effect engine
	_effectEngine->allChannelsCleared();
}

QList<int> Hyperion::getActivePriorities() const
{
	return _muxer.getPriorities();
}

const Hyperion::InputInfo &Hyperion::getPriorityInfo(const int priority) const
{
	return _muxer.getInputInfo(priority);
}

const std::list<EffectDefinition> & Hyperion::getEffects() const
{
	return _effectEngine->getEffects();
}

const std::list<ActiveEffectDefinition> & Hyperion::getActiveEffects()
{
	return _effectEngine->getActiveEffects();
}

int Hyperion::setEffect(const std::string &effectName, int priority, int timeout)
{
	return _effectEngine->runEffect(effectName, priority, timeout);
}

int Hyperion::setEffect(const std::string &effectName, const Json::Value &args, int priority, int timeout)
{
	return _effectEngine->runEffect(effectName, args, priority, timeout);
}

void Hyperion::update()
{
	// Update the muxer, cleaning obsolete priorities
	_muxer.setCurrentTime(QDateTime::currentMSecsSinceEpoch());

	// Obtain the current priority channel
	int priority = _muxer.getCurrentPriority();
	const PriorityMuxer::InputInfo & priorityInfo  = _muxer.getInputInfo(priority);

	// Apply the correction and the transform to each led and color-channel
	// Avoid applying correction, the same task is performed by adjustment
	// std::vector<ColorRgb> correctedColors = _raw2ledCorrection->applyCorrection(priorityInfo.ledColors);
	std::vector<ColorRgb> adjustedColors = _raw2ledAdjustment->applyAdjustment(priorityInfo.ledColors);
	std::vector<ColorRgb> transformColors =_raw2ledTransform->applyTransform(adjustedColors);
	std::vector<ColorRgb> ledColors = _raw2ledTemperature->applyCorrection(transformColors);
	const std::vector<Led>& leds = _ledString.leds();
	int i = 0;
	for (ColorRgb& color : ledColors)
	{
		const ColorOrder ledColorOrder = leds.at(i).colorOrder;
		// correct the color byte order
		switch (ledColorOrder)
		{
		case ORDER_RGB:
			// leave as it is
			break;
		case ORDER_BGR:
			std::swap(color.red, color.blue);
			break;
		case ORDER_RBG:
			std::swap(color.green, color.blue);
			break;
		case ORDER_GRB:
			std::swap(color.red, color.green);
			break;
		case ORDER_GBR:
		{
			uint8_t temp = color.red;
			color.red = color.green;
			color.green = color.blue;
			color.blue = temp;
			break;
		}
		case ORDER_BRG:
		{
			uint8_t temp = color.red;
			color.red = color.blue;
			color.blue = color.green;
			color.green = temp;
			break;
		}
		}
		i++;
	}

	// Write the data to the device
	_device->write(ledColors);

	// Start the timeout-timer
	if (priorityInfo.timeoutTime_ms == -1)
	{
		_timer.stop();
	}
	else
	{
		int timeout_ms = std::max(0, int(priorityInfo.timeoutTime_ms - QDateTime::currentMSecsSinceEpoch()));
		_timer.start(timeout_ms);
	}
}
