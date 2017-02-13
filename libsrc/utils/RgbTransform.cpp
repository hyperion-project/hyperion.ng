#include <iostream>
#include <cmath>

#include <utils/RgbTransform.h>

RgbTransform::RgbTransform()
{
	init(1.0, 1.0, 1.0, 0.0, false, 1.0);
}

RgbTransform::RgbTransform(double gammaR, double gammaG, double gammaB, double backlightThreshold, bool backlightColored, double brightnessHigh)
{
	init(gammaR, gammaG, gammaB, backlightThreshold, backlightColored, brightnessHigh);
}

void RgbTransform::init(double gammaR, double gammaG, double gammaB, double backlightThreshold, bool backlightColored, double brightnessHigh)
{
	_backLightEnabled = true;
	setGamma(gammaR,gammaG,gammaB);
	setBacklightThreshold(backlightThreshold);
	setBacklightColored(backlightColored);
	setBrightness(brightnessHigh);
	initializeMapping();
}

RgbTransform::~RgbTransform()
{
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
		_mappingR[i] = std::min(std::max((int)(std::pow(i / 255.0, _gammaR) * 255), 0), 255);
		_mappingG[i] = std::min(std::max((int)(std::pow(i / 255.0, _gammaG) * 255), 0), 255);
		_mappingB[i] = std::min(std::max((int)(std::pow(i / 255.0, _gammaB) * 255), 0), 255);
	}
}


double RgbTransform::getBacklightThreshold() const
{
	return _backlightThreshold;
}

void RgbTransform::setBacklightThreshold(double backlightThreshold)
{
	_backlightThreshold = backlightThreshold;
	_sumBrightnessLow   = 765.0 * ((std::pow(2.0,backlightThreshold*2)-1) / 3.0);
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

double RgbTransform::getBrightness() const
{
	return _brightnessHigh;
}

void RgbTransform::setBrightness(double brightness)
{
	_brightnessHigh    = brightness;
	_sumBrightnessHigh = 765.0 * ((std::pow(2.0,brightness*2)-1) / 3.0);
}

void RgbTransform::transform(uint8_t & red, uint8_t & green, uint8_t & blue)
{
	// apply gamma
	red   = _mappingR[red];
	green = _mappingR[green];
	blue  = _mappingR[blue];

	//std::cout << (int)red << " " << (int)green << " " << (int)blue << " => ";
	// apply brightnesss
	int rgbSum = red+green+blue;

	if (_sumBrightnessHigh > 0 && rgbSum > _sumBrightnessHigh)
	{
		double cH = _sumBrightnessHigh / rgbSum;
		red   *= cH;
		green *= cH;
		blue  *= cH;
	}
	else if ( _backLightEnabled && _sumBrightnessLow>0 && rgbSum < _sumBrightnessLow)
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
			double cL =std::min((int)(_sumBrightnessLow /rgbSum), 255);

			red   *= cL;
			green *= cL;
			blue  *= cL;
		}
		else
		{
			red   = std::min((int)(_sumBrightnessLow/3.0), 255);
			green = red;
			blue  = red;
		}
	}
	//std::cout << _sumBrightnessLow << " " << (int)red << " " << (int)green << " " << (int)blue << std::endl;
}
