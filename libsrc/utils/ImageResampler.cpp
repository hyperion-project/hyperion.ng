#include "utils/ImageResampler.h"
#include <utils/Logger.h>

ImageResampler::ImageResampler()
	: _horizontalDecimation(1)
	, _verticalDecimation(1)
	, _cropLeft(0)
	, _cropRight(0)
	, _cropTop(0)
	, _cropBottom(0)
	, _videoMode(VideoMode::VIDEO_2D)
{
}

ImageResampler::~ImageResampler()
{
}

void ImageResampler::setHorizontalPixelDecimation(int decimator)
{
	_horizontalDecimation = decimator;
}

void ImageResampler::setVerticalPixelDecimation(int decimator)
{
	_verticalDecimation = decimator;
}

void ImageResampler::setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom)
{
	_cropLeft   = cropLeft;
	_cropRight  = cropRight;
	_cropTop    = cropTop;
	_cropBottom = cropBottom;
}

void ImageResampler::setVideoMode(VideoMode mode)
{
	_videoMode = mode;
}

void ImageResampler::processImage(const uint8_t * data, int width, int height, int lineLength, PixelFormat pixelFormat, Image<ColorRgb> &outputImage) const
{
	int cropRight  = _cropRight;
	int cropBottom = _cropBottom;

	// handle 3D mode
	switch (_videoMode)
	{
	case VideoMode::VIDEO_3DSBS:
		cropRight = width >> 1;
		break;
	case VideoMode::VIDEO_3DTAB:
		cropBottom = width >> 1;
		break;
	default:
		break;
	}

	// calculate the output size
	int outputWidth = (width - _cropLeft - cropRight - (_horizontalDecimation >> 1) + _horizontalDecimation - 1) / _horizontalDecimation;
	int outputHeight = (height - _cropTop - cropBottom - (_verticalDecimation >> 1) + _verticalDecimation - 1) / _verticalDecimation;

	if (outputImage.width() != unsigned(outputWidth) || outputImage.height() != unsigned(outputHeight))
		outputImage.resize(outputWidth, outputHeight);

	for (int yDest = 0, ySource = _cropTop + (_verticalDecimation >> 1); yDest < outputHeight; ySource += _verticalDecimation, ++yDest)
	{
		for (int xDest = 0, xSource = _cropLeft + (_horizontalDecimation >> 1); xDest < outputWidth; xSource += _horizontalDecimation, ++xDest)
		{
			ColorRgb & rgb = outputImage(xDest, yDest);

			switch (pixelFormat)
			{
				case PixelFormat::UYVY:
				{
					int index = lineLength * ySource + (xSource << 1);
					uint8_t y = data[index+1];
					uint8_t u = ((xSource&1) == 0) ? data[index  ] : data[index-2];
					uint8_t v = ((xSource&1) == 0) ? data[index+2] : data[index  ];
					yuv2rgb(y, u, v, rgb.red, rgb.green, rgb.blue);
				}
				break;
				case PixelFormat::YUYV:
				{
					int index = lineLength * ySource + (xSource << 1);
					uint8_t y = data[index];
					uint8_t u = ((xSource&1) == 0) ? data[index+1] : data[index-1];
					uint8_t v = ((xSource&1) == 0) ? data[index+3] : data[index+1];
					yuv2rgb(y, u, v, rgb.red, rgb.green, rgb.blue);
				}
				break;
				case PixelFormat::BGR16:
				{
					int index = lineLength * ySource + (xSource << 1);
					rgb.blue  = (data[index] & 0x1f) << 3;
					rgb.green = (((data[index+1] & 0x7) << 3) | (data[index] & 0xE0) >> 5) << 2;
					rgb.red   = (data[index+1] & 0xF8);
				}
				break;
				case PixelFormat::BGR24:
				{
					int index = lineLength * ySource + (xSource << 1) + xSource;
					rgb.blue  = data[index  ];
					rgb.green = data[index+1];
					rgb.red   = data[index+2];
				}
				break;
				case PixelFormat::RGB32:
				{
					int index = lineLength * ySource + (xSource << 2);
					rgb.red   = data[index  ];
					rgb.green = data[index+1];
					rgb.blue  = data[index+2];
				}
				break;
				case PixelFormat::BGR32:
				{
					int index = lineLength * ySource + (xSource << 2);
					rgb.blue  = data[index  ];
					rgb.green = data[index+1];
					rgb.red   = data[index+2];
				}
				break;
#ifdef HAVE_JPEG_DECODER
				case PixelFormat::MJPEG:
				break;
#endif
				case PixelFormat::NO_CHANGE:
					Error(Logger::getInstance("ImageResampler"), "Invalid pixel format given");
				break;
			}
		}
	}
}

