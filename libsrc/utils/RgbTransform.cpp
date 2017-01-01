#include <iostream>
#include <cmath>

#include <utils/RgbTransform.h>

RgbTransform::RgbTransform()
	: _thresholdLow(0)
	, _thresholdHigh(255)
	, _gammaR(1.0)
	, _gammaG(1.0)
	, _gammaB(1.0)
{
	initializeMapping();
}

RgbTransform::RgbTransform(double gammaR, double gammaG, double gammaB, double thresholdLow, double thresholdHigh)
	: _gammaR(gammaR)
	, _gammaG(gammaG)
	, _gammaB(gammaB)
{
	setThresholdLow(thresholdLow);
	setThresholdHigh(thresholdHigh);
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


double RgbTransform::getThresholdLow() const
{
	return _thresholdLowF;
}

void RgbTransform::setThresholdLow(double threshold)
{
	_thresholdLowF    = threshold;
	_sumThresholdLowF = 765.0 * threshold;
	_thresholdLow     = std::min(std::max((int)(threshold * 255), 0),255);
}

double RgbTransform::getThresholdHigh() const
{
	return _thresholdHigh;
}

void RgbTransform::setThresholdHigh(double threshold)
{
	_thresholdHighF    = threshold;
	_sumThresholdHighF = 765.0 * threshold;
	_thresholdHigh     = std::min(std::max((int)(threshold * 255), 0),255);
}

void RgbTransform::transform(uint8_t & red, uint8_t & green, uint8_t & blue)
{
	// apply gamma
	red   = _mappingR[red];
	green = _mappingR[green];
	blue  = _mappingR[blue];

	//std::cout << (int)red << " " << (int)green << " " << (int)blue << " => ";
	// apply _thresholds
	if (red  ==0) red   = 1;
	if (green==0) green = 1;
	if (blue ==0) blue  = 1;

	int rgbSum = red+green+blue;

	if (rgbSum > _sumThresholdHighF)
	{
		double cH = _sumThresholdHighF / rgbSum;
		red   *= cH;
		green *= cH;
		blue  *= cH;
	}
	else if (rgbSum < _sumThresholdLowF)
	{
		double cL = _sumThresholdLowF / rgbSum;
		red   *= cL;
		green *= cL;
		blue  *= cL;
	}
	//std::cout << (int)red << " " << (int)green << " " << (int)blue << std::endl;
}
