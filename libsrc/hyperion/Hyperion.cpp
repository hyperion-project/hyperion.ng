
// STL includes
#include <cassert>
#include <exception>
#include <sstream>

// QT includes
#include <QDateTime>
#include <QThread>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>

// hyperion include
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>
#include <hyperion/ColorTransform.h>
#include <hyperion/ColorAdjustment.h>

// Leddevice includes
#include <leddevice/LedDevice.h>
#include <leddevice/LedDeviceFactory.h>

#include "MultiColorTransform.h"
#include "MultiColorAdjustment.h"
#include "LinearColorSmoothing.h"

// effect engine includes
#include <effectengine/EffectEngine.h>

Hyperion* Hyperion::_hyperion = nullptr;

Hyperion* Hyperion::initInstance(const QJsonObject& qjsonConfig, const QString configFile) // REMOVE jsonConfig variable when the conversion from jsonCPP to QtJSON is finished
{
	if ( Hyperion::_hyperion != nullptr )
		throw std::runtime_error("Hyperion::initInstance can be called only one time");
	Hyperion::_hyperion = new Hyperion(qjsonConfig, configFile);

	return Hyperion::_hyperion;
}

Hyperion* Hyperion::getInstance()
{
	if ( Hyperion::_hyperion == nullptr )
		throw std::runtime_error("Hyperion::getInstance used without call of Hyperion::initInstance before");
		
	return Hyperion::_hyperion;
}

ColorOrder Hyperion::createColorOrder(const QJsonObject &deviceConfig)
{
	return stringToColorOrder(deviceConfig["colorOrder"].toString("rgb"));
}

