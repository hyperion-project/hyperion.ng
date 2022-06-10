#include <algorithm>

#include <utils/OkhsvTransform.h>
#include <utils/ColorSys.h>

/// Clamps between 0.f and 1.f. Should generally be branchless
float clamp(float value)
{
	return std::max(0.f, std::min(value, 1.f));
}

OkhsvTransform::OkhsvTransform()
{
	_saturationGain = 1.0;
	_valueGain = 1.0;
}

OkhsvTransform::OkhsvTransform(float saturationGain, float valueGain)
{
	_saturationGain = saturationGain;
	_valueGain = valueGain;
}

float OkhsvTransform::getSaturationGain() const
{
	return _saturationGain;
}

void OkhsvTransform::setSaturationGain(float saturationGain)
{
	_saturationGain = saturationGain;
}

float OkhsvTransform::getValueGain() const
{
	return _valueGain;
}

void OkhsvTransform::setValueGain(float valueGain)
{
	_valueGain = valueGain;
}

bool OkhsvTransform::isIdentity() const
{
	return getSaturationGain() == 1.0f && getValueGain() == 1.0f;
}

void OkhsvTransform::transform(uint8_t & red, uint8_t & green, uint8_t & blue)
{
	float hue
		, saturation
		, value;
	ColorSys::rgb2okhsv(red, green, blue, hue, saturation, value);

	saturation = clamp(saturation * _saturationGain);
	value      = clamp(value      * _valueGain);

	ColorSys::okhsv2rgb(hue, saturation, value, red, green, blue);
}
