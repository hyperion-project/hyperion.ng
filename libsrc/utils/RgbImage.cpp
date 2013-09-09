
// STL includes
#include <cassert>
#include <cstring>

// hyperion Utils includes
#include <utils/RgbImage.h>


RgbImage::RgbImage(const unsigned width, const unsigned height, const RgbColor background) :
	_width(width),
	_height(height),
	mColors(new RgbColor[width*height])
{
	for (unsigned i=0; i<width*height; ++i)
	{
		mColors[i] = background;
	}
}

RgbImage::~RgbImage()
{
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
	assert(x < _width);
	assert(y < _height);

	const unsigned index = toIndex(x, y);
	return mColors[index];
}

RgbColor& RgbImage::operator()(const unsigned x, const unsigned y)
{
	// Debug-mode sanity check on given index
	assert(x < _width);
	assert(y < _height);

	const unsigned index = toIndex(x, y);
	return mColors[index];
}
