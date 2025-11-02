#ifndef IMAGEDATA_H
#define IMAGEDATA_H

// STL includes
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <vector>

#include <QLoggingCategory>

#include "HyperionConfig.h"
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

Q_DECLARE_LOGGING_CATEGORY(memory_objects_image_create)
Q_DECLARE_LOGGING_CATEGORY(memory_objects_image_copy)
Q_DECLARE_LOGGING_CATEGORY(memory_objects_image_move)
Q_DECLARE_LOGGING_CATEGORY(memory_objects_image_assign)
Q_DECLARE_LOGGING_CATEGORY(memory_objects_image_destroy)

// Define to enable memory tracing for Image objects.
// Define as TraceEvent::All; for all events, or a combination of TraceEvent flags.
// e.g. constexpr TraceEvent TRACE_IMAGE_DATA_MEMORY_EVENTS = TraceEvent::Alloc | TraceEvent::Release

template <typename Pixel_T>
class ImageData;

// Base class to hold the static instance counter
class ImageDataCounter {
protected:
	// The static counter is defined in a .cpp file to ensure a single instance.
	static QAtomicInteger<quint64> _imageData_instance_counter;
};

template <typename>
class Image;

template <typename Pixel_T>
class ImageData : public QSharedData, private ImageDataCounter
{
	friend class Image<Pixel_T>;
public:
	using pixel_type = Pixel_T;

	ImageData(int width, int height, const pixel_type background);
	// Copy constructor for deep copies (for detach)
	ImageData(const ImageData& other);

	~ImageData();

	ImageData& operator=(ImageData rhs);

	void swap(ImageData& src) noexcept;

	// Move constructor
	ImageData(ImageData&& src) noexcept;

	// Check reference count
	int refCount() const;

	int width() const;

	int height() const;

	uint8_t red(int pixel) const;

	uint8_t green(int pixel) const;

	uint8_t blue(int pixel) const;

	const Pixel_T& operator()(int x, int y) const;

	Pixel_T& operator()(int x, int y);

	void resize(int width, int height);

	Pixel_T* memptr();

	const Pixel_T* memptr() const;

	void toRgb(ImageData<ColorRgb>& image) const;

	ssize_t size() const;

	void clear();

	void reset();

private:
	int toIndex(int x, int y) const;

	/// The width of the image
	int _width;
	/// The height of the image
	int _height;
	/// The pixels of the image
	std::vector<pixel_type> _pixels;

	quint64 _instanceId; // Unique ID for this data block
};

#endif // IMAGEDATA_H
