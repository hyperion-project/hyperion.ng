// Qt includes
#include <QImage>
#include <QCoreApplication>

// hyperion-v4l2 includes
#include "ScreenshotHandler.h"

ScreenshotHandler::ScreenshotHandler(const QString & filename, const QRectF & signalDetectionOffset)
	: _filename(filename)
	, _signalDetectionOffset(signalDetectionOffset)
{
}

ScreenshotHandler::~ScreenshotHandler()
{
}

void ScreenshotHandler::receiveImage(const Image<ColorRgb> & image)
{
	double x_frac_min = _signalDetectionOffset.x();
	double y_frac_min = _signalDetectionOffset.y();
	double x_frac_max = _signalDetectionOffset.width();
	double y_frac_max = _signalDetectionOffset.height();

	int xOffset   = image.width()  * x_frac_min;
	int yOffset   = image.height() * y_frac_min;
	int xMax      = image.width()  * x_frac_max;
	int yMax      = image.height() * y_frac_max;

	std::cout << std::endl << "Screenshot details"
	          << std::endl << "=================="
	          << std::endl << "dimension after decimation: " << image.width() << " x " << image.height()
	          << std::endl << "signal detection area  : " << xOffset << "," << yOffset << " x "  << xMax << "," << yMax  << std::endl;

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
	std::cout << "signal threshold color : " << noSignalThresholdColor << std::endl;
	std::cout << "signal threshold values: " << (float)noSignalThresholdColor.red/255.0f << ", "<< (float)noSignalThresholdColor.green/255.0f << ", " << (float)noSignalThresholdColor.blue/255.0f << std::endl;

	// store as PNG
	QImage pngImage((const uint8_t *) image.memptr(), image.width(), image.height(), 3*image.width(), QImage::Format_RGB888);
	pngImage.save(_filename);

	// Quit the application after the first image
	QCoreApplication::quit();
}
