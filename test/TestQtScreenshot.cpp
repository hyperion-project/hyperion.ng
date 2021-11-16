
// STL includes
#include <iostream>

// QT includes
#include <QApplication>
#include <QPixmap>
#include <QFile>
#include <QRgb>
#include <QScreen>

#include <QElapsedTimer>

// Utils includes
#include <utils/Image.h>
#include <utils/ColorRgb.h>

void createScreenshot(const int cropHorizontal, const int cropVertical, const int decimation, Image<ColorRgb> & image)
{
	// Create the full size screenshot
	QScreen *screen = QApplication::primaryScreen();
	const QRect screenSize = screen->availableGeometry();
	const int croppedWidth  = screenSize.width()  - 2*cropVertical;
	const int croppedHeight = screenSize.height() - 2*cropHorizontal;
	const QPixmap fullSizeScreenshot = screen->grabWindow(0, cropVertical, cropHorizontal, croppedWidth, croppedHeight);

	// Scale the screenshot to the required size
	const int width  = fullSizeScreenshot.width()/decimation;
	const int height = fullSizeScreenshot.height()/decimation;
	const QPixmap scaledScreenshot = fullSizeScreenshot.scaled(width, height, Qt::IgnoreAspectRatio, Qt::FastTransformation);

	// Convert the QPixmap to QImage in order to get out RGB values
	const QImage qImage = scaledScreenshot.toImage();

	// Make sure that the output image has the right size
	image.resize(width, height);

	// Copy the data into the output image
	for (int y=0; y<qImage.height(); ++y)
	{
		for (int x=0; x<qImage.width(); ++x)
		{
			// Get the pixel at [x;y] (format int32 #AARRGGBB)
			const QRgb inPixel = qImage.pixel(x,y);

			// Copy the color channels into the output pixel
			ColorRgb & outPixel = image(x,y);
			outPixel.red   = (inPixel & 0x00ff0000) >> 16;
			outPixel.green = (inPixel & 0x0000ff00) >>  8;
			outPixel.blue  = (inPixel & 0x000000ff);
		}
	}
}

int main(int argc, char** argv)
{
	int decimation = 10;

	QApplication app(argc, argv);
	QElapsedTimer timer;

	Image<ColorRgb> screenshot(64,64);

	int loopCnt = 100;
	timer.start();
	for (int i=0; i<loopCnt; ++i)
	{
		createScreenshot(0,0, decimation, screenshot);
	}
	std::cout << "Time required for single screenshot: " << timer.elapsed()/loopCnt << "ms" << std::endl;

	return 0;
}
