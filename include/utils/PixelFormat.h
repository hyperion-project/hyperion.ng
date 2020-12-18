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

	if (format.compare("yuyv") == 0)
	{
		return PixelFormat::YUYV;
	}
	else if (format.compare("uyvy")  == 0)
	{
		return PixelFormat::UYVY;
	}
	else if (format.compare("bgr16")  == 0)
	{
		return PixelFormat::BGR16;
	}
	else if (format.compare("bgr24")  == 0)
	{
		return PixelFormat::BGR24;
	}
	else if (format.compare("rgb32")  == 0)
	{
		return PixelFormat::RGB32;
	}
	else if (format.compare("bgr32")  == 0)
	{
		return PixelFormat::BGR32;
	}
#ifdef HAVE_JPEG_DECODER
	else if (format.compare("mjpeg")  == 0)
	{
		return PixelFormat::MJPEG;
	}
#endif

	// return the default NO_CHANGE
	return PixelFormat::NO_CHANGE;
}

inline QString pixelFormatToString(const PixelFormat& pixelFormat)
{

	if ( pixelFormat == PixelFormat::YUYV)
	{
		return "yuyv";
	}
	else if (pixelFormat == PixelFormat::UYVY)
	{
		return "uyvy";
	}
	else if (pixelFormat == PixelFormat::BGR16)
	{
		return "bgr16";
	}
	else if (pixelFormat == PixelFormat::BGR24)
	{
		return "bgr24";
	}
	else if (pixelFormat == PixelFormat::RGB32)
	{
		return "rgb32";
	}
	else if (pixelFormat == PixelFormat::BGR32)
	{
		return "bgr32";
	}
#ifdef HAVE_JPEG_DECODER
	else if (pixelFormat == PixelFormat::MJPEG)
	{
		return "mjpeg";
	}
#endif

	// return the default NO_CHANGE
	return "NO_CHANGE";
}