void ImageResampler::processImageHDR2SDR(const uint8_t * data, int width, int height, int lineLength, PixelFormat pixelFormat, unsigned char *lutBuffer, Image<ColorRgb> &outputImage) const
{
	uint8_t _red, _green, _blue;
	size_t ind_lutd;
	int cropRight  = _cropRight;
	int cropBottom = _cropBottom;

	// handle 3D mode
	switch (_videoMode)
	{
	case VideoMode::VIDEO_3DSBS:
		cropRight = width >> 1;
		break;
	case VideoMode::VIDEO_3DTAB:
		cropBottom = width >> 1;
		break;
	default:
		break;
	}

	// calculate the output size
	int outputWidth = (width - _cropLeft - cropRight - (_horizontalDecimation >> 1) + _horizontalDecimation - 1) / _horizontalDecimation;
	int outputHeight = (height - _cropTop - cropBottom - (_verticalDecimation >> 1) + _verticalDecimation - 1) / _verticalDecimation;

	if (outputImage.width() != unsigned(outputWidth) || outputImage.height() != unsigned(outputHeight))
		outputImage.resize(outputWidth, outputHeight);

	for (int yDest = 0, ySource = _cropTop + (_verticalDecimation >> 1); yDest < outputHeight; ySource += _verticalDecimation, ++yDest)
	{
		for (int xDest = 0, xSource = _cropLeft + (_horizontalDecimation >> 1); xDest < outputWidth; xSource += _horizontalDecimation, ++xDest)
		{
			ColorRgb & rgb = outputImage(xDest, yDest);

			switch (pixelFormat)
			{
				case PixelFormat::UYVY:
				{
					int index = lineLength * ySource + (xSource << 1);
					uint8_t y = data[index+1];
					uint8_t u = ((xSource&1) == 0) ? data[index  ] : data[index-2];
					uint8_t v = ((xSource&1) == 0) ? data[index+2] : data[index  ];
					yuv2rgbHDR2SDR(y, u, v, rgb.red, rgb.green, rgb.blue, lutBuffer);
				}
				break;
				case PixelFormat::YUYV:
				{
					int index = lineLength * ySource + (xSource << 1);
					uint8_t y = data[index];
					uint8_t u = ((xSource&1) == 0) ? data[index+1] : data[index-1];
					uint8_t v = ((xSource&1) == 0) ? data[index+3] : data[index+1];
					yuv2rgbHDR2SDR(y, u, v, rgb.red, rgb.green, rgb.blue, lutBuffer);
				}
				break;
				case PixelFormat::BGR16:
				{
					int index = lineLength * ySource + (xSource << 1);
					_blue  = (data[index] & 0x1f) << 3;
					_green = (((data[index+1] & 0x7) << 3) | (data[index] & 0xE0) >> 5) << 2;
					_red   = (data[index+1] & 0xF8);
					
					// HDR mapping
					ind_lutd = (LUTD_Y_STRIDE(_red) + LUTD_U_STRIDE(_green) + LUTD_V_STRIDE(_blue));	
					rgb.red = lutBuffer[ind_lutd + LUTD_C_STRIDE(0)];
					rgb.green = lutBuffer[ind_lutd + LUTD_C_STRIDE(1)];
					rgb.blue = lutBuffer[ind_lutd + LUTD_C_STRIDE(2)];				
				}
				break;
				case PixelFormat::BGR24:
				{
					int index = lineLength * ySource + (xSource << 1) + xSource;
					_blue  = data[index  ];
					_green = data[index+1];
					_red   = data[index+2];
					
					// HDR mapping
					ind_lutd = (LUTD_Y_STRIDE(_red) + LUTD_U_STRIDE(_green) + LUTD_V_STRIDE(_blue));
					rgb.red = lutBuffer[ind_lutd + LUTD_C_STRIDE(0)];
					rgb.green = lutBuffer[ind_lutd + LUTD_C_STRIDE(1)];
					rgb.blue = lutBuffer[ind_lutd + LUTD_C_STRIDE(2)];					
				}
				break;
				case PixelFormat::RGB32:
				{
					int index = lineLength * ySource + (xSource << 2);
					_red   = data[index  ];
					_green = data[index+1];
					_blue  = data[index+2];
					
					// HDR mapping
					ind_lutd = (LUTD_Y_STRIDE(_red) + LUTD_U_STRIDE(_green) + LUTD_V_STRIDE(_blue));	
					rgb.red = lutBuffer[ind_lutd + LUTD_C_STRIDE(0)];
					rgb.green = lutBuffer[ind_lutd + LUTD_C_STRIDE(1)];
					rgb.blue = lutBuffer[ind_lutd + LUTD_C_STRIDE(2)];				
				}
				break;
				case PixelFormat::BGR32:
				{
					int index = lineLength * ySource + (xSource << 2);
					_blue  = data[index  ];
					_green = data[index+1];
					_red   = data[index+2];
					
					// HDR mapping
					ind_lutd = (LUTD_Y_STRIDE(_red) + LUTD_U_STRIDE(_green) + LUTD_V_STRIDE(_blue));					
					rgb.red = lutBuffer[ind_lutd + LUTD_C_STRIDE(0)];
					rgb.green = lutBuffer[ind_lutd + LUTD_C_STRIDE(1)];
					rgb.blue = lutBuffer[ind_lutd + LUTD_C_STRIDE(2)];					
				}
				break;
#ifdef HAVE_JPEG_DECODER
				case PixelFormat::MJPEG:
				break;
#endif
				case PixelFormat::NO_CHANGE:
					Error(Logger::getInstance("ImageResampler"), "Invalid pixel format given");
				break;
			}
		}
	}
}

