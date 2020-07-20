#pragma once

// STL includes
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <cassert>
#include <type_traits>
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

	ImageData(const unsigned width, const unsigned height, const Pixel_T background) :
		_width(width),
		_height(height),
		_pixels(new Pixel_T[width * height + 1])
	{
		std::fill(_pixels, _pixels + width * height, background);
	}

	ImageData(const ImageData & other) :
		QSharedData(other),
		_width(other._width),
		_height(other._height),
		_pixels(new Pixel_T[other._width * other._height + 1])
	{
		memcpy(_pixels, other._pixels, (long) other._width * other._height * sizeof(Pixel_T));
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

	inline unsigned width() const
	{
		return _width;
	}

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

	const Pixel_T& operator()(const unsigned x, const unsigned y) const
	{
		return _pixels[toIndex(x,y)];
	}

	Pixel_T& operator()(const unsigned x, const unsigned y)
	{
		return _pixels[toIndex(x,y)];
	}

	void resize(const unsigned width, const unsigned height)
	{
		if (width == _width && height == _height)
			return;

		if ((width * height) > unsigned((_width * _height)))
		{
			delete[] _pixels;
			_pixels = new Pixel_T[width*height + 1];
		}

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

	void toRgb(ImageData<ColorRgb>& image)
	{
		if (image.width() != _width || image.height() != _height)
			image.resize(_width, _height);

		const unsigned imageSize = _width * _height;

		for (unsigned idx = 0; idx < imageSize; idx++)
		{
			const Pixel_T & color = _pixels[idx];
			image.memptr()[idx] = ColorRgb{color.red, color.green, color.blue};
		}
	}

	ssize_t size() const
	{
		return  (ssize_t) _width * _height * sizeof(Pixel_T);
	}

	void clear()
	{
		if (_width != 1 || _height != 1)
		{
			_width = 1;
			_height = 1;
			delete[] _pixels;
			_pixels = new Pixel_T[2];
		}

		memset(_pixels, 0, (unsigned long) _width * _height * sizeof(Pixel_T));
	}

private:
	inline unsigned toIndex(const unsigned x, const unsigned y) const
	{
		return y * _width + x;
	}

private:
	/// The width of the image
	unsigned _width;
	/// The height of the image
	unsigned _height;
	/// The pixels of the image
	Pixel_T* _pixels;
};
