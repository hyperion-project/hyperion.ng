#include <utils/ColorSys.h>

#include <QColor>
#include <oklab/ok_color.h>

inline uint8_t clamp(int x)
{
	return (x<0) ? 0 : ((x>255) ? 255 : uint8_t(x));
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

void ColorSys::rgb2okhsv(uint8_t red, uint8_t green, uint8_t blue, float & hue, float & saturation, float & value)
{
	ok_color::HSV color = ok_color::srgb_to_okhsv({ (float)red / 255.f, (float)green / 255.f, (float)blue / 255.f });
	hue = color.h;
	saturation = color.s;
	value = color.v;
}

void ColorSys::okhsv2rgb(float hue, float saturation, float value, uint8_t & red, uint8_t & green, uint8_t & blue)
{
	ok_color::RGB color = ok_color::okhsv_to_srgb({ hue, saturation, value });
	red = std::roundf(color.r * 255.f);
	green = std::roundf(color.g * 255.f);
	blue = std::roundf(color.b * 255.f);
}
