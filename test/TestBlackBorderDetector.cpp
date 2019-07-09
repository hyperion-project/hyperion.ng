
// STL includes
#include <random>

// Hyperion includes
#include <utils/ColorRgb.h>

// Blackborder includes
#include <blackborder/BlackBorderDetector.h>

using namespace hyperion;

ColorRgb randomColor()
{
	const uint8_t randomRedValue   = uint8_t(rand() % (std::numeric_limits<uint8_t>::max() + 1));
	const uint8_t randomGreenValue = uint8_t(rand() % (std::numeric_limits<uint8_t>::max() + 1));
	const uint8_t randomBlueValue  = uint8_t(rand() % (std::numeric_limits<uint8_t>::max() + 1));

	return {randomRedValue, randomGreenValue, randomBlueValue};
}

Image<ColorRgb> createImage(unsigned width, unsigned height, unsigned topBorder, unsigned leftBorder)
{
	Image<ColorRgb> image(width, height);
	for (unsigned x=0; x<image.width(); ++x)
	{
		for (unsigned y=0; y<image.height(); ++y)
		{
			if (y < topBorder || x < leftBorder)
			{
				image(x,y) = ColorRgb::BLACK;
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

	BlackBorderDetector detector(3);

	{
		Image<ColorRgb> image = createImage(64, 64, 0, 0);
		BlackBorder border = detector.process(image);
		if (border.unknown != false && border.horizontalSize != 0 && border.verticalSize != 0)
		{
			std::cerr << "Failed to correctly detect no border" << std::endl;
			result = -1;
		}
		else std::cout << "Correctly detected no border" << std::endl;
	}

	return result;
}

int TC_TOP_BORDER()
{
	int result = 0;

	BlackBorderDetector detector(3);

	{
		Image<ColorRgb> image = createImage(64, 64, 12, 0);
		BlackBorder border = detector.process(image);
		if (border.unknown != false && border.horizontalSize == 12 && border.verticalSize != 0)
		{
			std::cerr << "Failed to correctly detect horizontal border with correct size" << std::endl;
			result = -1;
		}
		else std::cout << "Correctly detected horizontal border with correct size" << std::endl;
	}

	return result;
}

int TC_LEFT_BORDER()
{
	int result = 0;

	BlackBorderDetector detector(3);

	{
		Image<ColorRgb> image = createImage(64, 64, 0, 12);
		BlackBorder border = detector.process(image);
		if (border.unknown != false && border.horizontalSize != 0 && border.verticalSize == 12)
		{
			std::cerr << "Failed to correctly detect vertical border with correct size" << std::endl;
			result = -1;
		}
		else std::cout << "Correctly detected vertical border with correct size" << std::endl;
	}

	return result;
}

int TC_DUAL_BORDER()
{
	int result = 0;

	BlackBorderDetector detector(3);

	{
		Image<ColorRgb> image = createImage(64, 64, 12, 12);
		BlackBorder border = detector.process(image);
		if (border.unknown != false && border.horizontalSize == 12 && border.verticalSize == 12)
		{
			std::cerr << "Failed to correctly detect two-sided border" << std::endl;
			result = -1;
		}
		else std::cout << "Correctly detected two-sided border" << std::endl;
	}
	return result;
}

int TC_UNKNOWN_BORDER()
{
	int result = 0;

	BlackBorderDetector detector(3);

	{
		Image<ColorRgb> image = createImage(64, 64, 30, 30);
		BlackBorder border = detector.process(image);
		if (border.unknown != true)
		{
			std::cerr << "Failed to correctly detect unknown border" << std::endl;
			result = -1;
		}
		else std::cout << "Correctly detected unknown border" << std::endl;
	}
	return result;
}

int main()
{
	TC_NO_BORDER();
	TC_TOP_BORDER();
	TC_LEFT_BORDER();
	TC_DUAL_BORDER();
	TC_UNKNOWN_BORDER();

	return 0;
}
