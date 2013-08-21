#pragma once

#include <cstdint>

namespace hyperion
{

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

	private:
		// integer version of the conversion are faster, but a little less accurate
		static void rgb2hsv(uint8_t red, uint8_t green, uint8_t blue, uint8_t & hue, uint8_t & saturation, uint8_t & value);
		static void hsv2rgb(uint8_t hue, uint8_t saturation, uint8_t value, uint8_t & red, uint8_t & green, uint8_t & blue);

	private:
		double _saturationGain;
		double _valueGain;
	};

} // namespace hyperion
