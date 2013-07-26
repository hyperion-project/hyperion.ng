// STL includes
#include <cstring>
#include <unistd.h>
#include <iostream>

// Json includes
#include <json/json.h>

// hyperion includes
#include <hyperion/LedString.h>

LedString LedString::construct(const Json::Value& ledsConfig, const Json::Value& colorConfig)
{
	LedString ledString;

	const Json::Value& redConfig   = colorConfig["red"];
	const Json::Value& greenConfig = colorConfig["greem"];
	const Json::Value& blueConfig  = colorConfig["blue"];

	ledString.red.gamma      = redConfig["gamma"].asDouble();
	ledString.red.adjust     = redConfig["adjust"].asDouble();
	ledString.red.blacklevel = redConfig["blacklevel"].asDouble();

	ledString.green.gamma      = greenConfig["gamma"].asDouble();
	ledString.green.adjust     = colorConfig["adjust"].asDouble();
	ledString.green.blacklevel = colorConfig["blacklevel"].asDouble();

	ledString.blue.gamma      = blueConfig["gamma"].asDouble();
	ledString.blue.adjust     = blueConfig["adjust"].asDouble();
	ledString.blue.blacklevel = blueConfig["blacklevel"].asDouble();

	for (const Json::Value& ledConfig : ledsConfig)
	{
		Led led;
		led.index = ledConfig["index"].asInt();
		const Json::Value& hscanConfig = ledConfig["hscan"];
		const Json::Value& vscanConfig = ledConfig["vscan"];
		led.minX_frac = std::max(0.0, std::min(100.0, hscanConfig["minimum"].asDouble()))/100.0;
		led.maxX_frac = std::max(0.0, std::min(100.0, hscanConfig["maximum"].asDouble()))/100.0;
		led.minY_frac = 1.0 - std::max(0.0, std::min(100.0, vscanConfig["maximum"].asDouble()))/100.0;
		led.maxY_frac = 1.0 - std::max(0.0, std::min(100.0, vscanConfig["minimum"].asDouble()))/100.0;

		ledString.mLeds.push_back(led);
	}
	return ledString;
}

LedString::LedString()
{
	// empty
}

LedString::~LedString()
{
}

const std::vector<Led>& LedString::leds() const
{
	return mLeds;
}
