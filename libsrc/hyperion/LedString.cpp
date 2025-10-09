// STL includes

#include <cstring>
#include <iostream>

// hyperion includes
#include <hyperion/LedString.h>
#include <utils/Logger.h>

// QT includes
#include <QJsonObject>

std::vector<Led>& LedString::leds()
{
	return _leds;
}

const std::vector<Led>& LedString::leds() const
{
	return _leds;
}

std::vector<int>& LedString::blacklistedLedIds()
{
	return _blacklistedLedIds;
}

const std::vector<int>& LedString::blacklistedLedIds() const
{
	return _blacklistedLedIds;
}

bool LedString::hasBlackListedLeds()
{

	if (_blacklistedLedIds.size() > 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Construct the 'led-string' with the integration area definition per led and the color
 * ordering of the RGB channels
 * @param ledsConfig   The configuration of the led areas
 * @param deviceOrder  The default RGB channel ordering
 * @param ledsUsed     The maximum number of LEDs used from the configuration
 * @return The constructed ledstring
 */
LedString LedString::createLedString(const QJsonArray& ledConfigArray, const ColorOrder deviceOrder, int maxLeds)
{
	LedString ledString;
	const QString deviceOrderStr = colorOrderToString(deviceOrder);

	int configuredLeds = static_cast<int>(ledConfigArray.size());
	int ledsUsed = configuredLeds;

	if (ledsUsed > maxLeds)
	{
		ledsUsed = maxLeds;
		Warning(Logger::getInstance("HYPERION"),"More LEDs configured [%d] via the layout than supported by the device [%d]. Using first [%d] layout entries", configuredLeds, maxLeds, ledsUsed);
	}

	for (signed i = 0; i < ledsUsed; ++i)
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

		led.isBlacklisted = false;
		if (led.minX_frac < std::numeric_limits<double>::epsilon() &&
			led.maxX_frac < std::numeric_limits<double>::epsilon() &&
			led.minY_frac < std::numeric_limits<double>::epsilon() &&
			led.maxY_frac < std::numeric_limits<double>::epsilon()
			)
		{
			led.isBlacklisted = true;
			ledString.blacklistedLedIds().push_back(i);
		}
		ledString.leds().push_back(led);
	}
	return ledString;
}
