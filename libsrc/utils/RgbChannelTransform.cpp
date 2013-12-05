// STL includes
#include <cmath>

// Utils includes
#include <utils/RgbChannelTransform.h>

RgbChannelTransform::RgbChannelTransform() :
	_threshold(0),
	_gamma(1.0),
	_blacklevel(0.0),
	_whitelevel(1.0)
{
	initializeMapping();
}

RgbChannelTransform::RgbChannelTransform(double threshold, double gamma, double blacklevel, double whitelevel) :
	_threshold(threshold),
	_gamma(gamma),
	_blacklevel(blacklevel),
	_whitelevel(whitelevel)
{
	initializeMapping();
}

RgbChannelTransform::~RgbChannelTransform()
{
}

double RgbChannelTransform::getThreshold() const
{
	return _threshold;
}

void RgbChannelTransform::setThreshold(double threshold)
{
	_threshold = threshold;
	initializeMapping();
}

double RgbChannelTransform::getGamma() const
{
	return _gamma;
}

void RgbChannelTransform::setGamma(double gamma)
{
	_gamma = gamma;
	initializeMapping();
}

double RgbChannelTransform::getBlacklevel() const
{
	return _blacklevel;
}

void RgbChannelTransform::setBlacklevel(double blacklevel)
{
	_blacklevel = blacklevel;
	initializeMapping();
}

double RgbChannelTransform::getWhitelevel() const
{
	return _whitelevel;
}

void RgbChannelTransform::setWhitelevel(double whitelevel)
{
	_whitelevel = whitelevel;
	initializeMapping();
}

void RgbChannelTransform::initializeMapping()
{
	// initialize the mapping as a linear array
	for (int i = 0; i < 256; ++i)
	{
		double output = i / 255.0;

		// apply linear transform
		if (output < _threshold)
		{
			output = 0.0;
		}

		// apply gamma correction
		output = std::pow(output, _gamma);

		// apply blacklevel and whitelevel
		output = _blacklevel + (_whitelevel - _blacklevel) * output;

		// calc mapping
		int mappingValue = output * 255;
		if (mappingValue < 0)
		{
			mappingValue = 0;
		}
		else if (mappingValue > 255)
		{
			mappingValue = 255;
		}
		_mapping[i] = mappingValue;
	}
}
