#pragma once

// STL includes
#include <cstdint>
#include <ostream>

#include <QString>
#include <QDebug>

#include <utils/ColorRgb.h>
#include <utils/Packed.h>

PACKED_STRUCT_BEGIN
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

	/// 'Black' ArgbColor (255, 0, 0, 0)
	static const ColorArgb BLACK;
	/// 'Red' ArgbColor (255, 255, 0, 0)
	static const ColorArgb RED;
	/// 'Green' ArgbColor (255, 0, 255, 0)
	static const ColorArgb GREEN;
	/// 'Blue' ArgbColor (255, 0, 0, 255)
	static const ColorArgb BLUE;
	/// 'Yellow' ArgbColor (255, 255, 255, 0)
	static const ColorArgb YELLOW;
	/// 'White' ArgbColor (255, 255, 255, 255)
	static const ColorArgb WHITE;

	ColorArgb() : ColorArgb(ColorArgb::BLACK)
	{
	}

	ColorArgb(uint8_t _alpha, uint8_t _red, uint8_t _green, uint8_t _blue):
			alpha(_alpha),
			red(_red),
			green(_green),
			blue(_blue)
	{
	}

	ColorArgb(ColorRgb rgb):
			alpha(255),
			red(rgb.red),
			green(rgb.green),
			blue(rgb.blue)
	{
	}

	ColorArgb operator-(const ColorArgb& b) const
	{
		ColorArgb a(*this);
		a.alpha -= b.alpha;
		a.red -= b.red;
		a.green -= b.green;
		a.blue -= b.blue;
		return a;
	}

	QString toQString() const
	{
		return QString("(%1,%2,%3,%4)").arg(alpha).arg(red).arg(green).arg(blue);
	}

	///
	/// Stream operator to write ColorArgb to an outputstream (format "'{'[alpha]', '[red]','[green]','[blue]'}'")
	///
	/// @param os The output stream
	/// @param color The color to write
	/// @return The output stream (with the color written to it)
	///
	friend inline std::ostream& operator<<(std::ostream& os, const ColorArgb& color)
	{
		os << "{"
			<< color.alpha << ","
			<< color.red   << ","
			<< color.green << ","
			<< color.blue
		<< "}";

		return os;
	}

	friend inline QDebug operator<<(QDebug dbg, const ColorArgb &color)
	{
		dbg.noquote().nospace() << color.toQString();
		return dbg.space();
	}

	friend inline QDebug operator<<(QDebug dbg, const QVector<ColorArgb> &colors)
	{
		dbg.noquote().nospace() << "Color " << limitForDebug(colors, -1);
		return dbg.space();
	}
}
PACKED_STRUCT_END;

static_assert(sizeof(ColorArgb) == 4, "ColorArgb must be exactly 4 bytes");

