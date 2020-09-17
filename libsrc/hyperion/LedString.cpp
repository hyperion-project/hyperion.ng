// STL includes
#include <cstring>
#include <iostream>

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
