#include <utils/RgbTransform.h>

#include <QtGlobal>
#include <QtMath>

#include <utils/KelvinToRgb.h>

RgbTransform::RgbTransform()
    : RgbTransform::RgbTransform(1.0, 1.0, 1.0, 0.0, false, 100, 100, ColorTemperature::DEFAULT)
{
}

RgbTransform::RgbTransform(double gammaR, double gammaG, double gammaB, int backlightThreshold, bool backlightColored, uint8_t brightness, uint8_t brightnessCompensation, int temperature)
    : _brightness(brightness)
    , _brightnessCompensation(brightnessCompensation)
{
	init(gammaR, gammaG, gammaB, backlightThreshold, backlightColored, _brightness, _brightnessCompensation, temperature);
}

void RgbTransform::init(double gammaR, double gammaG, double gammaB, int backlightThreshold, bool backlightColored, uint8_t brightness, uint8_t brightnessCompensation, int temperature)
{
	_backLightEnabled = true;
	setGamma(gammaR, gammaG, gammaB);
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
		auto clampedValueR = static_cast<quint8>(qBound(0.0, gammaCorrectedValueR, static_cast<double>(UINT8_MAX)));
		auto clampedValueG = static_cast<quint8>(qBound(0.0, gammaCorrectedValueG, static_cast<double>(UINT8_MAX)));
		auto clampedValueB = static_cast<quint8>(qBound(0.0, gammaCorrectedValueB, static_cast<double>(UINT8_MAX)));

		// Assign clamped values to _mapping arrays
		_mappingR[i] = clampedValueR;
		_mappingG[i] = clampedValueG;
		_mappingB[i] = clampedValueB;
	}
}

int RgbTransform::getBacklightThreshold() const
{
	return _backlightThreshold;
}

void RgbTransform::setBacklightThreshold(int backlightThreshold)
{
	_backlightThreshold = backlightThreshold;

	// Normalize to [0,1]
	double t = qBound(0.0, _backlightThreshold, 100.0) / 100.0;

	// Exponential shaping (k=2). Curve: f(t)= (2^(k*t)-1)/(2^k-1)
	// Ensures full dynamic use of 0..100 -> 0..255
	const double k = 2.0;
	double shaped = (qPow(2.0, k * t) - 1.0) / (qPow(2.0, k) - 1.0);

	double floorVal = static_cast<double>(UINT8_MAX) * shaped;

	_brightnessLow = static_cast<uint8_t>(qBound(0.0, floorVal, static_cast<double>(UINT8_MAX)));
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
	double Fw = _brightnessCompensation * 2.0 / 100.0 + 1.0;
	double Fcmy = _brightnessCompensation / 100.0 + 1.0;

	_brightness_rgb = 0;
	_brightness_cmy = 0;
	_brightness_w = 0;

	if (_brightness > 0)
	{
		double B_in = (_brightness < 50) ? -0.09 * _brightness + 7.5 : -0.04 * _brightness + 5.0;

		// Ensure that the result is converted to an integer before assigning to uint8_t
		_brightness_rgb = static_cast<uint8_t>(std::ceil(qMin(static_cast<double>(UINT8_MAX), UINT8_MAX / B_in)));
		_brightness_cmy = static_cast<uint8_t>(std::ceil(qMin(static_cast<double>(UINT8_MAX), UINT8_MAX / (B_in * Fcmy))));
		_brightness_w = static_cast<uint8_t>(std::ceil(qMin(static_cast<double>(UINT8_MAX), UINT8_MAX / (B_in * Fw))));
	}
}

void RgbTransform::getBrightnessComponents(uint8_t& rgb, uint8_t& cmy, uint8_t& white) const
{
	rgb = _brightness_rgb;
	cmy = _brightness_cmy;
	white = _brightness_w;
}

void RgbTransform::applyGamma(uint8_t& red, uint8_t& green, uint8_t& blue) const
{
	// apply gamma
	red = _mappingR[red];
	green = _mappingG[green];
	blue = _mappingB[blue];
}

void RgbTransform::applyBacklight(uint8_t& red, uint8_t& green, uint8_t& blue) const
{
	int rgbSum = red + green + blue;
	if (_backLightEnabled && rgbSum < _brightnessLow * 3)
	{
		if (_backlightColored)
		{
			red = qMax(red, _brightnessLow);
			green = qMax(green, _brightnessLow);
			blue = qMax(blue, _brightnessLow);
		}
		else
		{
			red = _brightnessLow;
			green = _brightnessLow;
			blue = _brightnessLow;
		}
	}
}

uint8_t RgbTransform::getTurnOnThreshold() const
{
	return _turnOnThreshold;
}

void RgbTransform::setTurnOnThreshold(uint8_t threshold)
{
	_turnOnThreshold = threshold;
	if (_turnOffThreshold > _turnOnThreshold)
	{
		_turnOffThreshold = _turnOnThreshold;
	}
}

uint8_t RgbTransform::getTurnOffThreshold() const
{
	return _turnOffThreshold;
}

void RgbTransform::setTurnOffThreshold(uint8_t threshold)
{
	_turnOffThreshold = qMin(threshold, _turnOnThreshold);
}

void RgbTransform::applyHysteresis(ColorRgb& color, bool& isOn) const
{
	/// Apply a Rec. 709 weighted luminance hysteresis to the given color based on its current on/off state.
	const auto luminance = static_cast<uint8_t>(
	    (54U * color.red + 183U * color.green + 19U * color.blue + 128U) >> 8U);
	const uint8_t threshold = isOn ? _turnOffThreshold : _turnOnThreshold;
	if (luminance < threshold)
	{
		color.red = 0;
		color.green = 0;
		color.blue = 0;
		isOn = false;
	}
	else
	{
		isOn = true;
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
	color.red = color.red * _temperatureRGB.red / UINT8_MAX;
	color.green = color.green * _temperatureRGB.green / UINT8_MAX;
	color.blue = color.blue * _temperatureRGB.blue / UINT8_MAX;
}
