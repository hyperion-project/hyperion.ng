#pragma once

#include <utils/VideoMode.h>
#include <utils/PixelFormat.h>
#include <utils/Image.h>
#include <utils/ColorRgb.h>

class ImageResampler
{
public:
	ImageResampler();
	~ImageResampler() {}

	void setHorizontalPixelDecimation(int decimator) { _horizontalDecimation = decimator; }
	void setVerticalPixelDecimation(int decimator) { _verticalDecimation = decimator; }
	void setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom);
	void setVideoMode(VideoMode mode) { _videoMode = mode; }
	void setFlipMode(FlipMode mode) { _flipMode = mode; }
	void processImage(const uint8_t * data, int width, int height, int lineLength, PixelFormat pixelFormat, Image<ColorRgb> & outputImage) const;

private:
	int _horizontalDecimation;
	int _verticalDecimation;
	int _cropLeft;
	int _cropRight;
	int _cropTop;
	int _cropBottom;
	VideoMode _videoMode;
	FlipMode _flipMode;
};

