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
struct ColorBgr
{
	/// The blue color channel
	uint8_t blue;
	/// The green color channel
	uint8_t green;
	/// The red color channel
	uint8_t red;

	/// 'Black' BgrColor (0, 0, 0)
	static const ColorBgr BLACK;
	/// 'Red' BgrColor (255, 0, 0)
	static const ColorBgr RED;
	/// 'Green' BgrColor (0, 255, 0)
	static const ColorBgr GREEN;
	/// 'Blue' BgrColor (0, 0, 255)
	static const ColorBgr BLUE;
	/// 'Yellow' BgrColor (255, 255, 0)
	static const ColorBgr YELLOW;
	/// 'White' BgrColor (255, 255, 255)
	static const ColorBgr WHITE;


	ColorBgr() : ColorBgr(ColorBgr::BLACK)
	{
	}

	ColorBgr(uint8_t _blue, uint8_t _green, uint8_t _red):
			blue(_blue),
			green(_green),
			red(_red)
	{
	}

	explicit ColorBgr(ColorRgb rgb):
			blue(rgb.blue),
			green(rgb.green),
			red(rgb.red)
	{
	}

    ColorBgr& operator=(const ColorRgb& rhs)
    {
        blue = rhs.blue;
        green = rhs.green;
        red = rhs.red;
        return *this;
    }

	friend inline ColorBgr operator-(ColorBgr a, const ColorBgr &b)
	{
		a.blue -= b.blue;		
		a.green -= b.green;
		a.red -= b.red;
		return a;
	}

	QString toQString() const
	{
		return QString("(%1,%2,%3)").arg(blue).arg(green).arg(red);
	}

	///
	/// Stream operator to write ColorBgr to an outputstream (format "'{'[blue]','[green]','[red]'}'")
	///
	/// @param os The output stream
	/// @param color The color to write
	/// @return The output stream (with the color written to it)
	///
	friend inline std::ostream& operator<<(std::ostream& os, const ColorBgr& color)
	{
		os << "{"
			<< color.red   << ","
			<< color.green << ","
			<< color.blue
		<< "}";

		return os;
	}

	/// Compare operator to check if a color is 'equal' to another color
	friend inline bool operator==(const ColorBgr & lhs, const ColorBgr & rhs)
	{
		return	(lhs.red   == rhs.red)   &&
			(lhs.green == rhs.green) &&
			(lhs.blue  == rhs.blue);
	}

	/// Compare operator to check if a color is 'smaller' than another color
	friend inline bool operator<(const ColorBgr & lhs, const ColorBgr & rhs)
	{
		return	(lhs.red   < rhs.red)   &&
			(lhs.green < rhs.green) &&
			(lhs.blue  < rhs.blue);
	}

	/// Compare operator to check if a color is 'smaller' than or 'equal' to another color
	friend inline bool operator<=(const ColorBgr & lhs, const ColorBgr & rhs)
	{
		return lhs < rhs || lhs == rhs;
	}	

	friend inline QDebug operator<<(QDebug dbg, const ColorBgr &color)
	{
		dbg.noquote().nospace() << color.toQString();
		return dbg.space();
	}

	friend inline QDebug operator<<(QDebug dbg, const QVector<ColorBgr> &colors)
	{
		dbg.noquote().nospace() << "Color " << limitForDebug(colors, -1);
		return dbg.space();
	}	

}
PACKED_STRUCT_END;

static_assert(sizeof(ColorBgr) == 3, "ColorBgr must be exactly 3 bytes");

