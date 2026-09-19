#include "ScreenshotUtil.h"

#include <QImage>

void saveScreenshot(const QString& filename, const Image<ColorRgb>& image)
{
	// store as PNG
	QImage const pngImage(
		reinterpret_cast<const uint8_t*>(image.memptr()),
		image.width(),
		image.height(),
		3 * image.width(),
		QImage::Format_RGB888);
	pngImage.save(filename);
}
