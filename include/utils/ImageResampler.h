#pragma once

#include <utils/VideoMode.h>
#include <utils/PixelFormat.h>
#include <utils/Image.h>
#include <utils/ColorRgb.h>

class ImageResampler
{
public:
	ImageResampler();
	~ImageResampler();

	void setHorizontalPixelDecimation(int decimator);

	void setVerticalPixelDecimation(int decimator);

	void setCropping(int cropLeft,
					 int cropRight,
					 int cropTop,
					 int cropBottom);

	void setVideoMode(VideoMode mode);

	void processImage(const uint8_t * data, int width, int height, int lineLength, PixelFormat pixelFormat, Image<ColorRgb> & outputImage) const;

private:
	static inline uint8_t clamp(int x);
	static void yuv2rgb(uint8_t y, uint8_t u, uint8_t v, uint8_t & r, uint8_t & g, uint8_t & b);

private:
	int _horizontalDecimation;
	int _verticalDecimation;
	int _cropLeft;
	int _cropRight;
	int _cropTop;
	int _cropBottom;
	VideoMode _videoMode;
};
