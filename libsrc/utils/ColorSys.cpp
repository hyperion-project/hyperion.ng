#include <utils/ColorSys.h>

#include <QColor>
void ColorSys::rgb2hsl(uint8_t red, uint8_t green, uint8_t blue, uint16_t & hue, float & saturation, float & luminance)
{
	QColor color(red,green,blue);
	qreal h,s,l;
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
