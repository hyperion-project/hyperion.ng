
// STL includes
#include <iostream>

// Utils includes
#include <utils/Image.h>
#include <utils/ColorRgb.h>

int main()
{
	std::cout << "Constructing image" << std::endl;
	Image<ColorRgb> image(64, 64, ColorRgb::BLACK);

	std::cout << "Writing image" << std::endl;
	for (unsigned y=0; y<64; ++y)
	{
		for (unsigned x=0; x<64; ++x)
		{
			image(x,y) = ColorRgb::RED;
		}
	}

	std::cout << "Finished (destruction will be performed)" << std::endl;

	return 0;
}
