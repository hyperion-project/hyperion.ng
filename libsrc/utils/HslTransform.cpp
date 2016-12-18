#include <algorithm>
#include <cmath>
#include <utils/HslTransform.h>

HslTransform::HslTransform()
	: _saturationGain(1.0)
	, _luminanceGain(1.0)
	, _luminanceMinimum(0.0)
{
}

HslTransform::HslTransform(double saturationGain, double luminanceGain, double luminanceMinimum) :
	_saturationGain(saturationGain),
	_luminanceGain(luminanceGain),
	_luminanceMinimum(luminanceMinimum)
{
}

HslTransform::~HslTransform()
{
}

void HslTransform::setSaturationGain(double saturationGain)
{
	_saturationGain = saturationGain;
}

double HslTransform::getSaturationGain() const
{
	return _saturationGain;
}

void HslTransform::setLuminanceGain(double luminanceGain)
{
	_luminanceGain = luminanceGain;
}

double HslTransform::getLuminanceGain() const
{
	return _luminanceGain;
}

void HslTransform::setLuminanceMinimum(double luminanceMinimum)
{
	_luminanceMinimum = luminanceMinimum;
}

double HslTransform::getLuminanceMinimum() const
{
	return _luminanceMinimum;
}

void HslTransform::transform(uint8_t & red, uint8_t & green, uint8_t & blue) const
{
	if (_saturationGain != 1.0 || _luminanceGain != 1.0 || _luminanceMinimum != 0.0)
	{
		uint16_t hue;
		float saturation, luminance;
		rgb2hsl(red, green, blue, hue, saturation, luminance);

		float s = saturation * _saturationGain;
		saturation = std::min(s, 1.0f);

		float l = luminance * _luminanceGain;
		if (l < _luminanceMinimum)
		{
			saturation = 0;
			l = _luminanceMinimum;
		}
		luminance = std::min(l, 1.0f);

		hsl2rgb(hue, saturation, luminance, red, green, blue);
	}
}

void HslTransform::rgb2hsl(uint8_t red, uint8_t green, uint8_t blue, uint16_t & hue, float & saturation, float & luminance)
{
	float r = (float)red   / 255.0f;
	float g = (float)green / 255.0f;
	float b = (float)blue  / 255.0f;	
	
	float rgbMin = std::min(r,std::min(g,b));
	float rgbMax = std::max(r,std::max(g,b));
	float diff = rgbMax - rgbMin;
		
	//luminance
	luminance = (rgbMin + rgbMax) / 2.0f;
	
	if (diff == 0.0f)
	{
		saturation = 0.0f;
		hue        = 0;
		return;
	}
	
	//saturation
	saturation = (luminance < 0.5f)
	           ? (diff / (rgbMin + rgbMax))
	           : (diff / (2.0f - rgbMin - rgbMax));
	
	if (rgbMax == r)
	{
		// start from 360 to be sure that we won't assign a negative number to the unsigned hue value
		hue = 360 + 60 * (g - b) / (rgbMax - rgbMin);

		if (hue > 359)
			hue -= 360;
	}
	else if (rgbMax == g)
	{
		hue = 120 + 60 * (b - r) / (rgbMax - rgbMin);
	}
	else
	{
		hue = 240 + 60 * (r - g) / (rgbMax - rgbMin);
	}
		
}

void HslTransform::hsl2rgb(uint16_t hue, float saturation, float luminance, uint8_t & red, uint8_t & green, uint8_t & blue)
{
	if (saturation == 0.0f)
	{
		red   = (uint8_t)(luminance * 255.0f);
		green = (uint8_t)(luminance * 255.0f);
		blue  = (uint8_t)(luminance * 255.0f);
		return;
	}

	float q = (luminance < 0.5f)
	        ? luminance * (1.0f + saturation)
	        : (luminance + saturation) - (luminance * saturation);

	float p = (2.0f * luminance) - q;	
	float h = hue / 360.0f;

	float t[3];
	
	t[0] = h + (1.0f / 3.0f);
	t[1] = h;
	t[2] = h - (1.0f / 3.0f);
	
	for (int i = 0; i < 3; i++)
	{
		if (t[i] < 0.0f)
			t[i] += 1.0f;
		if (t[i] > 1.0f)
			t[i] -= 1.0f;
	}
	
	float out[3];
	
	for (int i = 0; i < 3; i++)
	{
		if (t[i] * 6.0f < 1.0f)
			out[i] = p + (q - p) * 6.0f * t[i];
		else if (t[i] * 2.0f < 1.0f)
			out[i] = q;
		else if (t[i] * 3.0f < 2.0f)
			out[i] = p + (q - p) * ((2.0f / 3.0f) - t[i]) * 6.0f;
		else out[i] = p;
	}
	
	//convert back to 0...255 range
	red   = (uint8_t)(out[0] * 255.0f);
	green = (uint8_t)(out[1] * 255.0f);
	blue  = (uint8_t)(out[2] * 255.0f);

}
