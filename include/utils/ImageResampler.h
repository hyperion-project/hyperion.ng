#pragma once

#include <utils/VideoMode.h>
#include <utils/PixelFormat.h>
#include <utils/Image.h>
#include <utils/ColorRgb.h>


// some stuff for HDR tone mapping
#define IMG_BITS 8
#define LUTD_BITS IMG_BITS
#define LUTD_Y_STRIDE(y) (y << (0 * LUTD_BITS)) // Y: Shift by (2**LUTD_BITS)**0
#define LUTD_U_STRIDE(u) (u << (1 * LUTD_BITS)) // U: Shift by (2**LUTD_BITS)**1
#define LUTD_V_STRIDE(v) (v << (2 * LUTD_BITS)) // V: Shift by (2**LUTD_BITS)**2
#define LUTD_C_STRIDE(c) (c << (3 * LUTD_BITS)) // C: Shift by (2**LUTD_BITS)**3

class ImageResampler
{
public:
	ImageResampler();
	~ImageResampler();

	void setHorizontalPixelDecimation(int decimator);
	void setVerticalPixelDecimation(int decimator);
	void setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom);
	void setVideoMode(VideoMode mode);
	void processImage(const uint8_t * data, int width, int height, int lineLength, PixelFormat pixelFormat, Image<ColorRgb> & outputImage) const;
	void processImageHDR2SDR(const uint8_t * data, int width, int height, int lineLength, PixelFormat pixelFormat, unsigned char *lutBuffer, Image<ColorRgb> &outputImage) const;

private:
	int _horizontalDecimation;
	int _verticalDecimation;
	int _cropLeft;
	int _cropRight;
	int _cropTop;
	int _cropBottom;
	VideoMode _videoMode;
};

