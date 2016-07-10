// STL includes
#include <cmath>

// Utils includes
#include <utils/RgbChannelTransform.h>

RgbChannelTransform::RgbChannelTransform()
{
	setTransform(0.0, 1.0, 0.0, 1.0);
}

RgbChannelTransform::RgbChannelTransform(double threshold, double gamma, double blacklevel, double whitelevel)
{
	setTransform(threshold, gamma, blacklevel, whitelevel);
}

RgbChannelTransform::~RgbChannelTransform()
{
}

void RgbChannelTransform::setTransform(double threshold, double gamma, double blacklevel, double whitelevel)
{
	_threshold  = threshold;
	_gamma      = gamma;
	_blacklevel = blacklevel;
	_whitelevel = whitelevel;
	initializeMapping();
}

double RgbChannelTransform::getThreshold() const
{
	return _threshold;
}

void RgbChannelTransform::setThreshold(double threshold)
{
	setTransform(threshold, _gamma, _blacklevel, _whitelevel);
}

double RgbChannelTransform::getGamma() const
{
	return _gamma;
}

void RgbChannelTransform::setGamma(double gamma)
{
	setTransform(_threshold, gamma, _blacklevel, _whitelevel);
}

double RgbChannelTransform::getBlacklevel() const
{
	return _blacklevel;
}

void RgbChannelTransform::setBlacklevel(double blacklevel)
{
	setTransform(_threshold, _gamma, blacklevel, _whitelevel);
}

double RgbChannelTransform::getWhitelevel() const
{
	return _whitelevel;
}

void RgbChannelTransform::setWhitelevel(double whitelevel)
{
	setTransform(_threshold, _gamma, _blacklevel, whitelevel);
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
