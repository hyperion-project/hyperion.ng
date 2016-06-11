#pragma once

// STL includes
#include <cstdint>

///
/// Color transformation to adjust the saturation and luminance of a RGB color value
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
	HslTransform(double saturationGain, double luminanceGain, double luminanceMinimum);

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
	/// Updates the luminance minimum
	///
	/// @param luminanceMinimum New luminance minimum
	///
	void setLuminanceMinimum(double luminanceMinimum);

	///
	/// Returns the luminance minimum
	///
	/// @return The current luminance minimum
	///
	double getLuminanceMinimum() const;

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

	static void rgb2hsl(uint8_t red, uint8_t green, uint8_t blue, uint16_t & hue, float & saturation, float & luminance);

	///
	///	Translates an HSL (hue, saturation, luminance) color to an RGB (red, green, blue) color
	///
	/// @param[in] hue The hue HSL-component
	/// @param[in] saturation The saturation HSL-component
	/// @param[in] luminance The luminance HSL-component
	/// @param[out] red The red RGB-component
	/// @param[out] green The green RGB-component
	/// @param[out] blue The blue RGB-component
	///

	static void hsl2rgb(uint16_t hue, float saturation, float luminance, uint8_t & red, uint8_t & green, uint8_t & blue);

private:
	/// The saturation gain
	double _saturationGain;
	/// The luminance gain
	double _luminanceGain;
	/// The luminance minimum
	double _luminanceMinimum;
};
