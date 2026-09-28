#include "utils/ImageResampler.h"
#include <utils/ColorSys.h>
#include <utils/Logger.h>

ImageResampler::ImageResampler()
	: _horizontalDecimation(8)
	, _verticalDecimation(8)
	, _cropLeft(0)
	, _cropRight(0)
	, _cropTop(0)
	, _cropBottom(0)
	, _videoMode(VideoMode::VIDEO_2D)
	, _flipMode(FlipMode::NO_CHANGE)
{
}

void ImageResampler::setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom)
{
	_cropLeft   = cropLeft;
	_cropRight  = cropRight;
	_cropTop    = cropTop;
	_cropBottom = cropBottom;
}

/// @brief Converts and resamples a raw video frame into an @c Image<ColorRgb>.
///
/// The function applies, in order:
/// 1. **3D-mode crop adjustment** — for side-by-side (SBS) or top-and-bottom
///    (TAB) stereoscopic video, the crop margins are adjusted so that only the
///    left (SBS) or top (TAB) half of the frame is processed.
/// 2. **Crop + decimation** — the effective source region is
///    @c [cropLeft, width-cropRight) × [cropTop, height-cropBottom).  Within
///    that region, one sample is taken every @c _horizontalDecimation /
///    @c _verticalDecimation pixels, starting half a decimation step from the
///    crop boundary.  The formula includes a ceiling correction so that partial
///    final blocks are included in the output dimensions.
/// 3. **Flip mode** — destination coordinates are negated on one or both axes
///    to produce horizontal, vertical, or combined flips.  @c abs() is applied
///    at write time so the actual memory address is always non-negative.
/// 4. **Pixel-format conversion** — each sampled source pixel is decoded from
///    its native format and written as @c ColorRgb into @p outputImage.
///
/// @par Supported pixel formats
/// | Format | Layout | Notes |
/// |--------|--------|-------|
/// | UYVY / YUYV | 4:2:2 packed | U/V selected based on even/odd x position |
/// | BGR16 | 5-6-5 packed | Channels unpacked with bit-shift masks |
/// | RGB24 / BGR24 | 3 bytes/px | Direct channel copy |
/// | RGB32 / BGR32 | 4 bytes/px | Alpha byte ignored |
/// | NV12 / NV21 | 4:2:0 semi-planar | Interleaved UV plane at offset `height * lineLength` |
/// | I420 | 4:2:0 planar | Separate U and V planes |
/// | I422 | 4:2:2 planar | Separate U and V planes |
/// | MJPEG | — | Not decoded here; case is a no-op |
/// | P030 | — | Not yet supported; logs a warning |
/// | NO_CHANGE | — | Invalid sentinel; logs an error |
///
/// @param data        Pointer to the raw frame buffer.
/// @param width       Frame width in pixels.
/// @param height      Frame height in pixels.
/// @param lineLength  Number of bytes per source scan line (stride).
/// @param pixelFormat The pixel format of @p data.
/// @param outputImage Destination image; resized and filled in place.
void ImageResampler::processImage(const uint8_t * data, int width, int height, size_t lineLength, PixelFormat pixelFormat, Image<ColorRgb> &outputImage) const
{
	// Start with the configured crop margins; they may be adjusted for 3D modes.
	int cropLeft = _cropLeft;
	int cropRight  = _cropRight;
	int cropTop = _cropTop;
	int cropBottom = _cropBottom;

	// Adjust crop margins for stereoscopic 3D modes so only one eye's image is processed.
	switch (_videoMode)
	{
	case VideoMode::VIDEO_3DSBS:
		// Side-by-side: restrict to the left half of the frame.
		cropRight =  (width >> 1) + (cropRight >> 1);
		cropLeft = cropLeft >> 1;
		break;
	case VideoMode::VIDEO_3DTAB:
		// Top-and-bottom: restrict to the top half of the frame.
		cropBottom = (height >> 1) + (cropBottom >> 1);
		cropTop = cropTop >> 1;
		break;
	default:
		break;
	}

	// Calculate the output dimensions after cropping and decimation.
	// The ceiling formula ensures partial final blocks are included.
	int outputWidth = (width - cropLeft - cropRight - (_horizontalDecimation >> 1) + _horizontalDecimation - 1) / _horizontalDecimation;
	int outputHeight = (height - cropTop - cropBottom - (_verticalDecimation >> 1) + _verticalDecimation - 1) / _verticalDecimation;

	outputImage.resize(outputWidth, outputHeight);

	// Destination index bounds.  These are signed so that negative values can
	// be used to implement flipping: abs() is applied when addressing outputImage.
	int xDestStart {0};
	int xDestEnd = {outputWidth-1};
	int yDestStart = {0};
	int yDestEnd = {outputHeight-1};

	// Configure destination index traversal direction for each flip mode.
	switch (_flipMode)
	{
		case FlipMode::NO_CHANGE:
			//use the initalized values
			break;
		case FlipMode::HORIZONTAL:
			// Mirror top↔bottom: iterate y from -(outputHeight-1) up to 0.
			xDestStart = 0;
			xDestEnd = outputWidth-1;
			yDestStart = -(outputHeight-1);
			yDestEnd = 0;
			break;
		case FlipMode::VERTICAL:
			// Mirror left↔right: iterate x from -(outputWidth-1) up to 0.
			xDestStart = -(outputWidth-1);
			xDestEnd = 0;
			yDestStart = 0;
			yDestEnd = outputHeight-1;
			break;
		case FlipMode::BOTH:
			// Mirror on both axes.
			xDestStart = -(outputWidth-1);
			xDestEnd = 0;
			yDestStart = -(outputHeight-1);
			yDestEnd = 0;
			break;
	}

	// Decode pixels from the source buffer into the output image.
	// Each case handles one pixel format's byte layout and colour space.
	switch (pixelFormat)
	{
		case PixelFormat::UYVY:
		{
			// Packed 4:2:2 YCbCr: byte order U0 Y0 V0 Y1 per 2-pixel macro-block.
			// U and V are shared between even (xSource&1==0) and odd pixels.
			for (int yDest = yDestStart, ySource = cropTop + (_verticalDecimation >> 1); yDest <= yDestEnd; ySource += _verticalDecimation, ++yDest)
			{
				for (int xDest = xDestStart, xSource = cropLeft + (_horizontalDecimation >> 1); xDest <= xDestEnd; xSource += _horizontalDecimation, ++xDest)
				{
					ColorRgb & rgb = outputImage(abs(xDest), abs(yDest));
					size_t index = lineLength * ySource + (xSource << 1);
					uint8_t y = data[index+1];
					uint8_t u = ((xSource&1) == 0) ? data[index  ] : data[index-2];
					uint8_t v = ((xSource&1) == 0) ? data[index+2] : data[index  ];
					ColorSys::yuv2rgb(y, u, v, rgb.red, rgb.green, rgb.blue);
				}
			}
			break;
		}

		case PixelFormat::YUYV:
		{
			// Packed 4:2:2 YCbCr: byte order Y0 U0 Y1 V0 per 2-pixel macro-block.
			for (int yDest = yDestStart, ySource = cropTop + (_verticalDecimation >> 1); yDest <= yDestEnd; ySource += _verticalDecimation, ++yDest)
			{
				for (int xDest = xDestStart, xSource = cropLeft + (_horizontalDecimation >> 1); xDest <= xDestEnd; xSource += _horizontalDecimation, ++xDest)
				{
					ColorRgb & rgb = outputImage(abs(xDest), abs(yDest));
					size_t index = lineLength * ySource + (xSource << 1);
					uint8_t y = data[index];
					uint8_t u = ((xSource&1) == 0) ? data[index+1] : data[index-1];
					uint8_t v = ((xSource&1) == 0) ? data[index+3] : data[index+1];
					ColorSys::yuv2rgb(y, u, v, rgb.red, rgb.green, rgb.blue);
				}
			}
			break;
		}

		case PixelFormat::BGR16:
		{
			// 16-bit packed RGB 5-6-5 in BGR order: B[4:0] G[5:0] R[4:0].
			for (int yDest = yDestStart, ySource = cropTop + (_verticalDecimation >> 1); yDest <= yDestEnd; ySource += _verticalDecimation, ++yDest)
			{
				for (int xDest = xDestStart, xSource = cropLeft + (_horizontalDecimation >> 1); xDest <= xDestEnd; xSource += _horizontalDecimation, ++xDest)
				{
					ColorRgb & rgb = outputImage(abs(xDest), abs(yDest));
					size_t index = lineLength * ySource + (xSource << 1);
					rgb.blue  = static_cast<uint8_t>((data[index] & 0x1f) << 3);
					rgb.green = static_cast<uint8_t>((((data[index+1] & 0x7) << 3) | (data[index] & 0xE0) >> 5) << 2);
					rgb.red   = (data[index+1] & 0xF8);
				}
			}
			break;
		}

		case PixelFormat::RGB24:
		{
			// 3 bytes per pixel in R-G-B order.
			for (int yDest = yDestStart, ySource = cropTop + (_verticalDecimation >> 1); yDest <= yDestEnd; ySource += _verticalDecimation, ++yDest)
			{
				for (int xDest = xDestStart, xSource = cropLeft + (_horizontalDecimation >> 1); xDest <= xDestEnd; xSource += _horizontalDecimation, ++xDest)
				{
					ColorRgb & rgb = outputImage(abs(xDest), abs(yDest));
					size_t index = lineLength * ySource + (xSource << 1) + xSource;
					rgb.red   = data[index  ];
					rgb.green = data[index+1];
					rgb.blue  = data[index+2];
				}
			}
			break;
		}

		case PixelFormat::BGR24:
		{
			// 3 bytes per pixel in B-G-R order.
			for (int yDest = yDestStart, ySource = cropTop + (_verticalDecimation >> 1); yDest <= yDestEnd; ySource += _verticalDecimation, ++yDest)
			{
				for (int xDest = xDestStart, xSource = cropLeft + (_horizontalDecimation >> 1); xDest <= xDestEnd; xSource += _horizontalDecimation, ++xDest)
				{
					ColorRgb & rgb = outputImage(abs(xDest), abs(yDest));
					size_t index = lineLength * ySource + (xSource << 1) + xSource;
					rgb.blue  = data[index  ];
					rgb.green = data[index+1];
					rgb.red   = data[index+2];
				}
			}
			break;
		}

		case PixelFormat::RGB32:
		{
			// 4 bytes per pixel in R-G-B-x order; the fourth byte (alpha/padding) is ignored.
			for (int yDest = yDestStart, ySource = cropTop + (_verticalDecimation >> 1); yDest <= yDestEnd; ySource += _verticalDecimation, ++yDest)
			{
				for (int xDest = xDestStart, xSource = cropLeft + (_horizontalDecimation >> 1); xDest <= xDestEnd; xSource += _horizontalDecimation, ++xDest)
				{
					ColorRgb & rgb = outputImage(abs(xDest), abs(yDest));
					size_t index = lineLength * ySource + (xSource << 2);
					rgb.red   = data[index  ];
					rgb.green = data[index+1];
					rgb.blue  = data[index+2];
				}
			}
			break;
		}

		case PixelFormat::BGR32:
		{
			// 4 bytes per pixel in B-G-R-x order; the fourth byte (alpha/padding) is ignored.
			for (int yDest = yDestStart, ySource = cropTop + (_verticalDecimation >> 1); yDest <= yDestEnd; ySource += _verticalDecimation, ++yDest)
			{
				for (int xDest = xDestStart, xSource = cropLeft + (_horizontalDecimation >> 1); xDest <= xDestEnd; xSource += _horizontalDecimation, ++xDest)
				{
					ColorRgb & rgb = outputImage(abs(xDest), abs(yDest));
					size_t index = lineLength * ySource + (xSource << 2);
					rgb.blue  = data[index  ];
					rgb.green = data[index+1];
					rgb.red   = data[index+2];
				}
			}
			break;
		}

		case PixelFormat::NV12:
		{
			// Semi-planar 4:2:0: Y plane followed by interleaved U-V pairs.
			// The UV plane starts at row `height`, one UV row per two Y rows.
			for (int yDest = yDestStart, ySource = cropTop + (_verticalDecimation >> 1); yDest <= yDestEnd; ySource += _verticalDecimation, ++yDest)
			{
				size_t uOffset = (height + ySource / 2) * lineLength;
				for (int xDest = xDestStart, xSource = cropLeft + (_horizontalDecimation >> 1); xDest <= xDestEnd; xSource += _horizontalDecimation, ++xDest)
				{
					ColorRgb & rgb = outputImage(abs(xDest), abs(yDest));
					uint8_t y = data[lineLength * ySource + xSource];
					uint8_t u = data[uOffset + ((xSource >> 1) << 1)];
					uint8_t v = data[uOffset + ((xSource >> 1) << 1) + 1];
					ColorSys::yuv2rgb(y, u, v, rgb.red, rgb.green, rgb.blue);
				}
			}
			break;
		}

		case PixelFormat::NV21:
		{
			// Semi-planar 4:2:0: Y plane followed by interleaved V-U pairs (U/V swapped vs. NV12).
			for (int yDest = yDestStart, ySource = cropTop + (_verticalDecimation >> 1); yDest <= yDestEnd; ySource += _verticalDecimation, ++yDest)
			{
				size_t uOffset = (height + ySource / 2) * lineLength;
				for (int xDest = xDestStart, xSource = cropLeft + (_horizontalDecimation >> 1); xDest <= xDestEnd; xSource += _horizontalDecimation, ++xDest)
				{
					ColorRgb & rgb = outputImage(abs(xDest), abs(yDest));
					uint8_t y = data[lineLength * ySource + xSource];
					uint8_t v = data[uOffset + ((xSource >> 1) << 1)];     // note: V before U
					uint8_t u = data[uOffset + ((xSource >> 1) << 1) + 1];
					ColorSys::yuv2rgb(y, u, v, rgb.red, rgb.green, rgb.blue);
				}
			}
			break;
		}

		case PixelFormat::I420: // YUV 4:2:0 Planar
		{
			// Fully planar: Y plane, then U plane (quarter size), then V plane.
			for (int yDest = yDestStart, ySource = cropTop + (_verticalDecimation >> 1); yDest <= yDestEnd; ySource += _verticalDecimation, ++yDest)
			{
				int uOffset = width * height + (ySource/2) * width/2;
				int vOffset = width * height + (width * height / 4) + (ySource/2) * width/2;
				for (int xDest = xDestStart, xSource = cropLeft + (_horizontalDecimation >> 1); xDest <= xDestEnd; xSource += _horizontalDecimation, ++xDest)
				{
					ColorRgb & rgb = outputImage(abs(xDest), abs(yDest));
					uint8_t y = data[lineLength * ySource + xSource];
					uint8_t u = data[uOffset + (xSource >> 1)];
					uint8_t v = data[vOffset + (xSource >> 1)];
					ColorSys::yuv2rgb(y, u, v, rgb.red, rgb.green, rgb.blue);
				}
			}
			break;
		}

		case PixelFormat::I422: // YUV 4:2:2 Planar
		{
			// Fully planar: Y plane, then U plane (half width, full height), then V plane.
			for (int yDest = yDestStart, ySource = cropTop + (_verticalDecimation >> 1); yDest <= yDestEnd; ySource += _verticalDecimation, ++yDest)
			{
				int uOffset = width * height + ySource * (width/2);
				int vOffset = (width * height) + (width * height / 2) + ySource * (width/2);
				for (int xDest = xDestStart, xSource = cropLeft + (_horizontalDecimation >> 1); xDest <= xDestEnd; xSource += _horizontalDecimation, ++xDest)
				{
					ColorRgb & rgb = outputImage(abs(xDest), abs(yDest));
					uint8_t y = data[lineLength * ySource + xSource];
					uint8_t u = data[uOffset + (xSource >> 1)];
					uint8_t v = data[vOffset + (xSource >> 1)];
					ColorSys::yuv2rgb(y, u, v, rgb.red, rgb.green, rgb.blue);
				}
			}
			break;
		}

		case PixelFormat::MJPEG:
			// MJPEG frames are decoded upstream before reaching this function; nothing to do.
		break;
		case PixelFormat::P030:
			// 10-bit packed format — not yet implemented.
			Warning(Logger::getInstance("ImageResampler"), "%s",
					QSTRING_CSTR(QString("Pixel format %1 not supported yet").arg(pixelFormatToString(pixelFormat))));
			break;
		case PixelFormat::NO_CHANGE:
			Error(Logger::getInstance("ImageResampler"), "Invalid pixel format given");
		break;
	}
}
