// STL includes
#include <cstring>
#include <unistd.h>
#include <iostream>

// Json includes
#include <json/json.h>

// hyperion includes
#include <hyperion/LedString.h>


LedString::LedString()
{
	// empty
}

LedString::~LedString()
{
	// empty
}

std::vector<Led>& LedString::leds()
{
	return mLeds;
}

const std::vector<Led>& LedString::leds() const
{
	return mLeds;
}
