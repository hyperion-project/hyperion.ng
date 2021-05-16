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
	NV12,
	I420,
#ifdef HAVE_TURBO_JPEG
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
	else if (format.compare("i420")  == 0)
	{
		return PixelFormat::I420;
	}
	else if (format.compare("nv12") == 0)
	{
		return PixelFormat::NV12;
	}
#ifdef HAVE_TURBO_JPEG
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
		return "YUYV";
	}
	else if (pixelFormat == PixelFormat::UYVY)
	{
		return "UYVY";
	}
	else if (pixelFormat == PixelFormat::BGR16)
	{
		return "BGR16";
	}
	else if (pixelFormat == PixelFormat::BGR24)
	{
		return "BGR24";
	}
	else if (pixelFormat == PixelFormat::RGB32)
	{
		return "RGB32";
	}
	else if (pixelFormat == PixelFormat::BGR32)
	{
		return "BGR32";
	}
	else if (pixelFormat == PixelFormat::I420)
	{
		return "I420";
	}
	else if (pixelFormat == PixelFormat::NV12)
	{
		return "NV12";
	}
#ifdef HAVE_TURBO_JPEG
	else if (pixelFormat == PixelFormat::MJPEG)
	{
		return "MJPEG";
	}
#endif

	// return the default NO_CHANGE
	return "NO_CHANGE";
}

/**
 * Enumeration of the possible flip modes
 */

enum class FlipMode
{
	HORIZONTAL,
	VERTICAL,
	BOTH,
	NO_CHANGE
};

inline FlipMode parseFlipMode(const QString& flipMode)
{
	// convert to lower case
	QString mode = flipMode.toLower();

	if (mode.compare("horizontal") == 0)
	{
		return FlipMode::HORIZONTAL;
	}
	else if (mode.compare("vertical")  == 0)
	{
		return FlipMode::VERTICAL;
	}
	else if (mode.compare("both")  == 0)
	{
		return FlipMode::BOTH;
	}

	// return the default NO_CHANGE
	return FlipMode::NO_CHANGE;
}

inline QString flipModeToString(const FlipMode& flipMode)
{
	if ( flipMode == FlipMode::HORIZONTAL)
	{
		return "horizontal";
	}
	else if (flipMode == FlipMode::VERTICAL)
	{
		return "vertical";
	}
	else if (flipMode == FlipMode::BOTH)
	{
		return "both";
	}

	// return the default NO_CHANGE
	return "NO_CHANGE";
}
