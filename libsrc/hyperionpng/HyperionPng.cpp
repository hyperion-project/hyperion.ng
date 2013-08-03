// PNG includes
#ifndef NO_FREETYPE
#define NO_FREETYPE
#endif
#include "pngwriter.h"


#include <hyperionpng/HyperionPng.h>

HyperionPng::HyperionPng() :
	mBuffer(nullptr),
	mFrameCnt(0),
	mWriter(new pngwriter()),
	mFileIndex(0)
{
	// empty
}

HyperionPng::~HyperionPng()
{
	std::cout << "HyperionPng is being deleted" << std::endl;
	delete mBuffer;

	mWriter->close();
	delete mWriter;
}

void HyperionPng::setInputSize(const unsigned width, const unsigned height)
{
	delete mBuffer;
	mBuffer = new RgbImage(width, height);
}

RgbImage& HyperionPng::image()
{
	return *mBuffer;
}

void HyperionPng::commit()
{
	writeImage(*mBuffer);
}

void HyperionPng::operator() (const RgbImage& inputImage)
{
	writeImage(inputImage);
}

void HyperionPng::writeImage(const RgbImage& inputImage)
{
	// Write only every n'th frame
	if (mFrameCnt%10 == 0)
	{
		// Set the filename for the PNG
		char filename[64];
		sprintf(filename, "/home/pi/RASPI_%04lu.png", mFileIndex);
		mWriter->pngwriter_rename(filename);
		mWriter->resize(inputImage.width(), inputImage.height());

		// Plot the pixels from the image to the PNG-Writer
		for (unsigned y=0; y<inputImage.width(); ++y)
		{
			for (unsigned x=0; x<inputImage.height(); ++x)
			{
				const RgbColor& color = inputImage(x,y);
				mWriter->plot(x+1, inputImage.height()-y, color.red/255.0, color.green/255.0, color.blue/255.0);
			}
		}

		std::cout << "Writing the PNG" << std::endl;
		// Write-out the current frame and prepare for the next
		mWriter->write_png();

		++mFileIndex;
		std::cout << "PNGWRITER FINISHED" << std::endl;
	}
	++mFrameCnt;
}

