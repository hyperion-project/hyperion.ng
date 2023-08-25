// STL includes

#include <cstring>
#include <iostream>

// hyperion includes
#include <hyperion/LedString.h>

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
 * @return The constructed ledstring
 */
LedString LedString::createLedString(const QJsonArray& ledConfigArray, const ColorOrder deviceOrder)
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
