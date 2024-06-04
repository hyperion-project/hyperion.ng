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
	, _bottomUp(false)
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

void ImageResampler::processImage(const uint8_t * data, int width, int height, int lineLength, PixelFormat pixelFormat, Image<ColorRgb> &outputImage) const
{
	int cropLeft = _cropLeft;
	int cropRight  = _cropRight;
	int cropTop = _cropTop;
	int cropBottom = _cropBottom;

	// handle 3D mode
	switch (_videoMode)
	{
	case VideoMode::VIDEO_3DSBS:
		cropRight =  (width >> 1) + (cropRight >> 1);
		cropLeft = cropLeft >> 1;
		break;
	case VideoMode::VIDEO_3DTAB:
		cropBottom = (height >> 1) + (cropBottom >> 1);
		cropTop = cropTop >> 1;
		break;
	default:
		break;
	}

	// calculate the output size
	int outputWidth = (width - cropLeft - cropRight - (_horizontalDecimation >> 1) + _horizontalDecimation - 1) / _horizontalDecimation;
	int outputHeight = (height - cropTop - cropBottom - (_verticalDecimation >> 1) + _verticalDecimation - 1) / _verticalDecimation;

	outputImage.resize(outputWidth, outputHeight);

	int xDestStart, xDestEnd;
	int yDestStart, yDestEnd;

	if (bottomUp)
	{
		if (_flipMode == FlipMode::NO_CHANGE)
			_flipMode = FlipMode::HORIZONTAL;
		else if (_flipMode == FlipMode::HORIZONTAL)
			_flipMode = FlipMode::NO_CHANGE;
		else if (_flipMode == FlipMode::VERTICAL)
			_flipMode = FlipMode::BOTH;
		else if (_flipMode == FlipMode::BOTH)
			_flipMode = FlipMode::VERTICAL;
	}

	switch (_flipMode)
	{
		case FlipMode::NO_CHANGE:
			xDestStart = 0;
			xDestEnd = outputWidth-1;
			yDestStart = 0;
			yDestEnd = outputHeight-1;
			break;
		case FlipMode::HORIZONTAL:
			xDestStart = 0;
			xDestEnd = outputWidth-1;
			yDestStart = -(outputHeight-1);
			yDestEnd = 0;
			break;
		case FlipMode::VERTICAL:
			xDestStart = -(outputWidth-1);
			xDestEnd = 0;
			yDestStart = 0;
			yDestEnd = outputHeight-1;
			break;
		case FlipMode::BOTH:
			xDestStart = -(outputWidth-1);
			xDestEnd = 0;
			yDestStart = -(outputHeight-1);
			yDestEnd = 0;
			break;
	}

	switch (pixelFormat)
	{
		case PixelFormat::UYVY:
		{
			for (int yDest = yDestStart, ySource = cropTop + (_verticalDecimation >> 1); yDest <= yDestEnd; ySource += _verticalDecimation, ++yDest)
			{
				for (int xDest = xDestStart, xSource = cropLeft + (_horizontalDecimation >> 1); xDest <= xDestEnd; xSource += _horizontalDecimation, ++xDest)
				{
					ColorRgb & rgb = outputImage(abs(xDest), abs(yDest));
					int index = lineLength * ySource + (xSource << 1);
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
			for (int yDest = yDestStart, ySource = cropTop + (_verticalDecimation >> 1); yDest <= yDestEnd; ySource += _verticalDecimation, ++yDest)
			{
				for (int xDest = xDestStart, xSource = cropLeft + (_horizontalDecimation >> 1); xDest <= xDestEnd; xSource += _horizontalDecimation, ++xDest)
				{
					ColorRgb & rgb = outputImage(abs(xDest), abs(yDest));
					int index = lineLength * ySource + (xSource << 1);
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
			for (int yDest = yDestStart, ySource = cropTop + (_verticalDecimation >> 1); yDest <= yDestEnd; ySource += _verticalDecimation, ++yDest)
			{
				for (int xDest = xDestStart, xSource = cropLeft + (_horizontalDecimation >> 1); xDest <= xDestEnd; xSource += _horizontalDecimation, ++xDest)
				{
					ColorRgb & rgb = outputImage(abs(xDest), abs(yDest));
					int index = lineLength * ySource + (xSource << 1);
					rgb.blue  = (data[index] & 0x1f) << 3;
					rgb.green = (((data[index+1] & 0x7) << 3) | (data[index] & 0xE0) >> 5) << 2;
					rgb.red   = (data[index+1] & 0xF8);
				}
			}
			break;
		}

		case PixelFormat::RGB24:
		{
			for (int yDest = yDestStart, ySource = cropTop + (_verticalDecimation >> 1); yDest <= yDestEnd; ySource += _verticalDecimation, ++yDest)
			{
				for (int xDest = xDestStart, xSource = cropLeft + (_horizontalDecimation >> 1); xDest <= xDestEnd; xSource += _horizontalDecimation, ++xDest)
				{
					ColorRgb & rgb = outputImage(abs(xDest), abs(yDest));
					int index = lineLength * ySource + (xSource << 1) + xSource;
					rgb.red   = data[index  ];
					rgb.green = data[index+1];
					rgb.blue  = data[index+2];
				}
			}
			break;
		}

		case PixelFormat::BGR24:
		{
			for (int yDest = yDestStart, ySource = cropTop + (_verticalDecimation >> 1); yDest <= yDestEnd; ySource += _verticalDecimation, ++yDest)
			{
				for (int xDest = xDestStart, xSource = cropLeft + (_horizontalDecimation >> 1); xDest <= xDestEnd; xSource += _horizontalDecimation, ++xDest)
				{
					ColorRgb & rgb = outputImage(abs(xDest), abs(yDest));
					int index = lineLength * ySource + (xSource << 1) + xSource;
					rgb.blue  = data[index  ];
					rgb.green = data[index+1];
					rgb.red   = data[index+2];
				}
			}
			break;
		}

		case PixelFormat::RGB32:
		{
			for (int yDest = yDestStart, ySource = cropTop + (_verticalDecimation >> 1); yDest <= yDestEnd; ySource += _verticalDecimation, ++yDest)
			{
				for (int xDest = xDestStart, xSource = cropLeft + (_horizontalDecimation >> 1); xDest <= xDestEnd; xSource += _horizontalDecimation, ++xDest)
				{
					ColorRgb & rgb = outputImage(abs(xDest), abs(yDest));
					int index = lineLength * ySource + (xSource << 2);
					rgb.red   = data[index  ];
					rgb.green = data[index+1];
					rgb.blue  = data[index+2];
				}
			}
			break;
		}

		case PixelFormat::BGR32:
		{
			for (int yDest = yDestStart, ySource = cropTop + (_verticalDecimation >> 1); yDest <= yDestEnd; ySource += _verticalDecimation, ++yDest)
			{
				for (int xDest = xDestStart, xSource = cropLeft + (_horizontalDecimation >> 1); xDest <= xDestEnd; xSource += _horizontalDecimation, ++xDest)
				{
					ColorRgb & rgb = outputImage(abs(xDest), abs(yDest));
					int index = lineLength * ySource + (xSource << 2);
					rgb.blue  = data[index  ];
					rgb.green = data[index+1];
					rgb.red   = data[index+2];
				}
			}
			break;
		}

		case PixelFormat::NV12:
		{
			for (int yDest = yDestStart, ySource = cropTop + (_verticalDecimation >> 1); yDest <= yDestEnd; ySource += _verticalDecimation, ++yDest)
			{
				int uOffset = (height + ySource / 2) * lineLength;
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

		case PixelFormat::I420:
		{
			for (int yDest = yDestStart, ySource = cropTop + (_verticalDecimation >> 1); yDest <= yDestEnd; ySource += _verticalDecimation, ++yDest)
			{
				int uOffset = width * height + (ySource/2) * width/2;
				int vOffset = width * height * 1.25 + (ySource/2) * width/2;
				for (int xDest = xDestStart, xSource = cropLeft + (_horizontalDecimation >> 1); xDest <= xDestEnd; xSource += _horizontalDecimation, ++xDest)
				{
					ColorRgb & rgb = outputImage(abs(xDest), abs(yDest));
					int y = data[lineLength * ySource + xSource];
					int u = data[uOffset + (xSource >> 1)];
					int v = data[vOffset + (xSource >> 1)];
					ColorSys::yuv2rgb(y, u, v, rgb.red, rgb.green, rgb.blue);
				}
			}
			break;
		}
		case PixelFormat::MJPEG:
		break;
		case PixelFormat::NO_CHANGE:
			Error(Logger::getInstance("ImageResampler"), "Invalid pixel format given");
		break;
	}
}
