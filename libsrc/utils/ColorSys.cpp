#include <utils/ColorSys.h>

#include <QColor>
#include <oklab/ok_color.h>

inline uint8_t clamp(int x)
{
	return (x<0) ? 0 : ((x>255) ? 255 : uint8_t(x));
}

inline double clamp(double x)
{
	return std::max(0.0, std::min(x, 1.0));
}

void ColorSys::rgb2hsl(uint8_t red, uint8_t green, uint8_t blue, uint16_t & hue, float & saturation, float & luminance)
{
	QColor color(red,green,blue);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	float h, s, l;
#else
	qreal h, s, l;
#endif
	color.getHslF(&h,&s,&l);
	hue        = h;
	saturation = s;
	luminance  = l;
}

void ColorSys::hsl2rgb(uint16_t hue, float saturation, float luminance, uint8_t & red, uint8_t & green, uint8_t & blue)
{
	QColor color(QColor::fromHslF(hue,(qreal)saturation,(qreal)luminance));
	red   = (uint8_t)color.red();
	green = (uint8_t)color.green();
	blue  = (uint8_t)color.blue();
}

void ColorSys::rgb2hsv(uint8_t red, uint8_t green, uint8_t blue, uint16_t & hue, uint8_t & saturation, uint8_t & value)
{
	QColor color(red,green,blue);
	hue        = color.hsvHue();
	saturation = color.hsvSaturation();
	value      = color.value();
}

void ColorSys::hsv2rgb(uint16_t hue, uint8_t saturation, uint8_t value, uint8_t & red, uint8_t & green, uint8_t & blue)
{
	QColor color(QColor::fromHsv(hue,saturation,value));
	red   = (uint8_t)color.red();
	green = (uint8_t)color.green();
	blue  = (uint8_t)color.blue();
}

void ColorSys::yuv2rgb(uint8_t y, uint8_t u, uint8_t v, uint8_t &r, uint8_t &g, uint8_t &b)
{
	// see: http://en.wikipedia.org/wiki/YUV#Y.27UV444_to_RGB888_conversion
	int c = y - 16;
	int d = u - 128;
	int e = v - 128;

	r = clamp((298 * c + 409 * e + 128) >> 8);
	g = clamp((298 * c - 100 * d - 208 * e + 128) >> 8);
	b = clamp((298 * c + 516 * d + 128) >> 8);
}

void ColorSys::rgb2okhsv(uint8_t red, uint8_t green, uint8_t blue, double & hue, double & saturation, double & value)
{
	ok_color::HSV color = ok_color::srgb_to_okhsv({ static_cast<double>(red)   / 255.0,
													static_cast<double>(green) / 255.0,
													static_cast<double>(blue)  / 255.0
	});
	hue = color.h;
	saturation = color.s;
	value = color.v;
}

void ColorSys::okhsv2rgb(double hue, double saturation, double value, uint8_t & red, uint8_t & green, uint8_t & blue)
{
	ok_color::RGB color = ok_color::okhsv_to_srgb({ hue, saturation, value });
	// okhsv_to_srgb can output rgb colors with slightly negative components. Clamping them before casting prevents rollover errors
	red = static_cast<uint8_t>(std::lround(clamp(color.r) * 255.0));
	green = static_cast<uint8_t>(std::lround(clamp(color.g) * 255.0));
	blue = static_cast<uint8_t>(std::lround(clamp(color.b) * 255.0));
}
