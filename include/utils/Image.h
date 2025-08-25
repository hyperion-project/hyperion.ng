#pragma once

#include <QDebug>
#include <QSharedDataPointer>

#include <utils/ImageData.h>
#include <utils/Logger.h>

#define IMAGE_ENABLE_MEMORY_TRACKING_ALLOC 0
#define IMAGE_ENABLE_MEMORY_TRACKING_DEEP 0
#define IMAGE_ENABLE_MEMORY_TRACKING_SHALLOW 0
#define IMAGE_ENABLE_MEMORY_TRACKING_MOVE 0
#define IMAGE_ENABLE_MEMORY_TRACKING_RELEASE 0

// Static counter for unique HANDLE instance IDs
static quint64 image_instance_counter = 0;

template <typename Pixel_T>
class Image
{
public:
	typedef Pixel_T pixel_type;

	Image() :
		Image(1, 1, Pixel_T())
	{
	}

	Image(int width, int height) :
		Image(width, height, Pixel_T())
	{
	}

	///
	/// Constructor for an image with specified width and height
	///
	/// @param width The width of the image
	/// @param height The height of the image
	/// @param background The color of the image
	///
	Image(int width, int height, const Pixel_T background) :
		_d_ptr(new ImageData<Pixel_T>(width, height, background)),
		_instanceId(++image_instance_counter)
	{
		DebugIf(IMAGE_ENABLE_MEMORY_TRACKING_ALLOC, Logger::getInstance("MEMORY-Image"), "ALLOC (HANDLE): New Image handle [%d] created.", _instanceId);
	}

	///
	/// Copy constructor for an image
	/// @param other The image which will be copied
	///
	// Copy constructor (Shallow Copy)
	Image(const Image& other) :
		_d_ptr(other._d_ptr), // This just increments the ref-counter
		_instanceId(++image_instance_counter)
	{
		DebugIf(IMAGE_ENABLE_MEMORY_TRACKING_SHALLOW, Logger::getInstance("MEMORY-Image"), "COPY (SHALLOW HANDLE): Image handle [%d] created, sharing data with handle [%d].", _instanceId, other._instanceId);
	}

	// Copy assignment
	Image& operator=(Image rhs)
	{
		DebugIf(IMAGE_ENABLE_MEMORY_TRACKING_SHALLOW, Logger::getInstance("MEMORY-Image"), "ASSIGN (SHALLOW HANDLE): Image handle [%d] now points to data from another handle.", this->_instanceId);
		rhs.swap(*this);
		return *this;
	}

	void swap(Image& swap) noexcept
	{
		std::swap(this->_d_ptr, swap._d_ptr);
		std::swap(this->_instanceId, swap._instanceId);
	}

	// Move constructor
	Image(Image&& src) noexcept :
		_d_ptr(src._d_ptr),
		_instanceId(src._instanceId)
	{
		src._instanceId = 0; // Invalidate moved-from handle
		DebugIf(IMAGE_ENABLE_MEMORY_TRACKING_MOVE, Logger::getInstance("MEMORY-Image"), "MOVE: New Image handle [%d] has been moved into a new instance.", _instanceId);
	}

	///
	/// Destructor
	///
	~Image()
	{
		if (_instanceId == 0)
		{
			return;
		}

		if (isDetached())
		{
			DebugIf(IMAGE_ENABLE_MEMORY_TRACKING_RELEASE, Logger::getInstance("MEMORY-Image"), "RELEASE (HANDLE): Image handle [%d] destroyed. This was the last handle.", _instanceId);
		}
		else {
			DebugIf(IMAGE_ENABLE_MEMORY_TRACKING_RELEASE, Logger::getInstance("MEMORY-Image"), "RELEASE (HANDLE): Image handle [%d] destroyed. Other handles still exist.", _instanceId);
		}
	}

	// Check if the data is unique
	bool isDetached() const
	{
		return _d_ptr != nullptr && _d_ptr->refCount() == 1;
	}

	///
	/// Returns the width of the image
	///
	/// @return The width of the image
	///
	int width() const
	{
		return _d_ptr->width();
	}

	///
	/// Returns the height of the image
	///
	/// @return The height of the image
	///
	int height() const
	{
		return _d_ptr->height();
	}

	uint8_t red(unsigned pixel) const
	{
		return _d_ptr->red(pixel);
	}

	uint8_t green(unsigned pixel) const
	{
		return _d_ptr->green(pixel);
	}

	///
	/// Returns a const reference to a specified pixel in the image
	///
	/// @param x The x index
	/// @param y The y index
	///
	/// @return const reference to specified pixel
	///
	uint8_t blue(int pixel) const
	{
		return _d_ptr->blue(pixel);
	}

	///
	/// Returns a reference to a specified pixel in the image
	///
	/// @param x The x index
	/// @param y The y index
	const Pixel_T& operator()(int x, int y) const
	{
		return _d_ptr->operator()(x, y);
	}

	///
	/// @return reference to specified pixel
	///
	Pixel_T& operator()(int x, int y)
	{
		return _d_ptr->operator()(x, y);
	}

	/// Resize the image
	/// @param width The width of the image
	/// @param height The height of the image
	void resize(int width, int height)
	{
		_d_ptr->resize(width, height);
	}

	///
	/// Returns a memory pointer to the first pixel in the image
	/// @return The memory pointer to the first pixel
	///
	Pixel_T* memptr()
	{
		if (!isDetached())
		{
			DebugIf(IMAGE_ENABLE_MEMORY_TRACKING_MOVE, Logger::getInstance("MEMORY-Image"), "COPY (DEEP): memptr() on shared Image handle [%d] is causing a detach.", _instanceId);
		}
		_d_ptr.detach();
		return _d_ptr->memptr();
	}

	///
	/// Returns a const memory pointer to the first pixel in the image
	/// @return The const memory pointer to the first pixel
	///
	const Pixel_T* memptr() const
	{
		return _d_ptr->memptr();
	}

	///
	/// Convert image of any color order to a RGB image.
	///
	/// @param[out] image  The image that buffers the output
	///
	void toRgb(Image<ColorRgb>& image) const
	{
		_d_ptr->toRgb(*image._d_ptr);
	}

	///
	/// Get size of buffer
	///
	ssize_t size() const
	{
		return _d_ptr->size();
	}

	///
	/// Clear the image
	///
	void clear()
	{
		_d_ptr->clear();
	}

private:
	template<class T>
	friend class Image;

	///
	/// Translate x and y coordinate to index of the underlying vector
	///
	/// @param x The x index
	/// @param y The y index
	///
	/// @return The index into the underlying data-vector
	///
	int toIndex(int x, int y) const
	{
		return _d_ptr->toIndex(x, y);
	}

	QSharedDataPointer<ImageData<Pixel_T>> _d_ptr;

	quint64 _instanceId; // Unique ID for this C++ object handle
};

