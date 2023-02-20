#ifndef COLORRGBSCALAR_H
#define COLORRGBSCALAR_H

// STL includes
#include <cstdint>
#include <iostream>

#include <QString>
#include <QTextStream>
#include <QRgb>
#include <utils/ColorRgb.h>

///
/// Plain-Old-Data structure containing the red-green-blue color specification. Size of the
/// structure is exactly 3 times int for easy writing to led-device
///
struct ColorRgbScalar
{
	/// The red color channel
	int red;
	/// The green color channel
	int green;
	/// The blue color channel
	int blue;

	/// 'Black' RgbColor (0, 0, 0)
	static const ColorRgbScalar BLACK;
	/// 'Red' RgbColor (255, 0, 0)
	static const ColorRgbScalar RED;
	/// 'Green' RgbColor (0, 255, 0)
	static const ColorRgbScalar GREEN;
	/// 'Blue' RgbColor (0, 0, 255)
	static const ColorRgbScalar BLUE;
	/// 'Yellow' RgbColor (255, 255, 0)
	static const ColorRgbScalar YELLOW;
	/// 'White' RgbColor (255, 255, 255)
	static const ColorRgbScalar WHITE;

	ColorRgbScalar() = default;

	ColorRgbScalar(int _red, int _green,int _blue):
		  red(_red),
		  green(_green),
		  blue(_blue)
	{

	}

	ColorRgbScalar(ColorRgb rgb):
		  red(rgb.red),
		  green(rgb.green),
		  blue(rgb.blue)
	{

	}

	ColorRgbScalar operator-(const ColorRgbScalar& b) const
	{
		ColorRgbScalar a(*this);
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
};
/// Assert to ensure that the size of the structure is 'only' 3 times int
static_assert(sizeof(ColorRgbScalar) == 3 * sizeof(int), "Incorrect size of ColorRgbInt");


///
/// Stream operator to write ColorRgbInt to an outputstream (format "'{'[red]','[green]','[blue]'}'")
///
/// @param os The output stream
/// @param color The color to write
/// @return The output stream (with the color written to it)
///
inline std::ostream& operator<<(std::ostream& os, const ColorRgbScalar& color)
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
inline QTextStream& operator<<(QTextStream &os, const ColorRgbScalar& color)
{
	os << "{"
	   << static_cast<unsigned>(color.red) << ","
	   << static_cast<unsigned>(color.green) << ","
	   << static_cast<unsigned>(color.blue)
	<< "}";

	return os;
}

/// Compare operator to check if a color is 'equal' to another color
inline bool operator==(const ColorRgbScalar & lhs, const ColorRgbScalar & rhs)
{
	return	lhs.red   == rhs.red   &&
		lhs.green == rhs.green &&
		lhs.blue  == rhs.blue;
}

/// Compare operator to check if a color is 'smaller' than another color
inline bool operator<(const ColorRgbScalar & lhs, const ColorRgbScalar & rhs)
{
	return	lhs.red   < rhs.red   &&
		lhs.green < rhs.green &&
		lhs.blue  < rhs.blue;
}

/// Compare operator to check if a color is 'not equal' to another color
inline bool operator!=(const ColorRgbScalar & lhs, const ColorRgbScalar & rhs)
{
	return !(lhs == rhs);
}

/// Compare operator to check if a color is 'smaller' than or 'equal' to another color
inline bool operator<=(const ColorRgbScalar & lhs, const ColorRgbScalar & rhs)
{
	return	lhs.red   <= rhs.red   &&
		lhs.green <= rhs.green &&
		lhs.blue  <= rhs.blue;
}

/// Compare operator to check if a color is 'greater' to another color
inline bool operator>(const ColorRgbScalar & lhs, const ColorRgbScalar & rhs)
{
	return	lhs.red   > rhs.red   &&
		lhs.green > rhs.green &&
		lhs.blue  > rhs.blue;
}

/// Compare operator to check if a color is 'greater' than or 'equal' to another color
inline bool operator>=(const ColorRgbScalar & lhs, const ColorRgbScalar & rhs)
{
	return	lhs.red   >= rhs.red   &&
		lhs.green >= rhs.green &&
		lhs.blue  >= rhs.blue;
}

inline ColorRgbScalar& operator+=(ColorRgbScalar& lhs, const ColorRgbScalar& rhs)
{
	lhs.red   += rhs.red;
	lhs.green   += rhs.green;
	lhs.blue   += rhs.blue;

	return	lhs;
}

inline ColorRgbScalar operator+(ColorRgbScalar lhs, const ColorRgbScalar rhs)
{
	lhs += rhs;
	return	lhs;
}

inline ColorRgbScalar& operator/=(ColorRgbScalar& lhs, int count)
{
	if (count > 0)
	{
		lhs.red   /= count;
		lhs.green /= count;
		lhs.blue  /= count;
	}
	return	lhs;
}

inline ColorRgbScalar operator/(ColorRgbScalar lhs, int count)
{
	lhs /= count;
	return	lhs;
}

#endif // COLORRGBSCALAR_H
