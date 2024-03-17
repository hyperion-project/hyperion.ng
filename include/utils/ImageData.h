#pragma once

// STL includes
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <utils/ColorRgb.h>

// QT includes
#include <QSharedData>

// https://docs.microsoft.com/en-us/windows/win32/winprog/windows-data-types#ssize-t
#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

template <typename Pixel_T>
class ImageData : public QSharedData
{
public:
	typedef Pixel_T pixel_type;

	ImageData(int width, int height, const Pixel_T background) :
		_width(width),
		_height(height),
		_pixels(new Pixel_T[static_cast<size_t>(width) * static_cast<size_t>(height)])
	{
		std::fill(_pixels, _pixels + width * height, background);
	}

	ImageData(const ImageData & other) :
		QSharedData(other),
		_width(other._width),
		_height(other._height),
		_pixels(new Pixel_T[static_cast<size_t>(other._width) * static_cast<size_t>(other._height)])
	{
		memcpy(_pixels, other._pixels, static_cast<size_t>(other._width) * static_cast<size_t>(other._height) * sizeof(Pixel_T));
	}

	ImageData& operator=(ImageData rhs)
	{
		rhs.swap(*this);
		return *this;
	}

	void swap(ImageData& s) noexcept
	{
		using std::swap;
		swap(this->_width, s._width);
		swap(this->_height, s._height);
		swap(this->_pixels, s._pixels);
	}

	ImageData(ImageData&& src) noexcept
		: _width(0)
		, _height(0)
		, _pixels(NULL)
	{
		src.swap(*this);
	}

	ImageData& operator=(ImageData&& src) noexcept
	{
		src.swap(*this);
		return *this;
	}

	~ImageData()
	{
		delete[] _pixels;
	}

	inline int width() const
	{
		return _width;
	}

	inline int height() const
	{
		return _height;
	}

	uint8_t red(int pixel) const
	{
		return (_pixels + pixel)->red;
	}

	uint8_t green(int pixel) const
	{
		return (_pixels + pixel)->green;
	}

	uint8_t blue(int pixel) const
	{
		return (_pixels + pixel)->blue;
	}

	const Pixel_T& operator()(int x, int y) const
	{
		return _pixels[toIndex(x,y)];
	}

	Pixel_T& operator()(int x, int y)
	{
		return _pixels[toIndex(x,y)];
	}

	void resize(int width, int height)
	{
		if (width == _width && height == _height)
		{
			return;
		}

		// Allocate a new buffer without initializing the content
		Pixel_T* newPixels = new Pixel_T[static_cast<size_t>(width) * static_cast<size_t>(height)];

		// Release the old buffer without copying data
		delete[] _pixels;

		// Update the pointer to the new buffer
		_pixels = newPixels;

		_width = width;
		_height = height;
	}

	Pixel_T* memptr()
	{
		return _pixels;
	}

	const Pixel_T* memptr() const
	{
		return _pixels;
	}

	void toRgb(ImageData<ColorRgb>& image) const
	{
		if (image.width() != _width || image.height() != _height)
		{
			image.resize(_width, _height);
		}

		const int imageSize = _width * _height;

		for (int idx = 0; idx < imageSize; idx++)
		{
			const Pixel_T & color = _pixels[idx];
			image.memptr()[idx] = ColorRgb{color.red, color.green, color.blue};
		}
	}

	ssize_t size() const
	{
		return  static_cast<ssize_t>(_width) * static_cast<ssize_t>(_height) * sizeof(Pixel_T);
	}

	void clear()
	{
		if (_width != 1 || _height != 1)
		{
			resize(1,1);
		}
		// Set the single pixel to the default background
		_pixels[0] = Pixel_T();
	}

private:
	inline int toIndex(int x, int y) const
	{
		return y * _width + x;
	}

	/// The width of the image
	int _width;
	/// The height of the image
	int _height;
	/// The pixels of the image
	Pixel_T* _pixels;
};
