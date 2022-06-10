#pragma once

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
	/// @param valueGain gain factor to apply to value/brightness
	///
	OkhsvTransform(float saturationGain, float valueGain);

	/// @return The current saturation gain value
	float getSaturationGain() const;

	/// @param saturationGain new saturation gain
	void setSaturationGain(float saturationGain);

	/// @return The current value gain value
	float getValueGain() const;

	/// @param valueGain new value/brightness gain
	void setValueGain(float valueGain);

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
	void transform(uint8_t & red, uint8_t & green, uint8_t & blue);

private:
	float _saturationGain
		, _valueGain;
};
