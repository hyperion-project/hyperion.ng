#ifndef OKHSVTRANSFORM_H
#define OKHSVTRANSFORM_H

#include <cstdint>

///
/// Color transformation to adjust the saturation and value of Okhsv colors
///
class OkhsvTransform
{
public:
	///
	/// Default constructor
	///
	OkhsvTransform();

	///
	/// Constructor
	///
	/// @param saturationGain gain factor to apply to saturation
	/// @param brightnessGain gain factor to apply to value/brightness
	///
	OkhsvTransform(double saturationGain, double brightnessGain);

	/// @return The current saturation gain value
	double getSaturationGain() const;

	/// @param saturationGain new saturation gain
	void setSaturationGain(double saturationGain);

	/// @return The current brightness gain value
	double getBrightnessGain() const;

	/// @param brightnessGain new value/brightness gain
	void setBrightnessGain(double brightnessGain);

	/// @return true if the current gain settings result in an identity transformation
	bool isIdentity() const;

	///
	/// Apply the transform the the given RGB values.
	///
	/// @param red The red color component
	/// @param green The green color component
	/// @param blue The blue color component
	///
	/// @note The values are updated in place.
	///
	void transform(uint8_t & red, uint8_t & green, uint8_t & blue) const;

private:
	/// Sets _isIdentity to true if both gain values are at their neutral setting
	void updateIsIdentity();

	/// Gain settings
	double _saturationGain;
	double _brightnessGain;

	/// Is true if the gain settings result in an identity transformation
	bool _isIdentity;
};

#endif // OKHSVTRANSFORM_H
