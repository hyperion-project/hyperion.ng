#pragma once

#include <QImage>
#include <QString>

#include <utils/ColorRgb.h>
#include <utils/Image.h>

///
/// Shared helper for standalone grabbers to persist a single captured frame as PNG.
/// Kept header-only so it can be used by every standalone grabber executable without
/// introducing an extra link target.
///
inline void saveScreenshot(const QString& filename, const Image<ColorRgb>& image)
{
	QImage const pngImage(
		reinterpret_cast<const uint8_t*>(image.memptr()),
		image.width(),
		image.height(),
		3 * image.width(),
		QImage::Format_RGB888);
	pngImage.save(filename);
}
