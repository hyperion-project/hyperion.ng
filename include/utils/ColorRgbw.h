#pragma once

// STL includes
#include <cstdint>
#include <iostream>

#include <QString>
#include <QDebug>

#include <utils/ColorRgb.h>
#include <utils/Packed.h>

///
/// Plain-Old-Data structure containing the red-green-blue color specification. Size of the
/// structure is exactly 3-bytes for easy writing to led-device
///
PACKED_STRUCT_BEGIN
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

	/// 'Black' RgbwColor (0, 0, 0, 0)
	static const ColorRgbw BLACK;
	/// 'Red' RgbwColor (255, 0, 0, 0)
	static const ColorRgbw RED;
	/// 'Green' RgbwColor (0, 255, 0, 0)
	static const ColorRgbw GREEN;
	/// 'Blue' RgbwColor (0, 0, 255, 0)
	static const ColorRgbw BLUE;
	/// 'Yellow' RgbwColor (255, 255, 0, 0)
	static const ColorRgbw YELLOW;
	/// 'White' RgbwColor (0, 0, 0, 255)
	static const ColorRgbw WHITE;

	ColorRgbw() : ColorRgbw(ColorRgbw::BLACK)
	{
	}

	ColorRgbw(uint8_t _red, uint8_t _green, uint8_t _blue, uint8_t _white):
			red(_red),
			green(_green),
			blue(_blue),
			white(_white)
	{
	}

	explicit ColorRgbw(ColorRgb rgb):
			red(rgb.red),
			green(rgb.green),
			blue(rgb.blue),
			white(255)
	{
	}

	ColorRgbw& operator=(const ColorRgb& rhs)
    {
        red = rhs.red;
        green = rhs.green;
        blue = rhs.blue;
        white = 255;
        return *this;
    }

	friend inline ColorRgbw operator-(ColorRgbw a, const ColorRgbw &b)
	{
		a.red -= b.red;
		a.green -= b.green;
		a.blue -= b.blue;
		a.white -= b.white;		
		return a;
	}

	QString toQString() const
	{
		return QString("(%1,%2,%3,%4)").arg(red).arg(green).arg(blue).arg(white);
	}

	///
	/// Stream operator to write ColorRgbw to an outputstream (format "'{'[red]','[green]','[blue]','[white]'}'")
	///
	/// @param os The output stream
	/// @param color The color to write
	/// @return The output stream (with the color written to it)
	///
	friend inline std::ostream& operator<<(std::ostream& os, const ColorRgbw& color)
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
	friend inline bool operator==(const ColorRgbw & lhs, const ColorRgbw & rhs)
	{
		return	lhs.red   == rhs.red   &&
			lhs.green == rhs.green &&
			lhs.blue  == rhs.blue  &&
			lhs.white == rhs.white;
	}

	/// Compare operator to check if a color is 'smaller' than another color
	friend inline bool operator<(const ColorRgbw & lhs, const ColorRgbw & rhs)
	{
		return	lhs.red   < rhs.red   &&
			lhs.green < rhs.green &&
			lhs.blue  < rhs.blue  &&
			lhs.white < rhs.white;
	}

	/// Compare operator to check if a color is 'smaller' than or 'equal' to another color
	friend inline bool operator<=(const ColorRgbw & lhs, const ColorRgbw & rhs)
	{
		return lhs < rhs || lhs == rhs;
	}

	friend inline QDebug operator<<(QDebug dbg, const ColorRgbw &color)
	{
		dbg.noquote().nospace() << color.toQString();
		return dbg.space();
	}

	friend inline QDebug operator<<(QDebug dbg, const QVector<ColorRgbw> &colors)
	{
		dbg.noquote().nospace() << "Color " << limitForDebug(colors, -1);
		return dbg.space();
	}

}
PACKED_STRUCT_END;

static_assert(sizeof(ColorRgbw) == 4, "ColorRgbw must be exactly 4 bytes");
