#include <iostream>
#include <cmath>

#include <utils/RgbTransform.h>

RgbTransform::RgbTransform()
	: _brightnessLow(0)
	, _brightnessHigh(255)
	, _gammaR(1.0)
	, _gammaG(1.0)
	, _gammaB(1.0)
{
	initializeMapping();
}

RgbTransform::RgbTransform(double gammaR, double gammaG, double gammaB, double brightnessLow, double brightnessHigh)
	: _gammaR(gammaR)
	, _gammaG(gammaG)
	, _gammaB(gammaB)
{
	setBrightnessMin(brightnessLow);
	setBrightness(brightnessHigh);
	initializeMapping();
}

RgbTransform::~RgbTransform()
{
}

double RgbTransform::getGammaR() const
{
	return _gammaB;
}

double RgbTransform::getGammaG() const
{
	return _gammaB;
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


double RgbTransform::getBrightnessMin() const
{
	return _brightnessLowF;
}

void RgbTransform::setBrightnessMin(double brightness)
{
	_brightnessLowF    = brightness;
	_sumBrightnessLowF = 765.0 * brightness;
	_brightnessLow     = std::min(std::max((int)(brightness * 255), 0),255);
}

double RgbTransform::getBrightness() const
{
	return _brightnessHigh;
}

void RgbTransform::setBrightness(double brightness)
{
	_brightnessHighF    = brightness;
	_sumBrightnessHighF = 765.0 * brightness;
	_brightnessHigh     = std::min(std::max((int)(brightness * 255), 0),255);
}

void RgbTransform::transform(uint8_t & red, uint8_t & green, uint8_t & blue)
{
	// apply gamma
	red   = _mappingR[red];
	green = _mappingR[green];
	blue  = _mappingR[blue];

	//std::cout << (int)red << " " << (int)green << " " << (int)blue << " => ";
	// apply brightnesss
	if (red  ==0) red   = 1;
	if (green==0) green = 1;
	if (blue ==0) blue  = 1;

	int rgbSum = red+green+blue;

	if (rgbSum > _sumBrightnessHighF)
	{
		double cH = _sumBrightnessHighF / rgbSum;
		red   *= cH;
		green *= cH;
		blue  *= cH;
	}
	else if (rgbSum < _sumBrightnessLowF)
	{
		double cL = _sumBrightnessLowF / rgbSum;
		red   *= cL;
		green *= cL;
		blue  *= cL;
	}
	//std::cout << (int)red << " " << (int)green << " " << (int)blue << std::endl;
}
