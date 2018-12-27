#pragma once

// STL includes
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <cassert>
#include <utils/ColorRgb.h>


template <typename Pixel_T>
class Image
{
public:

	typedef Pixel_T pixel_type;

	///
	/// Default constructor for an image
	///
	Image() :
		_width(1),
		_height(1),
		_pixels(new Pixel_T[2]),
		_endOfPixels(_pixels + 1)
	{
		memset(_pixels, 0, 2*sizeof(Pixel_T));
	}

	///
	/// Constructor for an image with specified width and height
	///
	/// @param width The width of the image
	/// @param height The height of the image
	///
	Image(const unsigned width, const unsigned height) :
		_width(width),
		_height(height),
		_pixels(new Pixel_T[width * height + 1]),
		_endOfPixels(_pixels + width * height)
	{
		memset(_pixels, 0, (_width*_height+1)*sizeof(Pixel_T));
	}

	///
	/// Constructor for an image with specified width and height
	///
	/// @param width The width of the image
	/// @param height The height of the image
	/// @param background The color of the image
	///
	Image(const unsigned width, const unsigned height, const Pixel_T background) :
		_width(width),
		_height(height),
		_pixels(new Pixel_T[width * height + 1]),
		_endOfPixels(_pixels + width * height)
	{
		std::fill(_pixels, _endOfPixels, background);
	}

	///
	/// Copy constructor for an image
	///
	Image(const Image & other) :
		_width(other._width),
		_height(other._height),
		_pixels(new Pixel_T[other._width * other._height + 1]),
		_endOfPixels(_pixels + other._width * other._height)
	{
		memcpy(_pixels, other._pixels, other._width * other._height * sizeof(Pixel_T));
	}

	// Define assignment operator in terms of the copy constructor
	// More to read: https://stackoverflow.com/questions/255612/dynamically-allocating-an-array-of-objects?answertab=active#tab-top
	Image& operator=(Image rhs)
	{
		rhs.swap(*this);
		return *this;
	}

	void swap(Image& s) noexcept
	{
		using std::swap;
		swap(this->_width, s._width);
		swap(this->_height, s._height);
		swap(this->_pixels, s._pixels);
		swap(this->_endOfPixels, s._endOfPixels);
	}

	// C++11
	Image(Image&& src) noexcept
		: _width(0)
		, _height(0)
		, _pixels(NULL)
		, _endOfPixels(NULL)
	{
		src.swap(*this);
	}
	Image& operator=(Image&& src) noexcept
	{
		src.swap(*this);
		return *this;
	}

	///
	/// Destructor
	///
	~Image()
	{
		delete[] _pixels;
	}

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

	uint8_t red(const unsigned pixel) const
	{
		return (_pixels + pixel)->red;
	}

	uint8_t green(const unsigned pixel) const
	{
		return (_pixels + pixel)->green;
	}

	uint8_t blue(const unsigned pixel) const
	{
		return (_pixels + pixel)->blue;
	}

	///
	/// Returns a const reference to a specified pixel in the image
	///
	/// @param x The x index
	/// @param y The y index
	///
	/// @return const reference to specified pixel
	///
	const Pixel_T& operator()(const unsigned x, const unsigned y) const
	{
		return _pixels[toIndex(x,y)];
	}

	///
	/// Returns a reference to a specified pixel in the image
	///
	/// @param x The x index
	/// @param y The y index
	///
	/// @return reference to specified pixel
	///
	Pixel_T& operator()(const unsigned x, const unsigned y)
	{
		return _pixels[toIndex(x,y)];
	}

	/// Resize the image
	/// @param width The width of the image
	/// @param height The height of the image
	void resize(const unsigned width, const unsigned height)
	{
		if ((width*height) > unsigned((_endOfPixels-_pixels)))
		{
			delete[] _pixels;
			_pixels = new Pixel_T[width*height + 1];
			_endOfPixels = _pixels + width*height;
		}

		_width = width;
		_height = height;
	}

	///
	/// Copies another image into this image. The images should have exactly the same size.
	///
	/// @param other The image to copy into this
	///
	void copy(const Image<Pixel_T>& other)
	{
		assert(other._width == _width);
		assert(other._height == _height);

		memcpy(_pixels, other._pixels, _width*_height*sizeof(Pixel_T));
	}

	///
	/// Returns a memory pointer to the first pixel in the image
	/// @return The memory pointer to the first pixel
	///
	Pixel_T* memptr()
	{
		return _pixels;
	}

	///
	/// Returns a const memory pointer to the first pixel in the image
	/// @return The const memory pointer to the first pixel
	///
	const Pixel_T* memptr() const
	{
		return _pixels;
	}


	///
	/// Convert image of any color order to a RGB image.
	///
	/// @param[out] image  The image that buffers the output
	///
	void toRgb(Image<ColorRgb>& image)
	{
		image.resize(_width, _height);
		const unsigned imageSize = _width * _height;

		for (unsigned idx=0; idx<imageSize; idx++)
		{
			const Pixel_T color = memptr()[idx];
			image.memptr()[idx] = ColorRgb{color.red, color.green, color.blue};
		}
	}

	///
	/// get size of buffer
	//
	ssize_t size()
	{
		return  _width * _height * sizeof(Pixel_T);
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
	unsigned _width;
	/// The height of the image
	unsigned _height;

	/// The pixels of the image
	Pixel_T* _pixels;

	/// Pointer to the last(extra) pixel
	Pixel_T* _endOfPixels;
};
