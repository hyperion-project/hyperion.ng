
// STL includes
#include <cassert>
#include <random>
#include <iostream>

// Utils includes
#include <utils/Image.h>
#include <utils/ColorRgb.h>

// Blackborder includes
#include "blackborder/BlackBorderProcessor.h"

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
			if (y < topBorder || y > ( height - topBorder ) || x < leftBorder || x > (width - leftBorder) )
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

int main()
{
//	unsigned unknownCnt = 600;
	unsigned borderCnt  = 50;
//	unsigned blurCnt    = 0;
	QJsonObject config;

//	BlackBorderProcessor processor(unknownCnt, borderCnt, blurCnt, 3, config);
	BlackBorderProcessor processor(config);

	// Start with 'no border' detection
	Image<ColorRgb> noBorderImage = createImage(64, 64, 0, 0);
	for (unsigned i=0; i<10; ++i)
	{
		bool newBorder = processor.process(noBorderImage);
		if (i == 0)
		{
			// Switch to 'no border' should immediate
			if (!newBorder)
			{
				std::cerr << "Failed to detect 'no border' when required" << std::endl;
				exit(EXIT_FAILURE);
			}
		}
		else
		{
			if (newBorder)
			{
				std::cerr << "Incorrectly detected new border, when there in none" << std::endl;
				exit(EXIT_FAILURE);
			}
		}
	}

	// Verify that the border is indeed
	if (processor.getCurrentBorder().unknown != false || processor.getCurrentBorder().horizontalSize != 0 || processor.getCurrentBorder().verticalSize != 0)
	{
		std::cerr << "Incorrectlty identified 'no border'" << std::endl;
		exit(EXIT_FAILURE);
	}

	int borderSize = 12;
	Image<ColorRgb> horzImage = createImage(64, 64, borderSize, 0);
	for (unsigned i=0; i<borderCnt*2; ++i)
	{
		bool newBorder = processor.process(horzImage);
		if (i == borderCnt+10)// 10 frames till new border gets a chance to proof consistency
		{
			if (!newBorder)
			{
				std::cerr << "Failed to detect 'horizontal border' when required after " << borderCnt << " images" << std::endl;
				exit(EXIT_FAILURE);
			}
		}
		else
		{
			if (newBorder)
			{
				std::cerr << "Incorrectly detected new border, when there in none" << std::endl;
				exit(EXIT_FAILURE);
			}
		}
	}

	if (processor.getCurrentBorder().unknown != false || processor.getCurrentBorder().horizontalSize != borderSize || processor.getCurrentBorder().verticalSize != 0)
	{

		std::cerr << "Incorrectlty found 'horizontal border' (" << processor.getCurrentBorder().unknown << "," << processor.getCurrentBorder().horizontalSize << "," << processor.getCurrentBorder().verticalSize << ")" << std::endl;
		exit(EXIT_FAILURE);
	}

	for (unsigned i=0; i<borderCnt*2; ++i)
	{

		bool newBorder = processor.process(noBorderImage);
		if (i == borderCnt+10)// 10 frames till new border gets a chance to proof consistency
		{
			if (!newBorder)
			{
				std::cerr << "Failed to detect 'no border' when required after " << borderCnt << " images" << std::endl;
				exit(EXIT_FAILURE);
			}
		}
		else
		{
			if (newBorder)
			{
				std::cerr << "Incorrectly detected no border, when there in none" << std::endl;
				exit(EXIT_FAILURE);
			}
		}
	}

		// Check switch back to no border
		if ( (processor.getCurrentBorder().unknown != false || processor.getCurrentBorder().horizontalSize != 0 || processor.getCurrentBorder().verticalSize != 0))
		{
			std::cerr << "Failed to switch back to 'no border'" << std::endl;
			exit(EXIT_FAILURE);
		}



	Image<ColorRgb> vertImage = createImage(64, 64, 0, borderSize);
	for (unsigned i=0; i<borderCnt*2; ++i)
	{
		bool newBorder = processor.process(vertImage);
		if (i == borderCnt+10)// 10 frames till new border gets a chance to proof consistency
		{
			if (!newBorder)
			{
				std::cerr << "Failed to detect 'vertical border' when required after " << borderCnt << " images" << std::endl;
				exit(EXIT_FAILURE);
			}
		}
		else
		{
			if (newBorder)
			{
				std::cerr << "Incorrectly detected new border, when there in none" << std::endl;
				exit(EXIT_FAILURE);
			}
		}
	}

	if (processor.getCurrentBorder().unknown != false || processor.getCurrentBorder().horizontalSize != 0 || processor.getCurrentBorder().verticalSize != borderSize)
	{
		std::cerr << "Incorrectlty found 'vertical border'" << std::endl;
		exit(EXIT_FAILURE);
	}

	// Switch back (in one shot) to no border
//	assert(processor.process(noBorderImage));
//	assert(processor.getCurrentBorder().verticalSize == 0 && processor.getCurrentBorder().horizontalSize == 0);

	return 0;
}
