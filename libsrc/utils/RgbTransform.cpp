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
	: _thresholdLow(std::min(std::max((int)(thresholdLow * 255), 0),255))
	, _thresholdHigh(std::min(std::max((int)(thresholdHigh * 255), 0),255))
	, _gammaR(gammaR)
	, _gammaG(gammaG)
	, _gammaB(gammaB)
{
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
	return _thresholdLow;
}

void RgbTransform::setThresholdLow(double threshold)
{
	_thresholdLow = std::min(std::max((int)(threshold * 255), 0),255);
}

double RgbTransform::getThresholdHigh() const
{
	return _thresholdHigh;
}

void RgbTransform::setThresholdHigh(double threshold)
{
	_thresholdHigh = std::min(std::max((int)(threshold * 255), 0),255);
}

void RgbTransform::transform(uint8_t & red, uint8_t & green, uint8_t & blue)
{
	// apply gamma
	red   = _mappingR[red];
	green = _mappingR[green];
	blue  = _mappingR[blue];

	// apply _thresholds
	if ( _thresholdLow > 0 && red<_thresholdLow && green<_thresholdLow && blue<_thresholdLow)
	{
		uint8_t delta = _thresholdLow  - std::max(red,std::max(green,blue));
		red   += delta;
		green += delta;
		blue  += delta;
	}
	else if ( _thresholdHigh<255 && (red>_thresholdHigh || green>_thresholdHigh || blue>_thresholdHigh))
	{
		uint8_t delta = _thresholdHigh  - std::max(red,std::max(green,blue));
		red   -= (red>=delta)  ?delta:0;
		green -= (green>=delta) ?delta:0;
		blue  -= (blue>=delta)  ?delta:0;
	}
}
