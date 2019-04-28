#pragma once

#include <QString>

/**
 * Enumeration of the possible pixel formats the grabber can be set to
 */
enum PixelFormat {
	PIXELFORMAT_YUYV,
	PIXELFORMAT_UYVY,
	PIXELFORMAT_BGR16,
	PIXELFORMAT_BGR24,
	PIXELFORMAT_RGB32,
	PIXELFORMAT_BGR32,
#ifdef HAVE_JPEG
	PIXELFORMAT_MJPEG,
#endif
	PIXELFORMAT_NO_CHANGE
};

inline PixelFormat parsePixelFormat(QString pixelFormat)
{
	// convert to lower case
	pixelFormat = pixelFormat.toLower();

	if (pixelFormat == "yuyv")
	{
		return PIXELFORMAT_YUYV;
	}
	else if (pixelFormat == "uyvy")
	{
		return PIXELFORMAT_UYVY;
	}
	else if (pixelFormat == "bgr16")
	{
		return PIXELFORMAT_BGR16;
	}
	else if (pixelFormat == "bgr24")
	{
		return PIXELFORMAT_BGR24;
	}
	else if (pixelFormat == "rgb32")
	{
		return PIXELFORMAT_RGB32;
	}
	else if (pixelFormat == "bgr32")
	{
		return PIXELFORMAT_BGR32;
	}
#ifdef HAVE_JPEG
	else if (pixelFormat == "mjpeg")
	{
		return PIXELFORMAT_MJPEG;
	}
#endif

	// return the default NO_CHANGE
	return PIXELFORMAT_NO_CHANGE;
}
