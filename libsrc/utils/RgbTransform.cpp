#include <QtCore/qmath.h>
#include <utils/RgbTransform.h>
#include <utils/KelvinToRgb.h>

#include<QDebug>

RgbTransform::RgbTransform()
	: RgbTransform::RgbTransform(1.0, 1.0, 1.0, 0.0, false, 100, 100, 6600)
{
}

RgbTransform::RgbTransform(double gammaR, double gammaG, double gammaB, double backlightThreshold, bool backlightColored, uint8_t brightness, uint8_t brightnessCompensation, int temperature)
	: _brightness(brightness)
	, _brightnessCompensation(brightnessCompensation)
{
	init(gammaR, gammaG, gammaB, backlightThreshold, backlightColored, _brightness, _brightnessCompensation, temperature);
}

void RgbTransform::init(double gammaR, double gammaG, double gammaB, double backlightThreshold, bool backlightColored, uint8_t brightness, uint8_t brightnessCompensation, int temperature)
{
	_backLightEnabled = true;
	setGamma(gammaR,gammaG,gammaB);
	setBacklightThreshold(backlightThreshold);
	setBacklightColored(backlightColored);
	setBrightness(brightness);
	setBrightnessCompensation(brightnessCompensation);
	setTemperature(temperature);
	initializeMapping();
}

double RgbTransform::getGammaR() const
{
	return _gammaR;
}

double RgbTransform::getGammaG() const
{
	return _gammaG;
}

double RgbTransform::getGammaB() const
{
	return _gammaB;
}

void RgbTransform::setGamma(double gammaR, double gammaG, double gammaB)
{
	_gammaR = gammaR;
	_gammaG = (gammaG < 0.0) ? _gammaR : gammaG;
	_gammaB = (gammaB < 0.0) ? _gammaR : gammaB;
	initializeMapping();
}

void RgbTransform::initializeMapping()
{
	for (int i = 0; i <= UINT8_MAX; ++i)
	{
		// Calculate normalized value
		double normalizedValueR = static_cast<double>(i) / UINT8_MAX;
		double normalizedValueG = static_cast<double>(i) / UINT8_MAX;
		double normalizedValueB = static_cast<double>(i) / UINT8_MAX;

		// Apply gamma correction
		double gammaCorrectedValueR = qPow(normalizedValueR, _gammaR) * UINT8_MAX;
		double gammaCorrectedValueG = qPow(normalizedValueG, _gammaG) * UINT8_MAX;
		double gammaCorrectedValueB = qPow(normalizedValueB, _gammaB) * UINT8_MAX;

		// Clamp values to valid range [0, UINT8_MAX]
		quint8 clampedValueR = static_cast<quint8>(qMin(qMax(gammaCorrectedValueR, 0.0), static_cast<double>(UINT8_MAX)));
		quint8 clampedValueG = static_cast<quint8>(qMin(qMax(gammaCorrectedValueG, 0.0), static_cast<double>(UINT8_MAX)));
		quint8 clampedValueB = static_cast<quint8>(qMin(qMax(gammaCorrectedValueB, 0.0), static_cast<double>(UINT8_MAX)));

		// Assign clamped values to _mapping arrays
		_mappingR[i] = clampedValueR;
		_mappingG[i] = clampedValueG;
		_mappingB[i] = clampedValueB;
	}
}


int RgbTransform::getBacklightThreshold() const
{
	return static_cast<int>(_backlightThreshold);
}

void RgbTransform::setBacklightThreshold(double backlightThreshold)
{
	_backlightThreshold = backlightThreshold;
	_sumBrightnessLow   = 765.0 * ((qPow(2.0,(_backlightThreshold/100)*2)-1) / 3.0);
}

bool RgbTransform::getBacklightColored() const
{
	return _backlightColored;
}

void RgbTransform::setBacklightColored(bool backlightColored)
{
	_backlightColored = backlightColored;
}

bool RgbTransform::getBackLightEnabled() const
{
	return _backLightEnabled;
}

void RgbTransform::setBackLightEnabled(bool enable)
{
	_backLightEnabled = enable;
}

uint8_t RgbTransform::getBrightness() const
{
	return _brightness;
}

void RgbTransform::setBrightness(uint8_t brightness)
{
	_brightness = brightness;
	updateBrightnessComponents();
}

void RgbTransform::setBrightnessCompensation(uint8_t brightnessCompensation)
{
	_brightnessCompensation = brightnessCompensation;
	updateBrightnessComponents();
}

uint8_t RgbTransform::getBrightnessCompensation() const
{
	return _brightnessCompensation;
}

void RgbTransform::updateBrightnessComponents()
{
	double Fw   = _brightnessCompensation*2.0/100.0+1.0;
	double Fcmy = _brightnessCompensation/100.0+1.0;

	_brightness_rgb = 0;
	_brightness_cmy = 0;
	_brightness_w   = 0;

	if (_brightness > 0)
	{
		double B_in = (_brightness < 50) ? -0.09 * _brightness + 7.5 : -0.04 * _brightness + 5.0;

		// Ensure that the result is converted to an integer before assigning to uint8_t
		_brightness_rgb = static_cast<uint8_t>(std::ceil(qMin(static_cast<double>(UINT8_MAX), UINT8_MAX / B_in)));
		_brightness_cmy = static_cast<uint8_t>(std::ceil(qMin(static_cast<double>(UINT8_MAX), UINT8_MAX / (B_in * Fcmy))));
		_brightness_w   = static_cast<uint8_t>(std::ceil(qMin(static_cast<double>(UINT8_MAX), UINT8_MAX / (B_in * Fw))));
	}
}

void RgbTransform::getBrightnessComponents(uint8_t & rgb, uint8_t & cmy, uint8_t & white) const
{
	rgb = _brightness_rgb;
	cmy = _brightness_cmy;
	white   = _brightness_w;
}

void RgbTransform::applyGamma(uint8_t & red, uint8_t & green, uint8_t & blue)
{
	// apply gamma
	red   = _mappingR[red];
	green = _mappingG[green];
	blue  = _mappingB[blue];
}

void RgbTransform::applyBacklight(uint8_t & red, uint8_t & green, uint8_t & blue) const
{
	// apply brightnesss
	int rgbSum = red+green+blue;

	if ( _backLightEnabled && _sumBrightnessLow > 0 && rgbSum < _sumBrightnessLow)
	{
		if (_backlightColored)
		{
			if (rgbSum == 0)
			{
				if (red  ==0) { red   = 1; }
				if (green==0) { green = 1; }
				if (blue ==0) { blue  = 1; }
				rgbSum = red+green+blue;
			}

			uint8_t cLow = static_cast<uint8_t>(qMin(static_cast<double>(_sumBrightnessLow/rgbSum), static_cast<double>(UINT8_MAX)));
			red   *= cLow;
			green *= cLow;
			blue  *= cLow;
		}
		else
		{
			red   = static_cast<uint8_t>(qMin(static_cast<double>(_sumBrightnessLow/3.0), static_cast<double>(UINT8_MAX)));
			green = red;
			blue  = red;
		}
	}
}

void RgbTransform::setTemperature(int temperature)
{
	_temperature = temperature;
	_temperatureRGB = getRgbFromTemperature(_temperature);
}

int RgbTransform::getTemperature() const
{
	return _temperature;
}

void RgbTransform::applyTemperature(ColorRgb& color) const
{
	color.red   = color.red * _temperatureRGB.red / UINT8_MAX;
	color.green = color.green * _temperatureRGB.green / UINT8_MAX;
	color.blue  = color.blue * _temperatureRGB.blue / UINT8_MAX;
}
