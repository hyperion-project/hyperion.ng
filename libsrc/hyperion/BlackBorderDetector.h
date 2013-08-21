
#pragma once

// Utils includes
#include <utils/RgbImage.h>

struct BlackBorder
{
	enum Type
	{
		none,
		horizontal,
		vertical,
		unknown
	};

	Type type;
	int size;

};

class BlackBorderDetector
{
public:
	BlackBorderDetector();

	BlackBorder process(const RgbImage& image);


private:

	inline bool isBlack(const RgbColor& color)
	{
		return RgbColor::BLACK == color;
	}


};
