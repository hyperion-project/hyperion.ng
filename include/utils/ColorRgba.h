#pragma once

// STL includes
#include <cstdint>
#include <ostream>

#include <QString>
#include <QDebug>

#include <utils/ColorRgb.h>
#include <utils/Packed.h>

PACKED_STRUCT_BEGIN
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

	/// 'Black' RgbaColor (0, 0, 0, 255)
	static const ColorRgba BLACK;
	/// 'Red' RgbaColor (255, 0, 0, 255)
	static const ColorRgba RED;
	/// 'Green' RgbaColor (0, 255, 0, 255)
	static const ColorRgba GREEN;
	/// 'Blue' RgbaColor (0, 0, 255, 255)
	static const ColorRgba BLUE;
	/// 'Yellow' RgbaColor (255, 255, 0, 255)
	static const ColorRgba YELLOW;
	/// 'White' RgbaColor (255, 255, 255, 255
	static const ColorRgba WHITE;

	ColorRgba() : ColorRgba(ColorRgba::BLACK)
	{
	}

	ColorRgba(uint8_t _red, uint8_t _green, uint8_t _blue, uint8_t _alpha):
			red(_red),
			green(_green),
			blue(_blue),
			alpha(_alpha)
	{
	}

	explicit ColorRgba(ColorRgb rgb):
			red(rgb.red),
			green(rgb.green),
			blue(rgb.blue),
			alpha(255)
	{
	}

	ColorRgba& operator=(const ColorRgb& rhs)
    {
        red = rhs.red;
        green = rhs.green;
        blue = rhs.blue;
        alpha = 255;
        return *this;
    }

	friend inline ColorRgba operator-(ColorRgba a, const ColorRgba &b)
	{
		a.red -= b.red;
		a.green -= b.green;
		a.blue -= b.blue;
		a.alpha -= b.alpha;		
		return a;
	}

	QString toQString() const
	{
		return QString("(%1,%2,%3,%4)").arg(red).arg(green).arg(blue).arg(alpha);
	}

	///
	/// Stream operator to write ColorRgba to an outputstream (format "'{'[red]','[green]','[blue]','[alpha]'}'")
	///
	/// @param os The output stream
	/// @param color The color to write
	/// @return The output stream (with the color written to it)
	///
	friend inline std::ostream& operator<<(std::ostream& os, const ColorRgba& color)
	{
		os << "{"
			<< color.alpha << ","
			<< color.red   << ","
			<< color.green << ","
			<< color.blue
		<< "}";

		return os;
	}

	friend inline QDebug operator<<(QDebug dbg, const ColorRgba &color)
	{
		dbg.noquote().nospace() << color.toQString();
		return dbg.space();
	}

	friend inline QDebug operator<<(QDebug dbg, const QVector<ColorRgba> &colors)
	{
		dbg.noquote().nospace() << "Color " << limitForDebug(colors, -1);
		return dbg.space();
	}	
}
PACKED_STRUCT_END;

static_assert(sizeof(ColorRgba) == 4, "ColorRgba must be exactly 4 bytes");

