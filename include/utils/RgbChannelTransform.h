#pragma once

// STL includes
#include <cstdint>

/// Transform for a single color byte value
///
/// Transforms are applied in the following order:
/// 1) a threshold is applied. All values below threshold will be set to zero
/// 2) gamma color correction is applied
/// 3) the output value is scaled from the [0:1] to [blacklevel:whitelevel]
/// 4) finally, in case of a weird choice of parameters, the output is clamped between [0:1]
///
/// All configuration values are doubles and assume the color value to be between 0 and 1
class RgbChannelTransform
{
public:
	/// Default constructor
	RgbChannelTransform();

	/// Constructor
	/// @param threshold  The minimum threshold
	/// @param gamma The gamma of the gamma-curve correction
	/// @param blacklevel The minimum value for the RGB-Channel
	/// @param whitelevel The maximum value for the RGB-Channel
	RgbChannelTransform(double threshold, double gamma, double blacklevel, double whitelevel);

	/// Destructor
	~RgbChannelTransform();

	/// @return The current threshold value
	double getThreshold() const;

	/// @param threshold New threshold value
	void setThreshold(double threshold);

	/// @return The current gamma value
	double getGamma() const;

	/// @param gamma New gamma value
	void setGamma(double gamma);

	/// @return The current blacklevel value
	double getBlacklevel() const;

	/// @param blacklevel New blacklevel value
	void setBlacklevel(double blacklevel);

	/// @return The current whitelevel value
	double getWhitelevel() const;

	/// @param whitelevel New whitelevel value
	void setWhitelevel(double whitelevel);

	/// Transform the given byte value
	/// @param input The input color byte
	/// @return The transformed byte value
	uint8_t transform(uint8_t input) const
	{
		return _mapping[input];
	}

private:
	/// (re)-initilize the color mapping
	void initializeMapping();

private:
	/// The threshold value
	double _threshold;
	/// The gamma value
	double _gamma;
	/// The blacklevel
	double _blacklevel;
	/// The whitelevel
	double _whitelevel;

	/// The mapping from input color to output color
	uint8_t _mapping[256];
};
