#pragma once

// STL includes
#include <cstdint>
#include <ostream>

struct ColorRgba
{
	/// The red color channel
	uint8_t red;
	/// The green color channel
	uint8_t green;
	/// The blue color channel
	uint8_t blue;

	/// The alpha mask channel
	uint8_t alpha;

	/// 'Black' RgbColor (0, 0, 0, 255)
	static const ColorRgba BLACK;
	/// 'Red' RgbColor (255, 0, 0, 255)
	static const ColorRgba RED;
	/// 'Green' RgbColor (0, 255, 0, 255)
	static const ColorRgba GREEN;
	/// 'Blue' RgbColor (0, 0, 255, 255)
	static const ColorRgba BLUE;
	/// 'Yellow' RgbColor (255, 255, 0, 255)
	static const ColorRgba YELLOW;
	/// 'White' RgbColor (255, 255, 255, 255
	static const ColorRgba WHITE;
};

/// Assert to ensure that the size of the structure is 'only' 4 bytes
static_assert(sizeof(ColorRgba) == 4, "Incorrect size of ColorARGB");

///
/// Stream operator to write ColorRgba to an outputstream (format "'{'[alpha]', '[red]','[green]','[blue]'}'")
///
/// @param os The output stream
/// @param color The color to write
/// @return The output stream (with the color written to it)
///
inline std::ostream& operator<<(std::ostream& os, const ColorRgba& color)
{
	os << "{"
		<< color.alpha << ","
		<< color.red   << ","
		<< color.green << ","
		<< color.blue
	<< "}";

	return os;
}
