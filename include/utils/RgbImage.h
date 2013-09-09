
#pragma once

#include <cassert>
#include <cstring>
#include <vector>

// Local includes
#include "RgbColor.h"

///
/// The RgbImage holds a 2D matrix of RgbColors's (or image). Width and height of the image are
/// fixed at construction.
///
class RgbImage
{
public:

	///
	/// Constructor for an image with specified width and height
	///
	/// @param width The width of the image
	/// @param height The height of the image
	/// @param background The color of the image (default = BLACK)
	///
	RgbImage(const unsigned width, const unsigned height, const RgbColor background = RgbColor::BLACK);

	///
	/// Destructor
	///
	~RgbImage();

	///
	/// Returns the width of the image
	///
	/// @return The width of the image
	///
	inline unsigned width() const
	{
		return _width;
	}

	///
	/// Returns the height of the image
	///
	/// @return The height of the image
	///
	inline unsigned height() const
	{
		return _height;
	}

	///
	/// Sets the color of a specific pixel in the image
	///
	/// @param x The x index
	/// @param y The y index
	/// @param color The new color
	///
	void setPixel(const unsigned x, const unsigned y, const RgbColor color);

	///
	/// Returns a const reference to a specified pixel in the image
	///
	/// @param x The x index
	/// @param y The y index
	///
	/// @return const reference to specified pixel
	///
	const RgbColor& operator()(const unsigned x, const unsigned y) const;

	///
	/// Returns a reference to a specified pixel in the image
	///
	/// @param x The x index
	/// @param y The y index
	///
	/// @return reference to specified pixel
	///
	RgbColor& operator()(const unsigned x, const unsigned y);

	///
	/// Copies another image into this image. The images should have exactly the same size.
	///
	/// @param other The image to copy into this
	///
	inline void copy(const RgbImage& other)
	{
		assert(other._width == _width);
		assert(other._height == _height);

		memcpy(mColors, other.mColors, _width*_height*sizeof(RgbColor));
	}

	///
	/// Returns a memory pointer to the first pixel in the image
	/// @return The memory pointer to the first pixel
	///
	RgbColor* memptr()
	{
		return mColors;
	}

	///
	/// Returns a const memory pointer to the first pixel in the image
	/// @return The const memory pointer to the first pixel
	///
	const RgbColor* memptr() const
	{
		return mColors;
	}
private:

	///
	/// Translate x and y coordinate to index of the underlying vector
	///
	/// @param x The x index
	/// @param y The y index
	///
	/// @return The index into the underlying data-vector
	///
	inline unsigned toIndex(const unsigned x, const unsigned y) const
	{
		return y*_width + x;
	}

private:
	/// The width of the image
	const unsigned _width;
	/// The height of the image
	const unsigned _height;

	/** The colors of the image */
	RgbColor* mColors;
};
