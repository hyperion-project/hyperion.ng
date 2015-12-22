#pragma once

// STL includes
#include <cstdint>

///
/// Color transformation to adjust the saturation and value of a RGB color value
///
class HslTransform
{
public:
	///
	/// Default constructor
	///
	HslTransform();

	///
	/// Constructor
	///
	/// @param saturationGain The used saturation gain
	/// @param luminanceGain The used luminance gain
	///
	HslTransform(double saturationGain, double luminanceGain);

	///
	/// Destructor
	///
	~HslTransform();

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
	/// Updates the luminance gain
	///
	/// @param luminanceGain New luminance gain
	///
	void setLuminanceGain(double luminanceGain);

	///
	/// Returns the luminance gain
	///
	/// @return The current luminance gain
	///
	double getLuminanceGain() const;

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
	///	Translates an RGB (red, green, blue) color to an HSL (hue, saturation, luminance) color
	///
	/// @param[in] red The red RGB-component
	/// @param[in] green The green RGB-component
	/// @param[in] blue The blue RGB-component
	/// @param[out] hue The hue HSL-component
	/// @param[out] saturation The saturation HSL-component
	/// @param[out] luminance The luminance HSL-component
	///
	/// @note Integer version of the conversion are faster, but a little less accurate all values
	/// are unsigned 8 bit values and scaled between 0 and 255 except for the hue which is a 16 bit
	/// number and scaled between 0 and 360
	///
	static void rgb2hsl(uint8_t red, uint8_t green, uint8_t blue, uint16_t & hue, float & saturation, float & luminance);

	///
	///	Translates an HSL (hue, saturation, luminance) color to an RGB (red, green, blue) color
	///
	/// @param[in] hue The hue HSV-component
	/// @param[in] saturation The saturation HSV-component
	/// @param[in] luminance The luminance HSL-component
	/// @param[out] red The red RGB-component
	/// @param[out] green The green RGB-component
	/// @param[out] blue The blue RGB-component
	///
	/// @note Integer version of the conversion are faster, but a little less accurate all values
	/// are unsigned 8 bit values and scaled between 0 and 255 except for the hue which is a 16 bit
	/// number and scaled between 0 and 360
	///
	static void hsl2rgb(uint16_t hue, float saturation, float luminance, uint8_t & red, uint8_t & green, uint8_t & blue);

private:
	/// The saturation gain
	double _saturationGain;
	/// The value gain
	double _luminanceGain;
	/// aux function
	float hue2rgb(float p, float q, float t);
};
