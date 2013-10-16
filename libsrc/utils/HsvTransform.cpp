#include <iostream>
#include <utils/HsvTransform.h>

HsvTransform::HsvTransform() :
	_saturationGain(1.0),
	_valueGain(1.0)
{
}

HsvTransform::HsvTransform(double saturationGain, double valueGain) :
	_saturationGain(saturationGain),
	_valueGain(valueGain)
{
}

HsvTransform::~HsvTransform()
{
}

void HsvTransform::setSaturationGain(double saturationGain)
{
	_saturationGain = saturationGain;
}

double HsvTransform::getSaturationGain() const
{
	return _saturationGain;
}

void HsvTransform::setValueGain(double valueGain)
{
	_valueGain = valueGain;
}

double HsvTransform::getValueGain() const
{
	return _valueGain;
}

void HsvTransform::transform(uint8_t & red, uint8_t & green, uint8_t & blue) const
{
	if (_saturationGain != 1.0 || _valueGain != 1.0)
	{
		uint16_t hue;
		uint8_t saturation, value;
		rgb2hsv(red, green, blue, hue, saturation, value);

		int s = saturation * _saturationGain;
		if (s > 255)
			saturation = 255;
		else
			saturation = s;

		int v = value * _valueGain;
		if (v > 255)
			value = 255;
		else
			value = v;

		hsv2rgb(hue, saturation, value, red, green, blue);
	}
}

void HsvTransform::rgb2hsv(uint8_t red, uint8_t green, uint8_t blue, uint16_t & hue, uint8_t & saturation, uint8_t & value)
{
	uint8_t rgbMin, rgbMax;

	rgbMin = red < green ? (red < blue ? red : blue) : (green < blue ? green : blue);
	rgbMax = red > green ? (red > blue ? red : blue) : (green > blue ? green : blue);

	value = rgbMax;
	if (value == 0)
	{
		hue = 0;
		saturation = 0;
		return;
	}

	saturation = 255 * long(rgbMax - rgbMin) / value;
	if (saturation == 0)
	{
		hue = 0;
		return;
	}

	if (rgbMax == red)
	{
		// start from 360 to be sure that we won't assign a negative number to the unsigned hue value
		hue = 360 + 60 * (green - blue) / (rgbMax - rgbMin);

		if (hue > 359)
			hue -= 360;
	}
	else if (rgbMax == green)
	{
		hue = 120 + 60 * (blue - red) / (rgbMax - rgbMin);
	}
	else
	{
		hue = 240 + 60 * (red - green) / (rgbMax - rgbMin);
	}
}

void HsvTransform::hsv2rgb(uint16_t hue, uint8_t saturation, uint8_t value, uint8_t & red, uint8_t & green, uint8_t & blue)
{
	uint8_t region, remainder, p, q, t;

	if (saturation == 0)
	{
		red = value;
		green = value;
		blue = value;
		return;
	}

	region = hue / 60;
	remainder = (hue - (region * 60)) * 256 / 60;

	p = (value * (255 - saturation)) >> 8;
	q = (value * (255 - ((saturation * remainder) >> 8))) >> 8;
	t = (value * (255 - ((saturation * (255 - remainder)) >> 8))) >> 8;

	switch (region)
	{
		case 0:
			red = value; green = t; blue = p;
			break;
		case 1:
			red = q; green = value; blue = p;
			break;
		case 2:
			red = p; green = value; blue = t;
			break;
		case 3:
			red = p; green = q; blue = value;
			break;
		case 4:
			red = t; green = p; blue = value;
			break;
		default:
			red = value; green = p; blue = q;
			break;
	}
}
