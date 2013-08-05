#include <cmath>

#include "ColorTransform.h"

ColorTransform::ColorTransform() :
    _threshold(0),
    _gamma(1.0),
    _blacklevel(0.0),
    _whitelevel(1.0)
{
    initializeMapping();
}

ColorTransform::ColorTransform(double threshold, double gamma, double blacklevel, double whitelevel) :
    _threshold(threshold),
    _gamma(gamma),
    _blacklevel(blacklevel),
    _whitelevel(whitelevel)
{
    initializeMapping();
}

ColorTransform::~ColorTransform()
{
}

double ColorTransform::getThreshold() const
{
    return _threshold;
}

void ColorTransform::setThreshold(double threshold)
{
    _threshold = threshold;
    initializeMapping();
}

double ColorTransform::getGamma() const
{
    return _gamma;
}

void ColorTransform::setGamma(double gamma)
{
    _gamma = gamma;
    initializeMapping();
}

double ColorTransform::getBlacklevel() const
{
    return _blacklevel;
}

void ColorTransform::setBlacklevel(double blacklevel)
{
    _blacklevel = blacklevel;
    initializeMapping();
}

double ColorTransform::getWhitelevel() const
{
    return _whitelevel;
}

void ColorTransform::setWhitelevel(double whitelevel)
{
    _whitelevel = whitelevel;
    initializeMapping();
}

void ColorTransform::initializeMapping()
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
