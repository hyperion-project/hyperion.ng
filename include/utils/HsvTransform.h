#pragma once

// STL includes
#include <cstdint>

///
/// Color transformation to adjust the saturation and value of a RGB color value
///
class HsvTransform
{
public:
	///
	/// Default constructor
	///
	HsvTransform();

	///
	/// Constructor
	///
	/// @param saturationGain The used saturation gain
	/// @param valueGain The used value gain
	///
	HsvTransform(double saturationGain, double valueGain);

	///
	/// Destructor
	///
	~HsvTransform();

	///
	/// Updates the saturation gain
	///
	/// @param saturationGain New saturationGain
	///
	void setSaturationGain(double saturationGain);

	///
	/// Returns the saturation gain
	///
	/// @return The current Saturation gain
	///
	double getSaturationGain() const;

	///
	/// Updates the value gain
	///
	/// @param valueGain New value gain
	///
	void setValueGain(double valueGain);

	///
	/// Returns the value gain
	///
	/// @return The current value gain
	///
	double getValueGain() const;

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

	///
	/// integer version of the conversion are faster, but a little less accurate
	/// all values are unsigned 8 bit values and scaled between 0 and 255 except
	/// for the hue which is a 16 bit number and scaled between 0 and 360
	///
	static void rgb2hsv(uint8_t red, uint8_t green, uint8_t blue, uint16_t & hue, uint8_t & saturation, uint8_t & value);
	static void hsv2rgb(uint16_t hue, uint8_t saturation, uint8_t value, uint8_t & red, uint8_t & green, uint8_t & blue);

private:
	/// The saturation gain
	double _saturationGain;
	/// The value gain
	double _valueGain;
};
