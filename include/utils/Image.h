#pragma once

#include <QDebug>
#include <QSharedDataPointer>
#include <QAtomicInt>
#include <QImage>



#include <utils/ImageData.h>
#include <utils/ColorRgb.h>
#include <utils/ColorRgba.h>
#include <utils/Logger.h>

#define IMAGE_ENABLE_MEMORY_TRACKING_ALLOC 0
#define IMAGE_ENABLE_MEMORY_TRACKING_DEEP 0
#define IMAGE_ENABLE_MEMORY_TRACKING_SHALLOW 0
#define IMAGE_ENABLE_MEMORY_TRACKING_MOVE 0
#define IMAGE_ENABLE_MEMORY_TRACKING_RELEASE 0

namespace {
// Counter for unique HANDLE instance IDs
QAtomicInteger<quint64> image_instance_counter(0);
}


// Trait to map a Pixel_T to a QImage::Format
template<typename Pixel_T>
struct PixelFormatTraits;

// Specialization for ColorRgb
template<>
struct PixelFormatTraits<ColorRgb> {
	static constexpr QImage::Format format = QImage::Format_RGB888;
	static_assert(sizeof(ColorRgb) == 3,
		"ColorRgb must be exactly 3 bytes to match QImage::Format_RGB888");
};

// Specialization for ColorRgba
template<>
struct PixelFormatTraits<ColorRgba> {
	// Note: QImage byte order is ARGB for 32-bit.
	// If your ColorRgba is {R,G,B,A}, you may need to use Format_RGBA8888 and potentially swizzle bytes.
	// Format_RGB32 is a common alternative, mapping to {0,R,G,B}.
	// We'll assume Format_RGB8888 aligns with {R,G,B,A} layout.
	static constexpr QImage::Format format = QImage::Format_RGBA8888;
	static_assert(sizeof(ColorRgba) == 4,
		"ColorRgba must be exactly 4 bytes to match QImage::Format_RGBA8888");
};

template <typename Pixel_T>
void imageDataCleanupHandler(void* info)
{
	// This function is called by QImage when it's destroyed.
	// It safely decrements the reference count of our shared data.
	auto* data = static_cast<ImageData<Pixel_T>*>(info);
	if (data)
	{
		data->ref.deref();
	}
}

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

	// Move constructor
	Image(Image&& src) noexcept :
		_d_ptr(src._d_ptr),
		_instanceId(src._instanceId)
	{
		src._instanceId = 0; // Invalidate moved-from handle
		DebugIf(IMAGE_ENABLE_MEMORY_TRACKING_MOVE, Logger::getInstance("MEMORY-Image"), "MOVE: Image handle [%d] has been moved into a new instance.", _instanceId);
	}

	// Destructor
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

	// Copy assignment operator (Shallow Copy)
	Image& operator=(const Image& other)
	{
		if (this != &other)
		{
			_d_ptr = other._d_ptr;
			DebugIf(IMAGE_ENABLE_MEMORY_TRACKING_SHALLOW, Logger::getInstance("MEMORY-Image"), "ASSIGN (SHALLOW HANDLE): Image handle [%d]  now shares data with handle [%d].", _instanceId, other._instanceId);
		}
		return *this;
	}

	// Move assignment operator
	Image& operator=(Image&& other) noexcept
	{
		if (this != &other)
		{
			_d_ptr = std::move(other._d_ptr);
			_instanceId = other._instanceId;
			other._instanceId = 0;
			DebugIf(IMAGE_ENABLE_MEMORY_TRACKING_MOVE, Logger::getInstance("MEMORY-Image"), "MOVE ASSIGN: Image handle [%d] has taken ownership from another handle.", _instanceId);
		}
		return *this;
	}

	void swap(Image& other) noexcept
	{
		std::swap(this->_d_ptr, other._d_ptr);
		std::swap(this->_instanceId, other._instanceId);
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
			DebugIf(IMAGE_ENABLE_MEMORY_TRACKING_DEEP, Logger::getInstance("MEMORY-Image"), "COPY (DEEP): memptr() on shared Image handle [%d] is causing a detach.", _instanceId);
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
	/// Clear the image, i.e. fill the image with default background color
	///
	void clear()
	{
		_d_ptr->clear();
	}

	///
	/// Reset the image to 1x1 with default background color
	///
	void reset()
	{
		_d_ptr->reset();
	}

	quint64 id() const
	{
		return _instanceId;
	}

	///
	/// Returns a const QImage that shares data with this Image object.
	/// No data is copied. The returned QImage is read-only.
	///
	QImage toQImage() const
	{
		const ImageData<Pixel_T>* d_ptr = _d_ptr.constData();
		if (d_ptr == nullptr)
		{
			return QImage();
		}

		// Manually increment the reference count. Use const_cast to bypass the const-check.
		const_cast<ImageData<Pixel_T>*>(d_ptr)->ref.ref();

		return QImage(
			reinterpret_cast<const uchar*>(d_ptr->memptr()),
			d_ptr->width(),
			d_ptr->height(),
			d_ptr->width() * sizeof(Pixel_T),
			PixelFormatTraits<Pixel_T>::format,
			imageDataCleanupHandler<Pixel_T>,
			const_cast<ImageData<Pixel_T>*>(d_ptr)
		);
	}

	///
	/// Returns a modifiable QImage that shares data with this Image object.
	/// This may trigger a deep copy (detach) if the data is currently shared,
	/// preserving copy-on-write semantics.
	///
	QImage toQImage()
	{
		// First, ensure we have a unique copy of the data before allowing modification.
		// This is the core of copy-on-write.
		memptr(); // This calls detach() internally

		ImageData<Pixel_T>* d_ptr = _d_ptr.data();
		if (d_ptr == nullptr)
		{
			return QImage();
		}

		// Manually increment the reference count
		d_ptr->ref.ref();

		// Create a modifiable QImage wrapper
		return QImage(
			reinterpret_cast<uchar*>(d_ptr->memptr()),
			d_ptr->width(),
			d_ptr->height(),
			d_ptr->width() * sizeof(Pixel_T),
			PixelFormatTraits<Pixel_T>::format,
			imageDataCleanupHandler<Pixel_T>,
			d_ptr
		);
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

