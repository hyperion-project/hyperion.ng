
// STL includes
#include <cassert>
#include <cstring>

// hyperion Utils includes
#include <utils/RgbImage.h>


RgbImage::RgbImage(const unsigned width, const unsigned height, const RgbColor background) :
	mWidth(width),
	mHeight(height),
	mColors(nullptr)
{
	mColors = new RgbColor[width*height];
	for (RgbColor* color = mColors; color <= mColors+(mWidth*mHeight); ++color)
	{
		*color = background;
	}
}

RgbImage::~RgbImage()
{
	std::cout << "RgbImage(" << this << ") is being deleted" << std::endl;

	delete[] mColors;
}

void RgbImage::setPixel(const unsigned x, const unsigned y, const RgbColor color)
{
	// Debug-mode sanity check on given index
	(*this)(x,y) = color;
}

const RgbColor& RgbImage::operator()(const unsigned x, const unsigned y) const
{
	// Debug-mode sanity check on given index
	assert(x < mWidth);
	assert(y < mHeight);

	const unsigned index = toIndex(x, y);
	return mColors[index];
}

RgbColor& RgbImage::operator()(const unsigned x, const unsigned y)
{
	// Debug-mode sanity check on given index
	assert(x < mWidth);
	assert(y < mHeight);

	const unsigned index = toIndex(x, y);
	return mColors[index];
}
