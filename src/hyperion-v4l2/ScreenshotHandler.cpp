// Qt includes
#include <QImage>
#include <QCoreApplication>

// hyperion-v4l2 includes
#include "ScreenshotHandler.h"

ScreenshotHandler::ScreenshotHandler(const std::string & filename) :
		_filename(filename)
{
}

ScreenshotHandler::~ScreenshotHandler()
{
}

void ScreenshotHandler::receiveImage(const Image<ColorRgb> & image)
{
	// store as PNG
	QImage pngImage((const uint8_t *) image.memptr(), image.width(), image.height(), 3*image.width(), QImage::Format_RGB888);
	pngImage.save(_filename.c_str());

	// Quit the application after the first image
	QCoreApplication::quit();
}
