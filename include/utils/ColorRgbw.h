#pragma once

// STL includes
#include <cstdint>
#include <iostream>

///
/// Plain-Old-Data structure containing the red-green-blue color specification. Size of the
/// structure is exactly 3-bytes for easy writing to led-device
///
struct ColorRgbw
{
	/// The red color channel
	uint8_t red;
	/// The green color channel
	uint8_t green;
	/// The blue color channel
	uint8_t blue;
	/// The white color channel
	uint8_t white;

	/// 'Black' RgbColor (0, 0, 0, 0)
	static const ColorRgbw BLACK;
	/// 'Red' RgbColor (255, 0, 0, 0)
	static const ColorRgbw RED;
	/// 'Green' RgbColor (0, 255, 0, 0)
	static const ColorRgbw GREEN;
	/// 'Blue' RgbColor (0, 0, 255, 0)
	static const ColorRgbw BLUE;
	/// 'Yellow' RgbColor (255, 255, 0, 0)
	static const ColorRgbw YELLOW;
	/// 'White' RgbColor (0, 0, 0, 255)
	static const ColorRgbw WHITE;
};

/// Assert to ensure that the size of the structure is 'only' 4 bytes
static_assert(sizeof(ColorRgbw) == 4, "Incorrect size of ColorRgbw");

///
/// Stream operator to write ColorRgb to an outputstream (format "'{'[red]','[green]','[blue]'}'")
///
/// @param os The output stream
/// @param color The color to write
/// @return The output stream (with the color written to it)
///
inline std::ostream& operator<<(std::ostream& os, const ColorRgbw& color)
{
	os << "{"
		<< color.red   << ","
		<< color.green << ","
		<< color.blue  << ","
		<< color.white <<
	"}";

	return os;
}

/// Compare operator to check if a color is 'equal' than another color
inline bool operator==(const ColorRgbw & lhs, const ColorRgbw & rhs)
{
	return	lhs.red   == rhs.red   &&
		lhs.green == rhs.green &&
		lhs.blue  == rhs.blue  &&
		lhs.white == rhs.white;
}

/// Compare operator to check if a color is 'smaller' than another color
inline bool operator<(const ColorRgbw & lhs, const ColorRgbw & rhs)
{
	return	lhs.red   < rhs.red   &&
		lhs.green < rhs.green &&
		lhs.blue  < rhs.blue  &&
		lhs.white < rhs.white;
}

/// Compare operator to check if a color is 'smaller' than or 'equal' to another color
inline bool operator<=(const ColorRgbw & lhs, const ColorRgbw & rhs)
{
	return lhs < rhs || lhs == rhs;
}
