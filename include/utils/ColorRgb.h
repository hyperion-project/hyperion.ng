#pragma once

// STL includes
#include <cstdint>
#include <iostream>

#include <QString>
#include <QTextStream>
#include <QRgb>
#include <QDebug>

#include <utils/global_defines.h>
#include <utils/Packed.h>

///
/// Plain-Old-Data structure containing the red-green-blue color specification. Size of the
/// structure is exactly 3-bytes for easy writing to led-device
///

PACKED_STRUCT_BEGIN
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
	/// 'Cyan' RgbColor (0, 255, 255)
	static const ColorRgb CYAN;
	/// 'Magenta' RgbColor (255, 0,255)
	static const ColorRgb MAGENTA;

	ColorRgb() : ColorRgb(ColorRgb::BLACK)
	{
	}

	ColorRgb(uint8_t _red, uint8_t _green, uint8_t _blue) : red(_red),
															green(_green),
															blue(_blue)
	{
	}

	explicit ColorRgb(const QRgb rgb)
	{
		setRgb(rgb);
	}

	ColorRgb& operator=(const ColorRgb& rhs) = default;	

	friend inline ColorRgb operator-(ColorRgb a, const ColorRgb &b)
	{
		a.red -= b.red;
		a.green -= b.green;
		a.blue -= b.blue;
		return a;
	}

	QRgb rgb() const
	{
		return qRgb(red, green, blue);
	}

	void setRgb(QRgb rgb)
	{
		red = static_cast<uint8_t>(qRed(rgb));
		green = static_cast<uint8_t>(qGreen(rgb));
		blue = static_cast<uint8_t>(qBlue(rgb));
	}

	QString toQString() const
	{
		return QString("(%1,%2,%3)").arg(red).arg(green).arg(blue);
	}

	static ColorRgb white(uint16_t whiteColorTempK)
	{
		// based on https://tannerhelland.com/2012/09/18/convert-temperature-rgb-algorithm-code.html
		const double t = whiteColorTempK / 100.0;
		ColorRgb result;

		// red channel
		double r = (t <= 66) ? 255.0 : 329.698727446 * pow(t - 60, -0.1332047592);
		result.red = static_cast<uint8_t>(qBound(0.0, r, 255.0));

		// green channel
		double g = (t <= 66) ? 99.4708025861 * log(t) - 161.1195681661
		                     : 288.1221695283 * pow(t - 60, -0.0755148492);
		result.green = static_cast<uint8_t>(qBound(0.0, g, 255.0));

		// blue channel
		double b;
		if (t >= 66)
			b = 255.0;
		else if (t <= 19)
			b = 0.0;
		else
			b = 138.5177312231 * log(t - 10) - 305.0447927307;
		result.blue = static_cast<uint8_t>(qBound(0.0, b, 255.0));

		return result;
	}

	///
	/// Stream operator to write ColorRgb to an outputstream (format "'{'[red]','[green]','[blue]'}'")
	///
	/// @param os The output stream
	/// @param color The color to write
	/// @return The output stream (with the color written to it)
	///
	friend inline std::ostream &operator<<(std::ostream &os, const ColorRgb &color)
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
	friend inline QTextStream &operator<<(QTextStream &os, const ColorRgb &color)
	{
		os << "{"
		   << static_cast<unsigned>(color.red) << ","
		   << static_cast<unsigned>(color.green) << ","
		   << static_cast<unsigned>(color.blue)
		   << "}";

		return os;
	}

	/// Compare operator to check if a color is 'equal' to another color
	friend inline bool operator==(const ColorRgb &lhs, const ColorRgb &rhs)
	{
		return lhs.red == rhs.red &&
			   lhs.green == rhs.green &&
			   lhs.blue == rhs.blue;
	}

	/// Compare operator to check if a color is 'smaller' than another color
	friend inline bool operator<(const ColorRgb &lhs, const ColorRgb &rhs)
	{
		return lhs.red < rhs.red &&
			   lhs.green < rhs.green &&
			   lhs.blue < rhs.blue;
	}

	/// Compare operator to check if a color is 'not equal' to another color
	friend inline bool operator!=(const ColorRgb &lhs, const ColorRgb &rhs)
	{
		return !(lhs == rhs);
	}

	/// Compare operator to check if a color is 'smaller' than or 'equal' to another color
	friend inline bool operator<=(const ColorRgb &lhs, const ColorRgb &rhs)
	{
		return lhs.red <= rhs.red &&
			   lhs.green <= rhs.green &&
			   lhs.blue <= rhs.blue;
	}

	/// Compare operator to check if a color is 'greater' to another color
	friend inline bool operator>(const ColorRgb &lhs, const ColorRgb &rhs)
	{
		return lhs.red > rhs.red &&
			   lhs.green > rhs.green &&
			   lhs.blue > rhs.blue;
	}

	/// Compare operator to check if a color is 'greater' than or 'equal' to another color
	friend inline bool operator>=(const ColorRgb &lhs, const ColorRgb &rhs)
	{
		return lhs.red >= rhs.red &&
			   lhs.green >= rhs.green &&
			   lhs.blue >= rhs.blue;
	}

	friend inline QDebug operator<<(QDebug dbg, const ColorRgb &color)
	{
		dbg.noquote().nospace() << color.toQString();
		return dbg.space();
	}

	friend inline QDebug operator<<(QDebug dbg, const QVector<ColorRgb> &colors)
	{
		dbg.noquote().nospace() << "Color " << limitForDebug(colors, -1);
		return dbg.space();
	}
} PACKED_STRUCT_END;

static_assert(sizeof(ColorRgb) == 3, "ColorRgb must be exactly 3 bytes");
