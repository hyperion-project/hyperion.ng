
#pragma once

// STL includes
#include <stdint.h>
#include <iostream>

// Forward class declaration
struct RgbColor;

struct RgbColor
{
	uint8_t red;
	uint8_t green;
	uint8_t blue;

	static RgbColor BLACK;
	static RgbColor RED;
	static RgbColor GREEN;
	static RgbColor BLUE;
	static RgbColor YELLOW;
	static RgbColor WHITE;

	inline bool operator==(const RgbColor& other) const
	{
		return red == other.red && green == other.green && blue == other.blue;
	}
};

static_assert(sizeof(RgbColor) == 3, "Incorrect size of RgbColor");


inline std::ostream& operator<<(std::ostream& os, const RgbColor& color)
{
	os << "{" << unsigned(color.red) << "," << unsigned(color.green) << "," << unsigned(color.blue) << "}";
	return os;
}

