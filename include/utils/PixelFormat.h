#ifndef PIXELFORMAT_H
#define PIXELFORMAT_H

#include <QString>

/**
 * Enumeration of the possible pixel formats the grabber can be set to
 */
enum class PixelFormat {
	YUYV,
	UYVY,
	BGR16,
	RGB24,
	BGR24,
	RGB32,
	BGR32,
	NV12,
	NV21,
	P030,
	I420,
	MJPEG,
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
	else if (format.compare("rgb24")  == 0)
	{
		return PixelFormat::RGB24;
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
	else if (format.compare("mjpeg")  == 0)
	{
		return PixelFormat::MJPEG;
	}

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
	else if (pixelFormat == PixelFormat::RGB24)
	{
		return "RGB24";
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
	else if (pixelFormat == PixelFormat::MJPEG)
	{
		return "MJPEG";
	}

	// return the default NO_CHANGE
	return "NO_CHANGE";
}

/**
 * Enumeration of the possible flip modes
 */

enum class FlipMode
{
	NO_CHANGE,
	HORIZONTAL,
	VERTICAL,
	BOTH
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

#endif // PIXELFORMAT_H
