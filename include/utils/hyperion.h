#pragma once

#include <sstream>

#include <hyperion/ColorAdjustment.h>
#include <hyperion/MultiColorAdjustment.h>
#include <hyperion/LedString.h>
// fg effect
#include <hyperion/Hyperion.h>

///
/// @brief Provide utility methods for Hyperion class
///
namespace hyperion {

	void handleInitialEffect(Hyperion* hyperion, const QJsonObject& FGEffectConfig)
	{
		#define FGCONFIG_ARRAY fgColorConfig.toArray()
		const int FG_PRIORITY = 0;
		const int DURATION_INFINITY = 0;

		// initial foreground effect/color
		if (FGEffectConfig["enable"].toBool(true))
		{
			const QString fgTypeConfig = FGEffectConfig["type"].toString("effect");
			const QString fgEffectConfig = FGEffectConfig["effect"].toString("Rainbow swirl fast");
			const QJsonValue fgColorConfig = FGEffectConfig["color"];
			int default_fg_duration_ms = 3000;
			int fg_duration_ms = FGEffectConfig["duration_ms"].toInt(default_fg_duration_ms);
			if (fg_duration_ms == DURATION_INFINITY)
			{
				fg_duration_ms = default_fg_duration_ms;
				Warning(Logger::getInstance("HYPERION"), "foreground effect duration 'infinity' is forbidden, set to default value %d ms",default_fg_duration_ms);
			}
			if ( fgTypeConfig.contains("color") )
			{
				std::vector<ColorRgb> fg_color = {
					ColorRgb {
						(uint8_t)FGCONFIG_ARRAY.at(0).toInt(0),
						(uint8_t)FGCONFIG_ARRAY.at(1).toInt(0),
						(uint8_t)FGCONFIG_ARRAY.at(2).toInt(0)
					}
				};
				hyperion->setColor(FG_PRIORITY, fg_color, fg_duration_ms);
				Info(Logger::getInstance("HYPERION"),"Initial foreground color set (%d %d %d)",fg_color.at(0).red,fg_color.at(0).green,fg_color.at(0).blue);
			}
			else
			{
				int result = hyperion->setEffect(fgEffectConfig, FG_PRIORITY, fg_duration_ms);
				Info(Logger::getInstance("HYPERION"),"Initial foreground effect '%s' %s", QSTRING_CSTR(fgEffectConfig), ((result == 0) ? "started" : "failed"));
			}
		}
		#undef FGCONFIG_ARRAY
	}

	ColorOrder createColorOrder(const QJsonObject &deviceConfig)
	{
		return stringToColorOrder(deviceConfig["colorOrder"].toString("rgb"));
	}

	RgbTransform createRgbTransform(const QJsonObject& colorConfig)
	{
		const double backlightThreshold = colorConfig["backlightThreshold"].toDouble(0.0);
		const bool   backlightColored   = colorConfig["backlightColored"].toBool(false);
		const double brightness         = colorConfig["brightness"].toInt(100);
		const double brightnessComp     = colorConfig["brightnessCompensation"].toInt(100);
		const double gammaR             = colorConfig["gammaRed"].toDouble(1.0);
		const double gammaG             = colorConfig["gammaGreen"].toDouble(1.0);
		const double gammaB             = colorConfig["gammaBlue"].toDouble(1.0);

		return RgbTransform(gammaR, gammaG, gammaB, backlightThreshold, backlightColored, brightness, brightnessComp);
	}

	RgbChannelAdjustment createRgbChannelAdjustment(const QJsonObject& colorConfig, const QString& channelName, int defaultR, int defaultG, int defaultB)
	{
		const QJsonArray& channelConfig  = colorConfig[channelName].toArray();
		return RgbChannelAdjustment(
			channelConfig[0].toInt(defaultR),
			channelConfig[1].toInt(defaultG),
			channelConfig[2].toInt(defaultB),
			"ChannelAdjust_" + channelName.toUpper()
		);
	}

	ColorAdjustment* createColorAdjustment(const QJsonObject & adjustmentConfig)
	{
		const QString id = adjustmentConfig["id"].toString("default");

		ColorAdjustment * adjustment = new ColorAdjustment();
		adjustment->_id = id;
		adjustment->_rgbBlackAdjustment   = createRgbChannelAdjustment(adjustmentConfig, "black"  ,   0,  0,  0);
		adjustment->_rgbWhiteAdjustment   = createRgbChannelAdjustment(adjustmentConfig, "white"  , 255,255,255);
		adjustment->_rgbRedAdjustment     = createRgbChannelAdjustment(adjustmentConfig, "red"    , 255,  0,  0);
		adjustment->_rgbGreenAdjustment   = createRgbChannelAdjustment(adjustmentConfig, "green"  ,   0,255,  0);
		adjustment->_rgbBlueAdjustment    = createRgbChannelAdjustment(adjustmentConfig, "blue"   ,   0,  0,255);
		adjustment->_rgbCyanAdjustment    = createRgbChannelAdjustment(adjustmentConfig, "cyan"   ,   0,255,255);
		adjustment->_rgbMagentaAdjustment = createRgbChannelAdjustment(adjustmentConfig, "magenta", 255,  0,255);
		adjustment->_rgbYellowAdjustment  = createRgbChannelAdjustment(adjustmentConfig, "yellow" , 255,255,  0);
		adjustment->_rgbTransform         = createRgbTransform(adjustmentConfig);

		return adjustment;
	}

