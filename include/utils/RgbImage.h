
#pragma once

#include <cassert>
#include <cstring>

// Local includes
#include "RgbColor.h"

class RgbImage
{
public:

	RgbImage(const unsigned width, const unsigned height, const RgbColor background = RgbColor::BLACK);

	~RgbImage();

	inline unsigned width() const
	{
		return mWidth;
	}

	inline unsigned height() const
	{
		return mHeight;
	}

	void setPixel(const unsigned x, const unsigned y, const RgbColor color);

	const RgbColor& operator()(const unsigned x, const unsigned y) const;

	RgbColor& operator()(const unsigned x, const unsigned y);

	inline void copy(const RgbImage& other)
	{
		std::cout << "This image size: [" << width() << "x" << height() << "]. Other image size: [" << other.width() << "x" << other.height() << "]" << std::endl;
		assert(other.mWidth == mWidth);
		assert(other.mHeight == mHeight);

		memcpy(mColors, other.mColors, mWidth*mHeight*sizeof(RgbColor));
	}

private:

	inline unsigned toIndex(const unsigned x, const unsigned y) const
	{
		return y*mWidth + x;
	}

private:
	unsigned mWidth;
	unsigned mHeight;

	/** The colors of the image */
	RgbColor* mColors;
};
