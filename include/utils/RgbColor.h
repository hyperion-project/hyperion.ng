
#pragma once

// STL includes
#include <stdint.h>
#include <iostream>

// Forward class declaration
struct RgbColor;

///
/// Plain-Old-Data structure containing the red-green-blue color specification. Size of the
/// structure is exactly 3-bytes for easy writing to led-device
///
struct RgbColor
{
	/// The red color channel
	uint8_t red;
	/// The green color channel
	uint8_t green;
	/// The blue color channel
	uint8_t blue;

	/// 'Black' RgbColor (0, 0, 0)
	static RgbColor BLACK;
	/// 'Red' RgbColor (255, 0, 0)
	static RgbColor RED;
	/// 'Green' RgbColor (0, 255, 0)
	static RgbColor GREEN;
	/// 'Blue' RgbColor (0, 0, 255)
	static RgbColor BLUE;
	/// 'Yellow' RgbColor (255, 255, 0)
	static RgbColor YELLOW;
	/// 'White' RgbColor (255, 255, 255)
	static RgbColor WHITE;

	///
	/// Checks is this exactly matches another color
	///
	/// @param other The other color
	///
	/// @return True if the colors are identical
	///
	inline bool operator==(const RgbColor& other) const
	{
		return red == other.red && green == other.green && blue == other.blue;
	}
};

/// Assert to ensure that the size of the structure is 'only' 3 bytes
static_assert(sizeof(RgbColor) == 3, "Incorrect size of RgbColor");

///
/// Stream operator to write RgbColor to an outputstream (format "'{'[red]','[green]','[blue]'}'")
///
/// @param os The output stream
/// @param color The color to write
/// @return The output stream (with the color written to it)
///
inline std::ostream& operator<<(std::ostream& os, const RgbColor& color)
{
	os << "{" << unsigned(color.red) << "," << unsigned(color.green) << "," << unsigned(color.blue) << "}";
	return os;
}
