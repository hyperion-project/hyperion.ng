#pragma once

// STL includes
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <utils/ColorRgb.h>
#include <utils/Logger.h>

// QT includes
#include <QSharedData>
#include <QAtomicInt>

// https://docs.microsoft.com/en-us/windows/win32/winprog/windows-data-types#ssize-t
#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#define IMAGEDATA_ENABLE_MEMORY_TRACKING_ALLOC 0
#define IMAGEDATA_ENABLE_MEMORY_TRACKING_DEEP 0
#define IMAGEDATA_ENABLE_MEMORY_TRACKING_RELEASE 0

namespace {
// Counter for unique image data instance IDs
QAtomicInteger<quint64> imageData_instance_counter(0);
}

template <typename Pixel_T>
class ImageData : public QSharedData
{
public:
	typedef Pixel_T pixel_type;

	ImageData(int width, int height, const Pixel_T background) :
		_width(width),
		_height(height),
		_pixels(new Pixel_T[static_cast<size_t>(width) * static_cast<size_t>(height)]),
		_instanceId(++imageData_instance_counter)
	{
		std::fill(_pixels, _pixels + width * height, background);

		DebugIf(IMAGEDATA_ENABLE_MEMORY_TRACKING_ALLOC, Logger::getInstance("MEMORY-ImageData"), "ALLOC (DATA): New ImageData [%d] created (%dx%d).", _instanceId, width, height);
	}

	// Copy constructor for deep copies (for detach)
	ImageData(const ImageData& other) :
		_width(other._width),
		_height(other._height),
		_pixels(new Pixel_T[static_cast<size_t>(other._width) * static_cast<size_t>(other._height)]),
		_instanceId(++imageData_instance_counter)
	{
		memcpy(_pixels, other._pixels, static_cast<size_t>(other._width) * static_cast<size_t>(other._height) * sizeof(Pixel_T));

		DebugIf(IMAGEDATA_ENABLE_MEMORY_TRACKING_DEEP, Logger::getInstance("MEMORY-ImageData"), "COPY (DEEP DATA): New ImageData [%d] created as a deep copy of [%d].", _instanceId, other._instanceId);
	}

	~ImageData()
	{
		delete[] _pixels;
		DebugIf(IMAGEDATA_ENABLE_MEMORY_TRACKING_RELEASE, Logger::getInstance("MEMORY-ImageData"), "RELEASE (DATA): ImageData [%d] destroyed and memory freed.", _instanceId);
	}

	ImageData& operator=(ImageData rhs)
	{
		rhs.swap(*this);
		return *this;
	}

	void swap(ImageData& src) noexcept
	{
		using std::swap;
		swap(this->_width, src._width);
		swap(this->_height, src._height);
		swap(this->_pixels, src._pixels);
		swap(this->_instanceId, src._instanceId);
	}

	ImageData(ImageData&& src) noexcept
		: _width(0)
		, _height(0)
		, _pixels(nullptr)
		, _instanceId(0)
	{
		src.swap(*this);
	}

	// Check reference count
	int refCount() const { return this->ref.loadRelaxed(); }

	int width() const
	{
		return _width;
	}

	int height() const
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
		return _pixels[y * _width + x];
	}

	Pixel_T& operator()(int x, int y)
	{
		return _pixels[y * _width + x];
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
			const Pixel_T& color = _pixels[idx];
			image.memptr()[idx] = ColorRgb{ color.red, color.green, color.blue };
		}
	}

	ssize_t size() const
	{
		return  static_cast<ssize_t>(_width) * static_cast<ssize_t>(_height) * sizeof(Pixel_T);
	}

	void clear()
	{
		// Fill the entire existing pixel buffer with the default-constructed pixel value
		std::fill(_pixels, _pixels + (static_cast<size_t>(_width) * _height), Pixel_T());
	}

	void reset()
	{
		if (_width != 1 || _height != 1)
		{
			resize(1, 1);
		}
		// Set the single pixel to the default background
		_pixels[0] = Pixel_T();
	}

private:
	int toIndex(int x, int y) const
	{
		return y * _width + x;
	}

	/// The width of the image
	int _width;
	/// The height of the image
	int _height;
	/// The pixels of the image
	Pixel_T* _pixels;

	quint64 _instanceId; // Unique ID for this data block
};
