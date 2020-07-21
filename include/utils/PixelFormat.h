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

inline PixelFormat parsePixelFormat(QString pixelFormat)
{
	// convert to lower case
	pixelFormat = pixelFormat.toLower();

	if (pixelFormat.compare("yuyv") )
	{
		return PixelFormat::YUYV;
	}
	else if (pixelFormat.compare("uyvy") )
	{
		return PixelFormat::UYVY;
	}
	else if (pixelFormat.compare("bgr16") )
	{
		return PixelFormat::BGR16;
	}
	else if (pixelFormat.compare("bgr24") )
	{
		return PixelFormat::BGR24;
	}
	else if (pixelFormat.compare("rgb32") )
	{
		return PixelFormat::RGB32;
	}
	else if (pixelFormat.compare("bgr32") )
	{
		return PixelFormat::BGR32;
	}
#ifdef HAVE_JPEG_DECODER
	else if (pixelFormat.compare("mjpeg") )
	{
		return PixelFormat::MJPEG;
	}
#endif

	// return the default NO_CHANGE
	return PixelFormat::NO_CHANGE;
}