	MultiColorAdjustment * createLedColorsAdjustment(unsigned ledCnt, const QJsonObject & colorConfig)
	{
		// Create the result, the transforms are added to this
		MultiColorAdjustment * adjustment = new MultiColorAdjustment(ledCnt);

		const QJsonValue adjustmentConfig = colorConfig["channelAdjustment"];
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
				//Info(Logger::getInstance("HYPERION"), "ColorAdjustment '%s' => [0-%d]", QSTRING_CSTR(colorAdjustment->_id), ledCnt-1);
				continue;
			}

			if (!overallExp.exactMatch(ledIndicesStr))
			{
				//Error(Logger::getInstance("HYPERION"), "Given led indices %d not correct format: %s", i, QSTRING_CSTR(ledIndicesStr));
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
			//Info(Logger::getInstance("HYPERION"), "ColorAdjustment '%s' => [%s]", QSTRING_CSTR(colorAdjustment->_id), ss.str().c_str());
		}

		return adjustment;
	}

	/**
	 * Construct the 'led-string' with the integration area definition per led and the color
	 * ordering of the RGB channels
	 * @param ledsConfig   The configuration of the led areas
	 * @param deviceOrder  The default RGB channel ordering
	 * @return The constructed ledstring
	 */
	LedString createLedString(const QJsonArray& ledConfigArray, const ColorOrder deviceOrder)
	{
		LedString ledString;
		const QString deviceOrderStr = colorOrderToString(deviceOrder);

		for (signed i = 0; i < ledConfigArray.size(); ++i)
		{
			const QJsonObject& ledConfig = ledConfigArray[i].toObject();
			Led led;

			led.minX_frac = qMax(0.0, qMin(1.0, ledConfig["hmin"].toDouble()));
			led.maxX_frac = qMax(0.0, qMin(1.0, ledConfig["hmax"].toDouble()));
			led.minY_frac = qMax(0.0, qMin(1.0, ledConfig["vmin"].toDouble()));
			led.maxY_frac = qMax(0.0, qMin(1.0, ledConfig["vmax"].toDouble()));
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
			led.colorOrder = stringToColorOrder(ledConfig["colorOrder"].toString(deviceOrderStr));
			ledString.leds().push_back(led);
		}
		return ledString;
	}

	QSize getLedLayoutGridSize(const QJsonArray& ledConfigArray)
	{
		std::vector<int> midPointsX;
		std::vector<int> midPointsY;

		for (signed i = 0; i < ledConfigArray.size(); ++i)
		{
			const QJsonObject& ledConfig = ledConfigArray[i].toObject();

			double minX_frac = qMax(0.0, qMin(1.0, ledConfig["hmin"].toDouble()));
			double maxX_frac = qMax(0.0, qMin(1.0, ledConfig["hmax"].toDouble()));
			double minY_frac = qMax(0.0, qMin(1.0, ledConfig["vmin"].toDouble()));
			double maxY_frac = qMax(0.0, qMin(1.0, ledConfig["vmax"].toDouble()));
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

		// remove duplicates
		std::sort(midPointsX.begin(), midPointsX.end());
		midPointsX.erase(std::unique(midPointsX.begin(), midPointsX.end()), midPointsX.end());
		std::sort(midPointsY.begin(), midPointsY.end());
		midPointsY.erase(std::unique(midPointsY.begin(), midPointsY.end()), midPointsY.end());

		QSize gridSize( midPointsX.size(), midPointsY.size() );
		//Debug(_log, "LED layout grid size: %dx%d", gridSize.width(), gridSize.height());

		// Limit to 80px for performance reasons
		const int pl = 80;
		if(gridSize.width() > pl || gridSize.height() > pl)
			gridSize.scale(pl, pl, Qt::KeepAspectRatio);

		// Correct the grid in case it is malformed in width vs height
		// Expected is at least 50% of width <-> height
		if((gridSize.width() / gridSize.height()) > 2)
			gridSize.setHeight(qMax(1,gridSize.width()/2));
		else if((gridSize.width() / gridSize.height()) < 0.5)
			gridSize.setWidth(qMax(1,gridSize.height()/2));

		return gridSize;
	}
};
