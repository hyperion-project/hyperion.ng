
// STL includes
#include <iostream>

// HyperionPNG includes
#include <hyperionpng/HyperionPng.h>

template <typename Hyperion_T>
void process(Hyperion_T& hyperion)
{
	hyperion.setInputSize(64, 64);

	// Obtain reference to buffer
	RgbImage& image = hyperion.image();

	// Write some data to the image
	std::cout << "Write data to buffer-image" << std::endl;
	for (unsigned y=0; y<image.height(); ++y)
	{
		for (unsigned x=0; x<image.width(); ++x)
		{
			const RgbColor color = {255, 0, 0};
			image(x,y) = color;
		}
	}

	std::cout << "Commit image to write png" << std::endl;
	for (unsigned i=0; i<40; ++i)
	{
		// Commit the image (writing first png)
		hyperion.commit();
	}
	std::cout << "FINISHED" << std::endl;
}

int main()
{
	// Construct instance of Hyperion-PNG
	std::cout << "Initialisaing Hyperion PNG" << std::endl;
	HyperionPng hyperion;

	process(hyperion);

	return 0;
}
