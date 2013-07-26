
// STL includes
#include <iostream>

// Boblight includes
#include <boblight.h>

int main()
{
	std::cout << "Initialisaing Boblight" << std::endl;
	void* blPng = boblight_init();

	int width  = 112;
	int height = 64;
	std::cout << "Defining scan range (" << width << "x" << height << ")" << std::endl;
	boblight_setscanrange(blPng, width, height);

	int colorPtr[3];
	colorPtr[0] = 255;
	colorPtr[1] = 0;
	colorPtr[2] = 0;
	std::cout << "Using color [" << colorPtr[0] << "; " << colorPtr[1] << "; " << colorPtr[2] << "]" << std::endl;

	int nrOfFrames = 150;
	std::cout << "Generating " << nrOfFrames << " frames" << std::endl;
	for (int iFrame=0; iFrame<nrOfFrames; ++iFrame)
	{
		for (int iWidth=0; iWidth<width; ++iWidth)
		{
			for (int iHeight=0; iHeight<height; ++iHeight)
			{
				boblight_addpixelxy(blPng, iWidth, iHeight, colorPtr);
			}
		}

		boblight_sendrgb(blPng, 0, NULL);
	}

	std::cout << "Destroying Boblight" << std::endl;
	boblight_destroy(blPng);

	return 0;
}
