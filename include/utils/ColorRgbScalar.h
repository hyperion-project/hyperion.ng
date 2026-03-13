#ifndef COLORRGBSCALAR_H
#define COLORRGBSCALAR_H

// STL includes
#include <cstdint>
#include <iostream>

#include <QString>
#include <QTextStream>
#include <QRgb>
#include <QDebug>

#include <utils/ColorRgb.h>
#include <utils/Packed.h>

///
/// Plain-Old-Data structure containing the red-green-blue color specification. Size of the
/// structure is exactly 3 times int for easy writing to led-device
///

PACKED_STRUCT_BEGIN
struct ColorRgbScalar
{
	/// The red color channel
	int red;
	/// The green color channel
	int green;
	/// The blue color channel
	int blue;

	/// 'Black' RgbScalarColor (0, 0, 0)
	static const ColorRgbScalar BLACK;
	/// 'Red' RgbScalarColor (255, 0, 0)
	static const ColorRgbScalar RED;
	/// 'Green' RgbScalarColor (0, 255, 0)
	static const ColorRgbScalar GREEN;
	/// 'Blue' RgbScalarColor (0, 0, 255)
	static const ColorRgbScalar BLUE;
	/// 'Yellow' RgbScalarColor (255, 255, 0)
	static const ColorRgbScalar YELLOW;
	/// 'White' RgbScalarColor (255, 255, 255)
	static const ColorRgbScalar WHITE;

	ColorRgbScalar() : ColorRgbScalar(ColorRgbScalar::BLACK)
	{
	}

	ColorRgbScalar(int _red, int _green,int _blue):
		  red(_red),
		  green(_green),
		  blue(_blue)
	{
	}

	explicit ColorRgbScalar(ColorRgb rgb):
		  red(rgb.red),
		  green(rgb.green),
		  blue(rgb.blue)
	{
	}

	ColorRgbScalar& operator=(const ColorRgb& rhs)
    {
        red = rhs.red;
        green = rhs.green;
        blue = rhs.blue;
        return *this;
    }

	friend inline ColorRgbScalar operator-(ColorRgbScalar a, const ColorRgbScalar &b)
	{
		a.red -= b.red;
		a.green -= b.green;
		a.blue -= b.blue;
		return a;
	}

	void setRgb(QRgb rgb)
	{
		red = qRed(rgb);
		green = qGreen(rgb);
		blue = qBlue(rgb);
	}

	void setRgb(ColorRgb rgb)
	{
		red = rgb.red;
		green = rgb.green;
		blue = rgb.blue;
	}

	QString toQString() const
	{
		return QString("(%1,%2,%3)").arg(red).arg(green).arg(blue);
	}
	
	///
	/// Stream operator to write ColorRgbInt to an outputstream (format "'{'[red]','[green]','[blue]'}'")
	///
	/// @param os The output stream
	/// @param color The color to write
	/// @return The output stream (with the color written to it)
	///
	friend inline std::ostream& operator<<(std::ostream& os, const ColorRgbScalar& color)
	{
		os << "{"
		<< static_cast<unsigned>(color.red) << ","
		<< static_cast<unsigned>(color.green) << ","
		<< static_cast<unsigned>(color.blue)
		<< "}";

		return os;
	}

	///
	/// Stream operator to write ColorRgbInt to a QTextStream (format "'{'[red]','[green]','[blue]'}'")
	///
	/// @param os The output stream
	/// @param color The color to write
	/// @return The output stream (with the color written to it)
	///
	friend inline QTextStream& operator<<(QTextStream &os, const ColorRgbScalar& color)
	{
		os << "{"
		<< static_cast<unsigned>(color.red) << ","
		<< static_cast<unsigned>(color.green) << ","
		<< static_cast<unsigned>(color.blue)
		<< "}";

		return os;
	}

	/// Compare operator to check if a color is 'equal' to another color
	friend inline bool operator==(const ColorRgbScalar & lhs, const ColorRgbScalar & rhs)
	{
		return	lhs.red   == rhs.red   &&
			lhs.green == rhs.green &&
			lhs.blue  == rhs.blue;
	}

	/// Compare operator to check if a color is 'smaller' than another color
	friend inline bool operator<(const ColorRgbScalar & lhs, const ColorRgbScalar & rhs)
	{
		return	lhs.red   < rhs.red   &&
			lhs.green < rhs.green &&
			lhs.blue  < rhs.blue;
	}

	/// Compare operator to check if a color is 'not equal' to another color
	friend inline bool operator!=(const ColorRgbScalar & lhs, const ColorRgbScalar & rhs)
	{
		return !(lhs == rhs);
	}

	/// Compare operator to check if a color is 'smaller' than or 'equal' to another color
	friend inline bool operator<=(const ColorRgbScalar & lhs, const ColorRgbScalar & rhs)
	{
		return	lhs.red   <= rhs.red   &&
			lhs.green <= rhs.green &&
			lhs.blue  <= rhs.blue;
	}

	/// Compare operator to check if a color is 'greater' to another color
	friend inline bool operator>(const ColorRgbScalar & lhs, const ColorRgbScalar & rhs)
	{
		return	lhs.red   > rhs.red   &&
			lhs.green > rhs.green &&
			lhs.blue  > rhs.blue;
	}

	/// Compare operator to check if a color is 'greater' than or 'equal' to another color
	friend inline bool operator>=(const ColorRgbScalar & lhs, const ColorRgbScalar & rhs)
	{
		return	lhs.red   >= rhs.red   &&
			lhs.green >= rhs.green &&
			lhs.blue  >= rhs.blue;
	}

	friend inline ColorRgbScalar& operator+=(ColorRgbScalar& lhs, const ColorRgbScalar& rhs)
	{
		lhs.red   += rhs.red;
		lhs.green   += rhs.green;
		lhs.blue   += rhs.blue;

		return	lhs;
	}

	friend inline ColorRgbScalar operator+(ColorRgbScalar lhs, const ColorRgbScalar rhs)
	{
		lhs += rhs;
		return	lhs;
	}

	friend inline ColorRgbScalar& operator/=(ColorRgbScalar& lhs, int count)
	{
		if (count > 0)
		{
			lhs.red   /= count;
			lhs.green /= count;
			lhs.blue  /= count;
		}
		return	lhs;
	}

	friend inline ColorRgbScalar operator/(ColorRgbScalar lhs, int count)
	{
		lhs /= count;
		return	lhs;
	}

	friend inline QDebug operator<<(QDebug dbg, const ColorRgbScalar &color)
	{
		dbg.noquote().nospace() << color.toQString();
		return dbg.space();
	}

	friend inline QDebug operator<<(QDebug dbg, const QVector<ColorRgbScalar> &colors)
	{
		dbg.noquote().nospace() << "Color " << limitForDebug(colors, -1);
		return dbg.space();
	}
}
PACKED_STRUCT_END;

/// Assert to ensure that the size of the structure is 'only' 3 times int
static_assert(sizeof(ColorRgbScalar) == 3 * sizeof(int), "Incorrect size of ColorRgbInt");


#endif // COLORRGBSCALAR_H
