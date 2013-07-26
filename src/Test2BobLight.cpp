// STL includes
#include <iostream>
#include <vector>

#include <unistd.h>

// Boblight includes
#include <boblight.h>

int main()
{
	void* boblight_ptr = boblight_init();
	if (!boblight_ptr)
	{
		std::cerr << "Failed to initialise bob-light" << std::endl;
		return -1;
	}

	const unsigned width  = 112;
	const unsigned height = 63;

	boblight_setscanrange(boblight_ptr, width, height);

	std::vector<int> rgbColor = { 255, 255, 0 };
	for (unsigned iY=0; iY<height; ++iY)
	{
		for (unsigned iX=0; iX<width; ++iX)
		{
			boblight_addpixelxy(boblight_ptr, iX, iY, rgbColor.data());
		}
	}
	boblight_sendrgb(boblight_ptr, 0, nullptr);

	//sleep(5);

	boblight_destroy(boblight_ptr);

	return 0;
}
