#pragma once

// STL includes
#include <cstdint>
#include <ostream>

struct ColorArgb
{
	/// The alpha mask channel
	uint8_t alpha;

	/// The red color channel
	uint8_t red;
	/// The green color channel
	uint8_t green;
	/// The blue color channel
	uint8_t blue;

	/// 'Black' RgbColor (255, 0, 0, 0)
	static const ColorArgb BLACK;
	/// 'Red' RgbColor (255, 255, 0, 0)
	static const ColorArgb RED;
	/// 'Green' RgbColor (255, 0, 255, 0)
	static const ColorArgb GREEN;
	/// 'Blue' RgbColor (255, 0, 0, 255)
	static const ColorArgb BLUE;
	/// 'Yellow' RgbColor (255, 255, 255, 0)
	static const ColorArgb YELLOW;
	/// 'White' RgbColor (255, 255, 255, 255)
	static const ColorArgb WHITE;
};

/// Assert to ensure that the size of the structure is 'only' 3 bytes
static_assert(sizeof(ColorArgb) == 4, "Incorrect size of ColorARGB");

///
/// Stream operator to write ColorRgb to an outputstream (format "'{'[alpha]', '[red]','[green]','[blue]'}'")
///
/// @param os The output stream
/// @param color The color to write
/// @return The output stream (with the color written to it)
///
inline std::ostream& operator<<(std::ostream& os, const ColorArgb& color)
{
	os << "{"
		<< color.alpha << ","
		<< color.red   << ","
		<< color.green << ","
		<< color.blue
	<< "}";

	return os;
}
