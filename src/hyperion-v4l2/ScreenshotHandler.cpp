// Qt includes
#include <QImage>
#include <QCoreApplication>

// hyperion-v4l2 includes
#include "ScreenshotHandler.h"

ScreenshotHandler::ScreenshotHandler(const QString & filename) :
	_filename(filename)
{
}

ScreenshotHandler::~ScreenshotHandler()
{
}

void ScreenshotHandler::receiveImage(const Image<ColorRgb> & image)
{
	std::cout << std::endl << "Screenshot details" << std::endl << "==================" << std::endl;
	std::cout << "dimension: " << image.width() << " x " << image.width() << std::endl;
	std::cout << "no signal detection area: " << (image.width()>>2) << "," << (image.height()>>2) << " x "
	          << (image.width()>>1)-1 << "," << (image.height()>>1)-1  << std::endl;

	ColorRgb noSignalThresholdColor = {0,0,0};

	for (unsigned x = 0; x < (image.width()>>1); ++x)
	{
		int xImage = (image.width()>>2) + x;

		for (unsigned y = 0; y < (image.height()>>1); ++y)
		{
			int yImage = (image.height()>>2) + y;

			ColorRgb rgb = image(xImage, yImage);
			if (rgb > noSignalThresholdColor)
			{
				noSignalThresholdColor = rgb;
			}
		}
	}
	std::cout << "no signal threshold Color: " << noSignalThresholdColor << std::endl;
	std::cout << "no signal threshold values: " << (float)noSignalThresholdColor.red/255.0f << ", "<< (float)noSignalThresholdColor.green/255.0f << ", " << (float)noSignalThresholdColor.blue/255.0f << std::endl;

	// store as PNG
	QImage pngImage((const uint8_t *) image.memptr(), image.width(), image.height(), 3*image.width(), QImage::Format_RGB888);
	pngImage.save(_filename);

	// Quit the application after the first image
	QCoreApplication::quit();
}
