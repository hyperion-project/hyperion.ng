#include <QtCore/qmath.h>
#include <utils/RgbTransform.h>

RgbTransform::RgbTransform()
{
	init(1.0, 1.0, 1.0, 0.0, false, 100, 100);
}

RgbTransform::RgbTransform(double gammaR, double gammaG, double gammaB, double backlightThreshold, bool backlightColored, uint8_t brightness, uint8_t brightnessCompensation)
{
	init(gammaR, gammaG, gammaB, backlightThreshold, backlightColored, brightness, brightnessCompensation);
}

void RgbTransform::init(double gammaR, double gammaG, double gammaB, double backlightThreshold, bool backlightColored, uint8_t brightness, uint8_t brightnessCompensation)
{
	_backLightEnabled = true;
	setGamma(gammaR,gammaG,gammaB);
	setBacklightThreshold(backlightThreshold);
	setBacklightColored(backlightColored);
	setBrightness(brightness);
	setBrightnessCompensation(brightnessCompensation);
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
	for (int i = 0; i < 256; ++i)
	{
		_mappingR[i] = qMin(qMax((int)(qPow(i / 255.0, _gammaR) * 255), 0), 255);
		_mappingG[i] = qMin(qMax((int)(qPow(i / 255.0, _gammaG) * 255), 0), 255);
		_mappingB[i] = qMin(qMax((int)(qPow(i / 255.0, _gammaB) * 255), 0), 255);
	}
}


int RgbTransform::getBacklightThreshold() const
{
	return _backlightThreshold;
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

	double B_in= 0;
	_brightness_rgb = 0;
	_brightness_cmy = 0;
	_brightness_w   = 0;

	if (_brightness > 0)
	{
		B_in = (_brightness<50)? -0.09*_brightness+7.5 : -0.04*_brightness+5.0;

		_brightness_rgb = std::ceil(qMin(255.0,255.0/B_in));
		_brightness_cmy = std::ceil(qMin(255.0,255.0/(B_in*Fcmy)));
		_brightness_w   = std::ceil(qMin(255.0,255.0/(B_in*Fw)));
	}
}

void RgbTransform::getBrightnessComponents(uint8_t & rgb, uint8_t & cmy, uint8_t & w) const
{
	rgb = _brightness_rgb;
	cmy = _brightness_cmy;
	w   = _brightness_w;
}

void RgbTransform::transform(uint8_t & red, uint8_t & green, uint8_t & blue)
{
	// apply gamma
	red   = _mappingR[red];
	green = _mappingG[green];
	blue  = _mappingB[blue];

	// apply brightnesss
	int rgbSum = red+green+blue;

	if ( _backLightEnabled && _sumBrightnessLow>0 && rgbSum < _sumBrightnessLow)
	{
		if (_backlightColored)
		{
			if (rgbSum == 0)
			{
				if (red  ==0) red   = 1;
				if (green==0) green = 1;
				if (blue ==0) blue  = 1;
				rgbSum = red+green+blue;
			}
			double cL =qMin((int)(_sumBrightnessLow /rgbSum), 255);

			red   *= cL;
			green *= cL;
			blue  *= cL;
		}
		else
		{
			red   = qMin((int)(_sumBrightnessLow/3.0), 255);
			green = red;
			blue  = red;
		}
	}
}