uint8_t ImageResampler::clamp(int x)
{
	return (x<0) ? 0 : ((x>255) ? 255 : uint8_t(x));
}

void ImageResampler::yuv2rgb(uint8_t y, uint8_t u, uint8_t v, uint8_t &r, uint8_t &g, uint8_t &b)
{
	// see: http://en.wikipedia.org/wiki/YUV#Y.27UV444_to_RGB888_conversion
	int c = y - 16;
	int d = u - 128;
	int e = v - 128;

	r = clamp((298 * c + 409 * e + 128) >> 8);
	g = clamp((298 * c - 100 * d - 208 * e + 128) >> 8);
	b = clamp((298 * c + 516 * d + 128) >> 8);
}


void ImageResampler::yuv2rgbHDR2SDR(uint8_t y, uint8_t u, uint8_t v, uint8_t &r, uint8_t &g, uint8_t &b, unsigned char *lutBuffer)
{
	// see: http://en.wikipedia.org/wiki/YUV#Y.27UV444_to_RGB888_conversion
	int c = y - 16;
	int d = u - 128;
	int e = v - 128;
	
	

	uint8_t _r = clamp((298 * c + 409 * e + 128) >> 8);
	uint8_t _g = clamp((298 * c - 100 * d - 208 * e + 128) >> 8);
	uint8_t _b = clamp((298 * c + 516 * d + 128) >> 8);
	
	size_t ind_lutd = (
						LUTD_Y_STRIDE(_r) +
						LUTD_U_STRIDE(_g) +
						LUTD_V_STRIDE(_b)
					);
					
	r = lutBuffer[ind_lutd + LUTD_C_STRIDE(0)];
	g = lutBuffer[ind_lutd + LUTD_C_STRIDE(1)];
	b = lutBuffer[ind_lutd + LUTD_C_STRIDE(2)];					
}
