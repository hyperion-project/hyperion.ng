#pragma once

// STL includes
#include <cstdint>
#include <iostream>

struct ColorRgb;

///
/// Plain-Old-Data structure containing the red-green-blue color specification. Size of the
/// structure is exactly 3-bytes for easy writing to led-device
///
struct ColorRgb
{
	/// The red color channel
	uint8_t red;
	/// The green color channel
	uint8_t green;
	/// The blue color channel
	uint8_t blue;

	/// 'Black' RgbColor (0, 0, 0)
	static ColorRgb BLACK;
	/// 'Red' RgbColor (255, 0, 0)
	static ColorRgb RED;
	/// 'Green' RgbColor (0, 255, 0)
	static ColorRgb GREEN;
	/// 'Blue' RgbColor (0, 0, 255)
	static ColorRgb BLUE;
	/// 'Yellow' RgbColor (255, 255, 0)
	static ColorRgb YELLOW;
	/// 'White' RgbColor (255, 255, 255)
	static ColorRgb WHITE;
};

/// Assert to ensure that the size of the structure is 'only' 3 bytes
static_assert(sizeof(ColorRgb) == 3, "Incorrect size of ColorRgb");

///
/// Stream operator to write ColorRgb to an outputstream (format "'{'[red]','[green]','[blue]'}'")
///
/// @param os The output stream
/// @param color The color to write
/// @return The output stream (with the color written to it)
///
inline std::ostream& operator<<(std::ostream& os, const ColorRgb& color)
{
	os << "{" << unsigned(color.red) << "," << unsigned(color.green) << "," << unsigned(color.blue) << "}";
	return os;
}

/// Compare operator to check if a color is 'smaller' than another color
inline bool operator<(const ColorRgb & lhs, const ColorRgb & rhs)
{
	return (lhs.red < rhs.red) && (lhs.green < rhs.green) && (lhs.blue < rhs.blue);
}

/// Compare operator to check if a color is 'smaller' than or 'equal' to another color
inline bool operator<=(const ColorRgb & lhs, const ColorRgb & rhs)
{
	return (lhs.red <= rhs.red) && (lhs.green <= rhs.green) && (lhs.blue <= rhs.blue);
}

/// Compare operator to check if a color is 'greater' to another color
inline bool operator>(const ColorRgb & lhs, const ColorRgb & rhs)
{
	return (lhs.red > rhs.red) && (lhs.green > rhs.green) && (lhs.blue > rhs.blue);
}

/// Compare operator to check if a color is 'greater' than or 'equal' to another color
inline bool operator>=(const ColorRgb & lhs, const ColorRgb & rhs)
{
	return (lhs.red >= rhs.red) && (lhs.green >= rhs.green) && (lhs.blue >= rhs.blue);
}

