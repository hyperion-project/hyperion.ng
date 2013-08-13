#pragma once

// STL includes
#include <cstdint>

namespace hyperion
{

/// Transform for a single color byte value
///
/// Transforms are applied in the following order:
/// 1) a threshold is applied. All values below threshold will be set to zero
/// 2) gamma color correction is applied
/// 3) the output value is scaled from the [0:1] to [blacklevel:whitelevel]
/// 4) finally, in case of a weird choice of parameters, the output is clamped between [0:1]
///
/// All configuration values are doubles and assume the color value to be between 0 and 1
class ColorTransform
{
public:
	ColorTransform();
	ColorTransform(double threshold, double gamma, double blacklevel, double whitelevel);
	~ColorTransform();

	double getThreshold() const;
	void setThreshold(double threshold);

	double getGamma() const;
	void setGamma(double gamma);

	double getBlacklevel() const;
	void setBlacklevel(double blacklevel);

	double getWhitelevel() const;
	void setWhitelevel(double whitelevel);

	/// get the transformed value for the given byte value
	uint8_t transform(uint8_t input) const
	{
		return _mapping[input];
	}

private:
	void initializeMapping();

private:
	double _threshold;
	double _gamma;
	double _blacklevel;
	double _whitelevel;

	uint8_t _mapping[256];
};

} // end namespace hyperion
