#pragma once

// STL includes
#include <cstdint>

///
/// Color transformation to adjust the saturation and luminance of a RGB color value
///
class ColorSys
{
public:
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
	///
	///	Translates an RGB (red, green, blue) color to an HSV (hue, saturation, value) color
	///
	/// @param[in] red The red RGB-component
	/// @param[in] green The green RGB-component
	/// @param[in] blue The blue RGB-component
	/// @param[out] hue The hue HSV-component
	/// @param[out] saturation The saturation HSV-component
	/// @param[out] value The value HSV-component
	///
	/// @note Integer version of the conversion are faster, but a little less accurate all values
	/// are unsigned 8 bit values and scaled between 0 and 255 except for the hue which is a 16 bit
	/// number and scaled between 0 and 360
	///
	static void rgb2hsv(uint8_t red, uint8_t green, uint8_t blue, uint16_t & hue, uint8_t & saturation, uint8_t & value);

	///
	///	Translates an HSV (hue, saturation, value) color to an RGB (red, green, blue) color
	///
	/// @param[in] hue The hue HSV-component
	/// @param[in] saturation The saturation HSV-component
	/// @param[in] value The value HSV-component
	/// @param[out] red The red RGB-component
	/// @param[out] green The green RGB-component
	/// @param[out] blue The blue RGB-component
	///
	/// @note Integer version of the conversion are faster, but a little less accurate all values
	/// are unsigned 8 bit values and scaled between 0 and 255 except for the hue which is a 16 bit
	/// number and scaled between 0 and 360
	///
	static void hsv2rgb(uint16_t hue, uint8_t saturation, uint8_t value, uint8_t & red, uint8_t & green, uint8_t & blue);
};
