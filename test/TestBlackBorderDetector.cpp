
// STL includes
#include <random>

// Hyperion includes
#include "hyperion/BlackBorderDetector.h"

RgbColor randomColor()
{
	const uint8_t randomRedValue   = uint8_t(rand() % (std::numeric_limits<uint8_t>::max() + 1));
	const uint8_t randomGreenValue = uint8_t(rand() % (std::numeric_limits<uint8_t>::max() + 1));
	const uint8_t randomBlueValue  = uint8_t(rand() % (std::numeric_limits<uint8_t>::max() + 1));

	return {randomRedValue, randomGreenValue, randomBlueValue};
}

RgbImage createImage(unsigned width, unsigned height, unsigned topBorder, unsigned leftBorder)
{
	RgbImage image(width, height);
	for (unsigned x=0; x<image.width(); ++x)
	{
		for (unsigned y=0; y<image.height(); ++y)
		{
			if (y < topBorder || x < leftBorder)
			{
				image(x,y) = RgbColor::BLACK;
			}
			else
			{
				image(x,y) = randomColor();
			}
		}
	}
	return image;
}

int TC_NO_BORDER()
{
	int result = 0;

	BlackBorderDetector detector;

	{
		RgbImage image = createImage(64, 64, 0, 0);
		BlackBorder border = detector.process(image);
		if (border.type != BlackBorder::none)
		{
			std::cerr << "Failed to correctly detect no border" << std::endl;
			result = -1;
		}
	}

	return result;
}

int TC_TOP_BORDER()
{
	int result = 0;

	BlackBorderDetector detector;

	{
		RgbImage image = createImage(64, 64, 12, 0);
		BlackBorder border = detector.process(image);
		if (border.type != BlackBorder::horizontal || border.size != 12)
		{
			std::cerr << "Failed to correctly detect horizontal border with correct size" << std::endl;
			result = -1;
		}
	}

	return result;
}

int TC_LEFT_BORDER()
{
	int result = 0;

	BlackBorderDetector detector;

	{
		RgbImage image = createImage(64, 64, 0, 12);
		BlackBorder border = detector.process(image);
		if (border.type != BlackBorder::vertical || border.size != 12)
		{
			std::cerr << "Failed to detected vertical border with correct size" << std::endl;
			result = -1;
		}
	}

	return result;
}

int TC_UNKNOWN_BORDER()
{
	int result = 0;

	BlackBorderDetector detector;

	{
		RgbImage image = createImage(64, 64, 12, 12);
		BlackBorder border = detector.process(image);
		if (border.type != BlackBorder::unknown)
		{
			std::cerr << "Failed to detected unknown border" << std::endl;
			result = -1;
		}
	}
	return result;
}

int main()
{
	TC_NO_BORDER();
	TC_TOP_BORDER();
	TC_LEFT_BORDER();
	TC_UNKNOWN_BORDER();

	return 0;
}
