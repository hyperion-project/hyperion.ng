
// STL includes
#include <iostream>

// Utils includes
#include <utils/Image.h>
#include <utils/ColorRgba.h>
#include <utils/ColorRgb.h>
#include <utils/ColorBgr.h>
#include <hyperion/ImageProcessor.h>

int main()
{
	std::cout << "Constructing image" << std::endl;
	int width = 64;
	int height = 64;
	Image<ColorRgb> image_rgb(width, height, ColorRgb::BLACK);
	Image<ColorBgr> image_bgr(image_rgb.width(), image_rgb.height(), ColorBgr::BLACK);

	std::cout << "Writing image" << std::endl;
	unsigned l = width * height;

	// BGR
 	for (unsigned i=0; i<l; ++i)
 		image_bgr.memptr()[i] = ColorBgr{0,128,255};

	
	// to RGB
	image_bgr.toRgb(image_rgb);

	// test
	for (unsigned i=0; i<l; ++i)
	{
		const ColorRgb rgb = image_rgb.memptr()[i];
		if ( rgb.red != 255 || rgb.green != 128 || rgb.blue != 0 )
			std::cout << "RGB error idx " << i << " " << rgb << std::endl;
	}

	
	
	

	std::cout << "Finished (destruction will be performed)" << std::endl;

	return 0;
}