ColorTransform * Hyperion::createColorTransform(const QJsonObject & transformConfig)
{
	const std::string id = transformConfig["id"].toString("default").toStdString();

	RgbChannelTransform * redTransform   = createRgbChannelTransform(transformConfig["red"].toObject());
	RgbChannelTransform * greenTransform = createRgbChannelTransform(transformConfig["green"].toObject());
	RgbChannelTransform * blueTransform  = createRgbChannelTransform(transformConfig["blue"].toObject());

	HsvTransform * hsvTransform = createHsvTransform(transformConfig["hsv"].toObject());
	HslTransform * hslTransform = createHslTransform(transformConfig["hsl"].toObject());

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


ColorAdjustment * Hyperion::createColorAdjustment(const QJsonObject & adjustmentConfig)
{
	const std::string id = adjustmentConfig["id"].toString("default").toStdString();
	
	// QT5.4 needed
	//~ RgbChannelAdjustment * blackAdjustment   = createRgbChannelAdjustment(adjustmentConfig["black"].  toArray(QJsonArray({"0","0","0"      })));
	//~ RgbChannelAdjustment * whiteAdjustment   = createRgbChannelAdjustment(adjustmentConfig["white"].  toArray(QJsonArray({"255","255","255"})));
	//~ RgbChannelAdjustment * redAdjustment     = createRgbChannelAdjustment(adjustmentConfig["red"].    toArray(QJsonArray({"255","0","0"    })));
	//~ RgbChannelAdjustment * greenAdjustment   = createRgbChannelAdjustment(adjustmentConfig["green"].  toArray(QJsonArray({"0","255","0"    })));
	//~ RgbChannelAdjustment * blueAdjustment    = createRgbChannelAdjustment(adjustmentConfig["blue"].   toArray(QJsonArray({"0","0","255"    })));
	//~ RgbChannelAdjustment * cyanAdjustment    = createRgbChannelAdjustment(adjustmentConfig["cyan"].   toArray(QJsonArray({"0","255","255"  })));
	//~ RgbChannelAdjustment * magentaAdjustment = createRgbChannelAdjustment(adjustmentConfig["magenta"].toArray(QJsonArray({"255","0","255"  })));
	//~ RgbChannelAdjustment * yellowAdjustment  = createRgbChannelAdjustment(adjustmentConfig["yellow"]. toArray(QJsonArray({"255","255","0"  })));
	
	RgbChannelAdjustment * blackAdjustment   = createRgbChannelAdjustment(adjustmentConfig["black"].toArray(),BLACK);
	RgbChannelAdjustment * whiteAdjustment   = createRgbChannelAdjustment(adjustmentConfig["white"].toArray(),WHITE);
	RgbChannelAdjustment * redAdjustment     = createRgbChannelAdjustment(adjustmentConfig["red"].toArray(),RED);
	RgbChannelAdjustment * greenAdjustment   = createRgbChannelAdjustment(adjustmentConfig["green"].toArray(),GREEN);
	RgbChannelAdjustment * blueAdjustment    = createRgbChannelAdjustment(adjustmentConfig["blue"].toArray(),BLUE);
	RgbChannelAdjustment * cyanAdjustment    = createRgbChannelAdjustment(adjustmentConfig["cyan"].toArray(),CYAN);
	RgbChannelAdjustment * magentaAdjustment = createRgbChannelAdjustment(adjustmentConfig["magenta"].toArray(),MAGENTA);
	RgbChannelAdjustment * yellowAdjustment  = createRgbChannelAdjustment(adjustmentConfig["yellow"].toArray(),YELLOW);
	
	ColorAdjustment * adjustment = new ColorAdjustment();
	adjustment->_id = id;
	adjustment->_rgbBlackAdjustment   = *blackAdjustment;
	adjustment->_rgbWhiteAdjustment   = *whiteAdjustment;
	adjustment->_rgbRedAdjustment     = *redAdjustment;
	adjustment->_rgbGreenAdjustment   = *greenAdjustment;
	adjustment->_rgbBlueAdjustment    = *blueAdjustment;
	adjustment->_rgbCyanAdjustment    = *cyanAdjustment;
	adjustment->_rgbMagentaAdjustment = *magentaAdjustment;
	adjustment->_rgbYellowAdjustment  = *yellowAdjustment;

	// Cleanup the allocated individual adjustments
	delete blackAdjustment;
	delete whiteAdjustment;
	delete redAdjustment;
	delete greenAdjustment;
	delete blueAdjustment;
	delete cyanAdjustment;
	delete magentaAdjustment;
	delete yellowAdjustment;
	
	return adjustment;
}


MultiColorTransform * Hyperion::createLedColorsTransform(const unsigned ledCnt, const QJsonObject & colorConfig)
{	
	// Create the result, the transforms are added to this
	MultiColorTransform * transform = new MultiColorTransform(ledCnt);
	Logger * log = Logger::getInstance("Core");
	
	const QJsonValue transformConfig = colorConfig["transform"];
	if (transformConfig.isNull())
	{
		// Old style color transformation config (just one for all leds)
		ColorTransform * colorTransform = createColorTransform(colorConfig);
		transform->addTransform(colorTransform);
		transform->setTransformForLed(colorTransform->_id, 0, ledCnt-1);
	}
	else if (transformConfig.isObject())
	{
		ColorTransform * colorTransform = createColorTransform(transformConfig.toObject());
		transform->addTransform(colorTransform);
		transform->setTransformForLed(colorTransform->_id, 0, ledCnt-1);
	}
	else if (transformConfig.isArray())
	{
		const QRegExp overallExp("([0-9]+(\\-[0-9]+)?)(,[ ]*([0-9]+(\\-[0-9]+)?))*");

		const QJsonArray & transformConfigArray = transformConfig.toArray();
		for (signed i = 0; i < transformConfigArray.size(); ++i)
		{
			const QJsonObject & config = transformConfigArray[i].toObject();
			ColorTransform * colorTransform = createColorTransform(config);
			transform->addTransform(colorTransform);

			const QString ledIndicesStr = config["leds"].toString("").trimmed();
			if (ledIndicesStr.compare("*") == 0)
			{
				// Special case for indices '*' => all leds
				transform->setTransformForLed(colorTransform->_id, 0, ledCnt-1);
 				Info(log, "ColorTransform '%s' => [0; %d]", colorTransform->_id.c_str(), ledCnt-1);
				continue;
			}

			if (!overallExp.exactMatch(ledIndicesStr))
			{
				Error(log, "Given led indices %d not correct format: %s", i, ledIndicesStr.toStdString().c_str());
				continue;
			}

			std::stringstream ss;
			const QStringList ledIndexList = ledIndicesStr.split(",");
			for (int i=0; i<ledIndexList.size(); ++i) {
				if (i > 0)
				{
					ss << ", ";
				}
				if (ledIndexList[i].contains("-"))
				{
					QStringList ledIndices = ledIndexList[i].split("-");
					int startInd = ledIndices[0].toInt();
					int endInd   = ledIndices[1].toInt();

					transform->setTransformForLed(colorTransform->_id, startInd, endInd);
					ss << startInd << "-" << endInd;
				}
				else
				{
					int index = ledIndexList[i].toInt();
					transform->setTransformForLed(colorTransform->_id, index, index);
					ss << index;
				}
			}
			Info(log, "ColorTransform '%s' => [%s]", colorTransform->_id.c_str(), ss.str().c_str()); 
		}
	}
	return transform;
}

MultiColorAdjustment * Hyperion::createLedColorsAdjustment(const unsigned ledCnt, const QJsonObject & colorConfig)
{
	// Create the result, the transforms are added to this
	MultiColorAdjustment * adjustment = new MultiColorAdjustment(ledCnt);
	Logger * log = Logger::getInstance("Core");

	const QJsonValue adjustmentConfig = colorConfig["channelAdjustment"];
	if (adjustmentConfig.isNull())
	{
		// Old style color transformation config (just one for all leds)
		ColorAdjustment * colorAdjustment = createColorAdjustment(colorConfig);
		adjustment->addAdjustment(colorAdjustment);
		adjustment->setAdjustmentForLed(colorAdjustment->_id, 0, ledCnt-1);
	}
	else if (adjustmentConfig.isObject())
	{
		ColorAdjustment * colorAdjustment = createColorAdjustment(adjustmentConfig.toObject());
		adjustment->addAdjustment(colorAdjustment);
		adjustment->setAdjustmentForLed(colorAdjustment->_id, 0, ledCnt-1);
	}
	else if (adjustmentConfig.isArray())
	{
		const QRegExp overallExp("([0-9]+(\\-[0-9]+)?)(,[ ]*([0-9]+(\\-[0-9]+)?))*");

		const QJsonArray & adjustmentConfigArray = adjustmentConfig.toArray();
		for (signed i = 0; i < adjustmentConfigArray.size(); ++i)
		{
			const QJsonObject & config = adjustmentConfigArray.at(i).toObject();
			ColorAdjustment * colorAdjustment = createColorAdjustment(config);
			adjustment->addAdjustment(colorAdjustment);

			const QString ledIndicesStr = config["leds"].toString("").trimmed();
			if (ledIndicesStr.compare("*") == 0)
			{
				// Special case for indices '*' => all leds
				adjustment->setAdjustmentForLed(colorAdjustment->_id, 0, ledCnt-1);
 				Info(log, "ColorAdjustment '%s' => [0; %d]", colorAdjustment->_id.c_str(), ledCnt-1);
				continue;
			}

			if (!overallExp.exactMatch(ledIndicesStr))
			{
				Error(log, "Given led indices %d not correct format: %s", i, ledIndicesStr.toStdString().c_str());
				continue;
			}

			std::stringstream ss;
			const QStringList ledIndexList = ledIndicesStr.split(",");
			for (int i=0; i<ledIndexList.size(); ++i) {
				if (i > 0)
				{
					ss << ", ";
				}
				if (ledIndexList[i].contains("-"))
				{
					QStringList ledIndices = ledIndexList[i].split("-");
					int startInd = ledIndices[0].toInt();
					int endInd   = ledIndices[1].toInt();

					adjustment->setAdjustmentForLed(colorAdjustment->_id, startInd, endInd);
					ss << startInd << "-" << endInd;
				}
				else
				{
					int index = ledIndexList[i].toInt();
					adjustment->setAdjustmentForLed(colorAdjustment->_id, index, index);
					ss << index;
				}
			}
			Info(log, "ColorAdjustment '%s' => [%s]", colorAdjustment->_id.c_str(), ss.str().c_str()); 
		}
	}
	return adjustment;
}

HsvTransform * Hyperion::createHsvTransform(const QJsonObject & hsvConfig)
{
	const double saturationGain = hsvConfig["saturationGain"].toDouble(1.0);
	const double valueGain      = hsvConfig["valueGain"].toDouble(1.0);

	return new HsvTransform(saturationGain, valueGain);
}

HslTransform * Hyperion::createHslTransform(const QJsonObject & hslConfig)
{
	const double saturationGain = hslConfig["saturationGain"].toDouble(1.0);
	const double luminanceGain  = hslConfig["luminanceGain"].toDouble(1.0);
	const double luminanceMinimum = hslConfig["luminanceMinimum"].toDouble(0.0);

	return new HslTransform(saturationGain, luminanceGain, luminanceMinimum);
}

RgbChannelTransform* Hyperion::createRgbChannelTransform(const QJsonObject& colorConfig)
{
	const double threshold  = colorConfig["threshold"].toDouble(0.0);
	const double gamma      = colorConfig["gamma"].toDouble(1.0);
	const double blacklevel = colorConfig["blacklevel"].toDouble(0.0);
	const double whitelevel = colorConfig["whitelevel"].toDouble(1.0);

	RgbChannelTransform* transform = new RgbChannelTransform(threshold, gamma, blacklevel, whitelevel);
	return transform;
}

RgbChannelAdjustment* Hyperion::createRgbChannelAdjustment(const QJsonArray& colorConfig, const RgbChannel color)
{
	int varR=0, varG=0, varB=0;
	if (color == BLACK) 
	{
		varR = colorConfig[0].toInt(0);
		varG = colorConfig[1].toInt(0);
		varB = colorConfig[2].toInt(0);
	}
	else if (color == WHITE)
	{
		varR = colorConfig[0].toInt(255);
		varG = colorConfig[1].toInt(255);
		varB = colorConfig[2].toInt(255);
	}		
	else if (color == RED) 
	{
		varR = colorConfig[0].toInt(255);
		varG = colorConfig[1].toInt(0);
		varB = colorConfig[2].toInt(0);
	}
	else if (color == GREEN)
	{
		varR = colorConfig[0].toInt(0);
		varG = colorConfig[1].toInt(255);
		varB = colorConfig[2].toInt(0);
	}		
	else if (color == BLUE)
	{
		varR = colorConfig[0].toInt(0);
		varG = colorConfig[1].toInt(0);
		varB = colorConfig[2].toInt(255);
	}
	else if (color == CYAN) 
	{
		varR = colorConfig[0].toInt(0);
		varG = colorConfig[1].toInt(255);
		varB = colorConfig[2].toInt(255);
	}
	else if (color == MAGENTA)
	{
		varR = colorConfig[0].toInt(255);
		varG = colorConfig[1].toInt(0);
		varB = colorConfig[2].toInt(255);
	}		
	else if (color == YELLOW)
	{
		varR = colorConfig[0].toInt(255);
		varG = colorConfig[1].toInt(255);
		varB = colorConfig[2].toInt(0);
	}
	
	RgbChannelAdjustment* adjustment = new RgbChannelAdjustment(varR, varG, varB);
	return adjustment;
}

LedString Hyperion::createLedString(const QJsonValue& ledsConfig, const ColorOrder deviceOrder)
{
	LedString ledString;
	const QString deviceOrderStr = colorOrderToString(deviceOrder);
	const QJsonArray & ledConfigArray = ledsConfig.toArray();
	int maxLedId = ledConfigArray.size();
	
	for (signed i = 0; i < ledConfigArray.size(); ++i)
	{
		const QJsonObject& index = ledConfigArray[i].toObject();

		Led led;
		led.index = index["index"].toInt();
		led.clone = index["clone"].toInt(-1);
		if ( led.clone < -1 || led.clone >= maxLedId )
		{
			Warning(Logger::getInstance("Core"), "LED %d: clone index of %d is out of range, clone ignored", led.index, led.clone);
			led.clone = -1;
		}

		if ( led.clone < 0 )
		{
			const QJsonObject& hscanConfig = ledConfigArray[i].toObject()["hscan"].toObject();
			const QJsonObject& vscanConfig = ledConfigArray[i].toObject()["vscan"].toObject();
			led.minX_frac = std::max(0.0, std::min(1.0, hscanConfig["minimum"].toDouble()));
			led.maxX_frac = std::max(0.0, std::min(1.0, hscanConfig["maximum"].toDouble()));
			led.minY_frac = std::max(0.0, std::min(1.0, vscanConfig["minimum"].toDouble()));
			led.maxY_frac = std::max(0.0, std::min(1.0, vscanConfig["maximum"].toDouble()));
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
			led.colorOrder = stringToColorOrder(index["colorOrder"].toString(deviceOrderStr));
			ledString.leds().push_back(led);
		}
	}

	// Make sure the leds are sorted (on their indices)
	std::sort(ledString.leds().begin(), ledString.leds().end(), [](const Led& lhs, const Led& rhs){ return lhs.index < rhs.index; });
	return ledString;
}

LedString Hyperion::createLedStringClone(const QJsonValue& ledsConfig, const ColorOrder deviceOrder)
{
	LedString ledString;
	const QString deviceOrderStr = colorOrderToString(deviceOrder);
	const QJsonArray & ledConfigArray = ledsConfig.toArray();
	int maxLedId = ledConfigArray.size();

	for (signed i = 0; i < ledConfigArray.size(); ++i)
	{
		const QJsonObject& index = ledConfigArray[i].toObject();

		Led led;
		led.index = index["index"].toInt();
		led.clone = index["clone"].toInt(-1);
		if ( led.clone < -1 || led.clone >= maxLedId )
		{
			Warning(Logger::getInstance("Core"), "LED %d: clone index of %d is out of range, clone ignored", led.index, led.clone);
			led.clone = -1;
		}

		if ( led.clone >= 0 )
		{
			Debug(Logger::getInstance("Core"), "LED %d: clone from led %d", led.index, led.clone);
			led.minX_frac = 0;
			led.maxX_frac = 0;
			led.minY_frac = 0;
			led.maxY_frac = 0;
			// Get the order of the rgb channels for this led (default is device order)
			led.colorOrder = stringToColorOrder(index["colorOrder"].toString(deviceOrderStr));

			ledString.leds().push_back(led);
		}

	}

	// Make sure the leds are sorted (on their indices)
	std::sort(ledString.leds().begin(), ledString.leds().end(), [](const Led& lhs, const Led& rhs){ return lhs.index < rhs.index; });
	return ledString;
}

QSize Hyperion::getLedLayoutGridSize(const QJsonValue& ledsConfig)
{
	std::vector<int> midPointsX;
	std::vector<int> midPointsY;
	const QJsonArray & ledConfigArray = ledsConfig.toArray();

	for (signed i = 0; i < ledConfigArray.size(); ++i)
	{
		const QJsonObject& index = ledConfigArray[i].toObject();

		if (index["clone"].toInt(-1) < 0 )
		{
			const QJsonObject& hscanConfig = ledConfigArray[i].toObject()["hscan"].toObject();
			const QJsonObject& vscanConfig = ledConfigArray[i].toObject()["vscan"].toObject();
			double minX_frac = std::max(0.0, std::min(1.0, hscanConfig["minimum"].toDouble()));
			double maxX_frac = std::max(0.0, std::min(1.0, hscanConfig["maximum"].toDouble()));
			double minY_frac = std::max(0.0, std::min(1.0, vscanConfig["minimum"].toDouble()));
			double maxY_frac = std::max(0.0, std::min(1.0, vscanConfig["maximum"].toDouble()));
			// Fix if the user swapped min and max
			if (minX_frac > maxX_frac)
			{
				std::swap(minX_frac, maxX_frac);
			}
			if (minY_frac > maxY_frac)
			{
				std::swap(minY_frac, maxY_frac);
			}

			// calculate mid point and make grid calculation
			midPointsX.push_back( int(1000.0*(minX_frac + maxX_frac) / 2.0) );
			midPointsY.push_back( int(1000.0*(minY_frac + maxY_frac) / 2.0) );
		}
	}

	// remove duplicates
	std::sort(midPointsX.begin(), midPointsX.end());
	midPointsX.erase(std::unique(midPointsX.begin(), midPointsX.end()), midPointsX.end());
	std::sort(midPointsY.begin(), midPointsY.end());
	midPointsY.erase(std::unique(midPointsY.begin(), midPointsY.end()), midPointsY.end());

	QSize gridSize( midPointsX.size(), midPointsY.size() );
	Debug(Logger::getInstance("Core"), "led layout grid: %dx%d", gridSize.width(), gridSize.height());

	return gridSize;
}



LinearColorSmoothing * Hyperion::createColorSmoothing(const QJsonObject & smoothingConfig, LedDevice* leddevice)
{
	Logger * log = Logger::getInstance("Core");
	std::string type = smoothingConfig["type"].toString("linear").toStdString();
	std::transform(type.begin(), type.end(), type.begin(), ::tolower);
	LinearColorSmoothing * device = nullptr;
	type = "linear"; // TODO currently hardcoded type, delete it if we have more types
	
	if (type == "linear")
	{
		Info( log, "Creating linear smoothing");
		device = new LinearColorSmoothing(
		            leddevice,
		            smoothingConfig["updateFrequency"].toDouble(25.0),
		            smoothingConfig["time_ms"].toInt(200),
		            smoothingConfig["updateDelay"].toInt(0),
		            smoothingConfig["continuousOutput"].toBool(true)
		            );
	}
	else
	{
		Error(log, "Smoothing disabled, because of unknown type '%s'.", type.c_str());
	}
	
	device->setEnable(smoothingConfig["enable"].toBool(true));
	InfoIf(!device->enabled(), log,"Smoothing disabled");

	assert(device != nullptr);
	return device;
}

MessageForwarder * Hyperion::createMessageForwarder(const QJsonObject & forwarderConfig)
{
		MessageForwarder * forwarder = new MessageForwarder();
		if ( !forwarderConfig.isEmpty() && forwarderConfig["enable"].toBool(true) )
		{
			if ( !forwarderConfig["json"].isNull() && forwarderConfig["json"].isArray() )
			{
				const QJsonArray & addr = forwarderConfig["json"].toArray();
				for (signed i = 0; i < addr.size(); ++i)
				{
					Info(Logger::getInstance("Core"), "Json forward to %s", addr.at(i).toString().toStdString().c_str());
					forwarder->addJsonSlave(addr[i].toString().toStdString());
				}
			}

			if ( !forwarderConfig["proto"].isNull() && forwarderConfig["proto"].isArray() )
			{
				const QJsonArray & addr = forwarderConfig["proto"].toArray();
				for (signed i = 0; i < addr.size(); ++i)
				{
					Info(Logger::getInstance("Core"), "Proto forward to %s", addr.at(i).toString().toStdString().c_str());
					forwarder->addProtoSlave(addr[i].toString().toStdString());
				}
			}
		}

	return forwarder;
}

MessageForwarder * Hyperion::getForwarder()
{
	return _messageForwarder;
}

Hyperion::Hyperion(const QJsonObject &qjsonConfig, const QString configFile)
	: _ledString(createLedString(qjsonConfig["leds"], createColorOrder(qjsonConfig["device"].toObject())))
	, _ledStringClone(createLedStringClone(qjsonConfig["leds"], createColorOrder(qjsonConfig["device"].toObject())))
	, _muxer(_ledString.leds().size())
	, _raw2ledTransform(createLedColorsTransform(_ledString.leds().size(), qjsonConfig["color"].toObject()))
	, _raw2ledAdjustment(createLedColorsAdjustment(_ledString.leds().size(), qjsonConfig["color"].toObject()))
	, _effectEngine(nullptr)
	, _messageForwarder(createMessageForwarder(qjsonConfig["forwarder"].toObject()))
	, _qjsonConfig(qjsonConfig)
	, _configFile(configFile)
	, _timer()
	, _log(Logger::getInstance("Core"))
	, _hwLedCount(_ledString.leds().size())
	, _colorAdjustmentV4Lonly(false)
	, _colorTransformV4Lonly(false)
	, _sourceAutoSelectEnabled(true)
	, _configHash()
	, _ledGridSize(getLedLayoutGridSize(qjsonConfig["leds"]))
{
	registerPriority("Off", PriorityMuxer::LOWEST_PRIORITY);
	
	if (!_raw2ledAdjustment->verifyAdjustments())
	{
		throw std::runtime_error("Color adjustment incorrectly set");
	}
	if (!_raw2ledTransform->verifyTransforms())
	{
		throw std::runtime_error("Color transformation incorrectly set");
	}
	// set color correction activity state
	const QJsonObject& color = qjsonConfig["color"].toObject();
	_transformEnabled   = color["transform_enable"].toBool(true);
	_adjustmentEnabled  = color["channelAdjustment_enable"].toBool(true);

	_colorTransformV4Lonly  = color["transform_v4l_only"].toBool(false);
	_colorAdjustmentV4Lonly = color["channelAdjustment_v4l_only"].toBool(false);

	InfoIf(!_transformEnabled  , _log, "Color transformation disabled" );
	InfoIf(!_adjustmentEnabled , _log, "Color adjustment disabled" );
	
	InfoIf(_colorTransformV4Lonly  , _log, "Color transformation for v4l inputs only" );
	InfoIf(_colorAdjustmentV4Lonly , _log, "Color adjustment for v4l inputs only" );
	
	// initialize the image processor factory
	_ledMAppingType = ImageProcessor::mappingTypeToInt(color["imageToLedMappingType"].toString());
	ImageProcessorFactory::getInstance().init(_ledString, qjsonConfig["blackborderdetector"].toObject(),_ledMAppingType );
	
	getComponentRegister().componentStateChanged(hyperion::COMP_FORWARDER, _messageForwarder->forwardingEnabled());

	// initialize leddevices
	_device       = LedDeviceFactory::construct(qjsonConfig["device"].toObject(),_hwLedCount);
	_deviceSmooth = createColorSmoothing(qjsonConfig["smoothing"].toObject(), _device);
	getComponentRegister().componentStateChanged(hyperion::COMP_SMOOTHING, _deviceSmooth->componentState());

	// setup the timer
	_timer.setSingleShot(true);
	QObject::connect(&_timer, SIGNAL(timeout()), this, SLOT(update()));

	// create the effect engine
	_effectEngine = new EffectEngine(this,qjsonConfig["effects"].toObject());
	
	const QJsonObject& device = qjsonConfig["device"].toObject();
	unsigned int hwLedCount = device["ledCount"].toInt(getLedCount());
	_hwLedCount = std::max(hwLedCount, getLedCount());
	Debug(_log,"configured leds: %d hw leds: %d", getLedCount(), _hwLedCount);
	WarningIf(hwLedCount < getLedCount(), _log, "more leds configured than available. check 'ledCount' in 'device' section");

	WarningIf(!configWriteable(), _log, "Your config is not writeable - you won't be able to use the web ui for configuration.");
	// initialize hash of current config
	configModified();

	const QJsonObject & generalConfig = qjsonConfig["general"].toObject();
	_configVersionId = generalConfig["configVersion"].toInt(-1);

	// initialize the leds
	update();
}


void Hyperion::freeObjects()
{
	// switch off all leds
	clearall();
	_device->switchOff();

	// delete components on exit of hyperion core
	delete _effectEngine;
	delete _device;
	delete _raw2ledTransform;
	delete _raw2ledAdjustment;
	delete _messageForwarder;
}

Hyperion::~Hyperion()
{
	freeObjects();
}

unsigned Hyperion::getLedCount() const
{
	return _ledString.leds().size();
}


bool Hyperion::configModified()
{
	bool isModified = false;
	QFile f(_configFile);
	if (f.open(QFile::ReadOnly))
	{
		QCryptographicHash hash(QCryptographicHash::Sha1);
		if (hash.addData(&f))
		{
			if (_configHash.size() == 0)
			{
				_configHash = hash.result();
			}
			else
			{
				isModified = _configHash != hash.result();
			}
		}
	}
	f.close();

	return isModified;
}

bool Hyperion::configWriteable()
{
	QFile file(_configFile);
	QFileInfo fileInfo(file);
	return fileInfo.isWritable() && fileInfo.isReadable();
}


void Hyperion::registerPriority(const std::string name, const int priority)
{
	Info(_log, "Register new input source named '%s' for priority channel '%d'", name.c_str(), priority );
	
	for(auto const &entry : _priorityRegister)
	{
		WarningIf( ( entry.first != name && entry.second == priority), _log,
		           "Input source '%s' uses same priority channel (%d) as '%s'.", name.c_str(), priority, entry.first.c_str());
	}

	_priorityRegister.emplace(name,priority);
}

void Hyperion::unRegisterPriority(const std::string name)
{
	Info(_log, "Unregister input source named '%s' from priority register", name.c_str());
	_priorityRegister.erase(name);
}

void Hyperion::setSourceAutoSelectEnabled(bool enabled)
{
	_sourceAutoSelectEnabled = enabled;
	if (! _sourceAutoSelectEnabled)
	{
		setCurrentSourcePriority(_muxer.getCurrentPriority());
	}
	DebugIf( !_sourceAutoSelectEnabled, _log, "source auto select is disabled");
	InfoIf(_sourceAutoSelectEnabled, _log, "set current input source to auto select");
}

bool Hyperion::setCurrentSourcePriority(int priority )
{
	bool priorityValid = _muxer.hasPriority(priority);
	if (priorityValid)
	{
		DebugIf(_sourceAutoSelectEnabled, _log, "source auto select is disabled");
		_sourceAutoSelectEnabled = false;
		_currentSourcePriority = priority;
		Info(_log, "set current input source to priority channel %d", _currentSourcePriority);
	}

	return priorityValid;
}

void Hyperion::setComponentState(const hyperion::Components component, const bool state)
{
	if (component == hyperion::COMP_SMOOTHING)
	{
		_deviceSmooth->setEnable(state);
		getComponentRegister().componentStateChanged(hyperion::COMP_SMOOTHING, _deviceSmooth->componentState());
	}
	else
	{
		emit componentStateChanged(component, state);
	}
}

void Hyperion::setColor(int priority, const ColorRgb &color, const int timeout_ms, bool clearEffects)
{
	// create led output
	std::vector<ColorRgb> ledColors(_ledString.leds().size(), color);

	// set colors
	setColors(priority, ledColors, timeout_ms, clearEffects, hyperion::COMP_COLOR);
}

void Hyperion::setColors(int priority, const std::vector<ColorRgb>& ledColors, const int timeout_ms, bool clearEffects, hyperion::Components component)
{
	// clear effects if this call does not come from an effect
	if (clearEffects)
	{
		_effectEngine->channelCleared(priority);
	}

	if (timeout_ms > 0)
	{
		const uint64_t timeoutTime = QDateTime::currentMSecsSinceEpoch() + timeout_ms;
		_muxer.setInput(priority, ledColors, timeoutTime, component);
	}
	else
	{
		_muxer.setInput(priority, ledColors, -1, component);
	}

	if (! _sourceAutoSelectEnabled || priority == _muxer.getCurrentPriority())
	{
		update();
	}
}

void Hyperion::setImage(int priority, const Image<ColorRgb> & image, int duration_ms)
{
	if (priority == getCurrentPriority())
	{
		emit emitImage(priority, image, duration_ms);
	}
}

const std::vector<std::string> & Hyperion::getTransformIds() const
{
	return _raw2ledTransform->getTransformIds();
}

const std::vector<std::string> & Hyperion::getAdjustmentIds() const
{
	return _raw2ledAdjustment->getAdjustmentIds();
}

ColorTransform * Hyperion::getTransform(const std::string& id)
{
	return _raw2ledTransform->getTransform(id);
}

ColorAdjustment * Hyperion::getAdjustment(const std::string& id)
{
	return _raw2ledAdjustment->getAdjustment(id);
}

void Hyperion::transformsUpdated()
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

int Hyperion::getCurrentPriority() const
{
	
	return _sourceAutoSelectEnabled || !_muxer.hasPriority(_currentSourcePriority) ? _muxer.getCurrentPriority() : _currentSourcePriority;
}

QList<int> Hyperion::getActivePriorities() const
{
	return _muxer.getPriorities();
}

const Hyperion::InputInfo &Hyperion::getPriorityInfo(const int priority) const
{
	return _muxer.getInputInfo(priority);
}

void Hyperion::reloadEffects()
{
	_effectEngine->readEffects();
}

const std::list<EffectDefinition> & Hyperion::getEffects() const
{
	return _effectEngine->getEffects();
}

const std::list<ActiveEffectDefinition> & Hyperion::getActiveEffects()
{
	return _effectEngine->getActiveEffects();
}

const std::list<EffectSchema> & Hyperion::getEffectSchemas()
{
	return _effectEngine->getEffectSchemas();
}

int Hyperion::setEffect(const QString &effectName, int priority, int timeout)
{
	return _effectEngine->runEffect(effectName, priority, timeout);
}

int Hyperion::setEffect(const QString &effectName, const QJsonObject &args, int priority, int timeout, QString pythonScript)
{
	return _effectEngine->runEffect(effectName, args, priority, timeout, pythonScript);
}

void Hyperion::setLedMappingType(int mappingType)
{
	_ledMAppingType = mappingType;
	emit imageToLedsMappingChanged(mappingType);
}

void Hyperion::update()
{
	// Update the muxer, cleaning obsolete priorities
	_muxer.setCurrentTime(QDateTime::currentMSecsSinceEpoch());

	// Obtain the current priority channel
	int priority = _sourceAutoSelectEnabled || !_muxer.hasPriority(_currentSourcePriority) ? _muxer.getCurrentPriority() : _currentSourcePriority;
	const PriorityMuxer::InputInfo & priorityInfo  =  _muxer.getInputInfo(priority);

	// copy ledcolors to local buffer
	_ledBuffer.reserve(_hwLedCount);
	_ledBuffer = priorityInfo.ledColors;

	if ( _transformEnabled && (!_colorTransformV4Lonly || priorityInfo.componentId == hyperion::COMP_V4L) )
	{
		_raw2ledTransform->applyTransform(_ledBuffer);
	}
	if ( _adjustmentEnabled && (!_colorAdjustmentV4Lonly || priorityInfo.componentId == hyperion::COMP_V4L) )
	{
		_raw2ledAdjustment->applyAdjustment(_ledBuffer);
	}

	// init colororder vector, if empty
	if (_ledStringColorOrder.empty())
	{
		for (Led& led : _ledString.leds())
		{
			_ledStringColorOrder.push_back(led.colorOrder);
		}
		for (Led& led : _ledStringClone.leds())
		{
			_ledStringColorOrder.insert(_ledStringColorOrder.begin() + led.index, led.colorOrder);
		}
	}
	
	// insert cloned leds into buffer
	for (Led& led : _ledStringClone.leds())
	{
		_ledBuffer.insert(_ledBuffer.begin() + led.index, _ledBuffer.at(led.clone));
	}
	
	int i = 0;
	for (ColorRgb& color : _ledBuffer)
	{
		//const ColorOrder ledColorOrder = leds.at(i).colorOrder;

		// correct the color byte order
		switch (_ledStringColorOrder.at(i))
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
			std::swap(color.red, color.green);
			std::swap(color.green, color.blue);
			break;
		}
		case ORDER_BRG:
		{
			std::swap(color.red, color.blue);
			std::swap(color.green, color.blue);
			break;
		}
		}
		i++;
	}

	if ( _hwLedCount > _ledBuffer.size() )
	{
		_ledBuffer.resize(_hwLedCount, ColorRgb::BLACK);
	}
	
	// Write the data to the device
	if (_deviceSmooth->enabled())
		_deviceSmooth->setLedValues(_ledBuffer);
	else
		_device->setLedValues(_ledBuffer);

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
