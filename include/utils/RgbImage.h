
#pragma once

#include <cassert>
#include <cstring>
#include <vector>

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
		assert(other.mWidth == mWidth);
		assert(other.mHeight == mHeight);

		memcpy(mColors, other.mColors, mWidth*mHeight*sizeof(RgbColor));
	}

	RgbColor* memptr()
	{
		return mColors;
	}

private:

	inline unsigned toIndex(const unsigned x, const unsigned y) const
	{
		return y*mWidth + x;
	}

private:
	const unsigned mWidth;
	const unsigned mHeight;

	/** The colors of the image */
	RgbColor* mColors;
};
