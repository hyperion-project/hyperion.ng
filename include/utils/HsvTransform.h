#pragma once

// STL includes
#include <cstdint>

class HsvTransform
{
public:
	HsvTransform();
	HsvTransform(double saturationGain, double valueGain);
	~HsvTransform();

	void setSaturationGain(double saturationGain);
	double getSaturationGain() const;

	void setValueGain(double valueGain);
	double getValueGain() const;

	void transform(uint8_t & red, uint8_t & green, uint8_t & blue) const;

	/// integer version of the conversion are faster, but a little less accurate
	/// all values are unsigned 8 bit values and scaled between 0 and 255 except
	/// for the hue which is a 16 bit number and scaled between 0 and 360
	static void rgb2hsv(uint8_t red, uint8_t green, uint8_t blue, uint16_t & hue, uint8_t & saturation, uint8_t & value);
	static void hsv2rgb(uint16_t hue, uint8_t saturation, uint8_t value, uint8_t & red, uint8_t & green, uint8_t & blue);

private:
	double _saturationGain;
	double _valueGain;
};
