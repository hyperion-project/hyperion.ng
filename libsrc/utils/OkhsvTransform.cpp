#include <algorithm>

#include <utils/OkhsvTransform.h>
#include <utils/ColorSys.h>

/// Clamps between 0.f and 1.f. Should generally be branchless
double clamp(double value)
{
	return std::max(0.0, std::min(value, 1.0));
}

OkhsvTransform::OkhsvTransform()
{
	_saturationGain = 1.0;
	_valueGain = 1.0;
	_isIdentity = true;
}

OkhsvTransform::OkhsvTransform(double saturationGain, double valueGain)
{
	_saturationGain = saturationGain;
	_valueGain = valueGain;
	updateIsIdentity();
}

double OkhsvTransform::getSaturationGain() const
{
	return _saturationGain;
}

void OkhsvTransform::setSaturationGain(double saturationGain)
{
	_saturationGain = saturationGain;
	updateIsIdentity();
}

double OkhsvTransform::getValueGain() const
{
	return _valueGain;
}

void OkhsvTransform::setValueGain(double valueGain)
{
	_valueGain = valueGain;
	updateIsIdentity();
}

bool OkhsvTransform::isIdentity() const
{
	return _isIdentity;
}

void OkhsvTransform::transform(uint8_t & red, uint8_t & green, uint8_t & blue) const
{
	double hue;
	double saturation;
	double value;
	ColorSys::rgb2okhsv(red, green, blue, hue, saturation, value);

	saturation = clamp(saturation * _saturationGain);
	value      = clamp(value      * _valueGain);

	ColorSys::okhsv2rgb(hue, saturation, value, red, green, blue);
}

void OkhsvTransform::updateIsIdentity()
{
	_isIdentity = _saturationGain == 1.0 && _valueGain == 1.0;
}
