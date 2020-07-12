#pragma once

// STL includes
#include <cstdint>
#include <iostream>

struct ColorBgr;

///
/// Plain-Old-Data structure containing the red-green-blue color specification. Size of the
/// structure is exactly 3-bytes for easy writing to led-device
///
struct ColorBgr
{
	/// The blue color channel
	uint8_t blue;
	/// The green color channel
	uint8_t green;
	/// The red color channel
	uint8_t red;

	/// 'Black' RgbColor (0, 0, 0)
	static ColorBgr BLACK;
	/// 'Red' RgbColor (255, 0, 0)
	static ColorBgr RED;
	/// 'Green' RgbColor (0, 255, 0)
	static ColorBgr GREEN;
	/// 'Blue' RgbColor (0, 0, 255)
	static ColorBgr BLUE;
	/// 'Yellow' RgbColor (255, 255, 0)
	static ColorBgr YELLOW;
	/// 'White' RgbColor (255, 255, 255)
	static ColorBgr WHITE;
};

/// Assert to ensure that the size of the structure is 'only' 3 bytes
static_assert(sizeof(ColorBgr) == 3, "Incorrect size of ColorBgr");

///
/// Stream operator to write ColorRgb to an outputstream (format "'{'[red]','[green]','[blue]'}'")
///
/// @param os The output stream
/// @param color The color to write
/// @return The output stream (with the color written to it)
///
inline std::ostream& operator<<(std::ostream& os, const ColorBgr& color)
{
	os << "{" << unsigned(color.red) << "," << unsigned(color.green) << "," << unsigned(color.blue) << "}";
	return os;
}


/// Compare operator to check if a color is 'smaller' than another color
inline bool operator<(const ColorBgr & lhs, const ColorBgr & rhs)
{
	return (lhs.red < rhs.red) && (lhs.green < rhs.green) && (lhs.blue < rhs.blue);
}

/// Compare operator to check if a color is 'smaller' than or 'equal' to another color
inline bool operator<=(const ColorBgr & lhs, const ColorBgr & rhs)
{
	return (lhs.red <= rhs.red) && (lhs.green <= rhs.green) && (lhs.blue <= rhs.blue);
}
