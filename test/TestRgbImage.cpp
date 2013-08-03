
// Utils includes
#include <utils/RgbImage.h>

int main()
{
	std::cout << "Constructing image" << std::endl;
	RgbImage image(64, 64, RgbColor::BLACK);

	std::cout << "Writing image" << std::endl;
	for (unsigned y=0; y<64; ++y)
	{
		for (unsigned x=0; x<64; ++x)
		{
			image(x,y) = RgbColor::RED;
		}
	}

	std::cout << "Finished (destruction will be performed)" << std::endl;

	return 0;
}
