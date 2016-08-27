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
	// store as PNG
	QImage pngImage((const uint8_t *) image.memptr(), image.width(), image.height(), 3*image.width(), QImage::Format_RGB888);
	pngImage.save(_filename);

	// Quit the application after the first image
	QCoreApplication::quit();
}
