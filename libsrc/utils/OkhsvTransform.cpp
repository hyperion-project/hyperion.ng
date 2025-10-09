#include <algorithm>

#include <utils/OkhsvTransform.h>
#include <utils/ColorSys.h>

/// Clamps between 0.f and 1.f. Should generally be branchless
inline double clamp(double value)
{
	return std::max(0.0, std::min(value, 1.0));
}

OkhsvTransform::OkhsvTransform()
{
	_saturationGain = 1.0;
	_brightnessGain = 1.0;
	_isIdentity = true;
}

OkhsvTransform::OkhsvTransform(double saturationGain, double brightnessGain)
{
	_saturationGain = saturationGain;
	_brightnessGain = brightnessGain;
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

double OkhsvTransform::getBrightnessGain() const
{
	return _brightnessGain;
}

void OkhsvTransform::setBrightnessGain(double brightnessGain)
{
	_brightnessGain= brightnessGain;
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
	double brightness;
	ColorSys::rgb2okhsv(red, green, blue, hue, saturation, brightness);

	saturation = clamp(saturation * _saturationGain);
	brightness = clamp(brightness * _brightnessGain);

	ColorSys::okhsv2rgb(hue, saturation, brightness, red, green, blue);
}

void OkhsvTransform::updateIsIdentity()
{
	_isIdentity = _saturationGain == 1.0 && _brightnessGain == 1.0;
}
