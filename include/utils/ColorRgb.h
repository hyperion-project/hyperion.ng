#pragma once

// STL includes
#include <cstdint>
#include <iostream>

#include <QTextStream>

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
	static const ColorRgb BLACK;
	/// 'Red' RgbColor (255, 0, 0)
	static const ColorRgb RED;
	/// 'Green' RgbColor (0, 255, 0)
	static const ColorRgb GREEN;
	/// 'Blue' RgbColor (0, 0, 255)
	static const ColorRgb BLUE;
	/// 'Yellow' RgbColor (255, 255, 0)
	static const ColorRgb YELLOW;
	/// 'White' RgbColor (255, 255, 255)
	static const ColorRgb WHITE;

	ColorRgb() = default;

	ColorRgb(uint8_t _red, uint8_t _green,uint8_t _blue):
		  red(_red),
		  green(_green),
		  blue(_blue)
	{

	}

	ColorRgb operator-(const ColorRgb& b) const
	{
		ColorRgb a(*this);
		a.red -= b.red;
		a.green -= b.green;
		a.blue -= b.blue;
		return a;
	}

	QString toQString() const
	{
		return QString("(%1,%2,%3)").arg(red).arg(green).arg(blue);
	}
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
	os << "{"
	   << static_cast<unsigned>(color.red) << ","
	   << static_cast<unsigned>(color.green) << ","
	   << static_cast<unsigned>(color.blue)
	<< "}";

	return os;
}

///
/// Stream operator to write ColorRgb to a QTextStream (format "'{'[red]','[green]','[blue]'}'")
///
/// @param os The output stream
/// @param color The color to write
/// @return The output stream (with the color written to it)
///
inline QTextStream& operator<<(QTextStream &os, const ColorRgb& color)
{
	os << "{"
	   << static_cast<unsigned>(color.red) << ","
	   << static_cast<unsigned>(color.green) << ","
	   << static_cast<unsigned>(color.blue)
	<< "}";

	return os;
}

/// Compare operator to check if a color is 'equal' to another color
inline bool operator==(const ColorRgb & lhs, const ColorRgb & rhs)
{
	return	lhs.red   == rhs.red   &&
		lhs.green == rhs.green &&
		lhs.blue  == rhs.blue;
}

/// Compare operator to check if a color is 'smaller' than another color
inline bool operator<(const ColorRgb & lhs, const ColorRgb & rhs)
{
	return	lhs.red   < rhs.red   &&
		lhs.green < rhs.green &&
		lhs.blue  < rhs.blue;
}

/// Compare operator to check if a color is 'not equal' to another color
inline bool operator!=(const ColorRgb & lhs, const ColorRgb & rhs)
{
	return !(lhs == rhs);
}

/// Compare operator to check if a color is 'smaller' than or 'equal' to another color
inline bool operator<=(const ColorRgb & lhs, const ColorRgb & rhs)
{
	return	lhs.red   <= rhs.red   &&
		lhs.green <= rhs.green &&
		lhs.blue  <= rhs.blue;
}

/// Compare operator to check if a color is 'greater' to another color
inline bool operator>(const ColorRgb & lhs, const ColorRgb & rhs)
{
	return	lhs.red   > rhs.red   &&
		lhs.green > rhs.green &&
		lhs.blue  > rhs.blue;
}

/// Compare operator to check if a color is 'greater' than or 'equal' to another color
inline bool operator>=(const ColorRgb & lhs, const ColorRgb & rhs)
{
	return	lhs.red   >= rhs.red   &&
		lhs.green >= rhs.green &&
		lhs.blue  >= rhs.blue;
}
