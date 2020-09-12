#pragma once

#include <QString>

/**
 * Enumeration of the possible pixel formats the grabber can be set to
 */
enum class PixelFormat {
	YUYV,
	UYVY,
	BGR16,
	BGR24,
	RGB32,
	BGR32,
#ifdef HAVE_JPEG_DECODER
	MJPEG,
#endif
	NO_CHANGE
};

inline PixelFormat parsePixelFormat(const QString& pixelFormat)
{
	// convert to lower case
	QString format = pixelFormat.toLower();

	if (format.compare("yuyv") )
	{
		return PixelFormat::YUYV;
	}
	else if (format.compare("uyvy") )
	{
		return PixelFormat::UYVY;
	}
	else if (format.compare("bgr16") )
	{
		return PixelFormat::BGR16;
	}
	else if (format.compare("bgr24") )
	{
		return PixelFormat::BGR24;
	}
	else if (format.compare("rgb32") )
	{
		return PixelFormat::RGB32;
	}
	else if (format.compare("bgr32") )
	{
		return PixelFormat::BGR32;
	}
#ifdef HAVE_JPEG_DECODER
	else if (format.compare("mjpeg") )
	{
		return PixelFormat::MJPEG;
	}
#endif

	// return the default NO_CHANGE
	return PixelFormat::NO_CHANGE;
}
