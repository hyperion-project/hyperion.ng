#ifndef IMAGE_H
#define IMAGE_H

#include <QSharedDataPointer>
#include <QAtomicInt>
#include <QImage>
#include <QDebug>

#include "HyperionConfig.h"
#include <utils/ImageData.h>
#include <utils/ColorRgb.h>
#include <utils/ColorRgba.h>
#include <utils/Logger.h>
#include <utils/TrackedMemory.h>

#include <cstdint>
#include <utils/Logger.h>

// Define to enable memory tracing for Image objects.
// Define as TraceEvent::All; for all events, or a combination of TraceEvent flags.
// e.g. constexpr TraceEvent TRACE_IMAGE_MEMORY_EVENTS = TraceEvent::Alloc | TraceEvent::Release

template <typename Pixel_T>
class Image;

#if defined(ENABLE_TRACE_IMAGE_MEMORY)
constexpr TraceEvent TRACE_IMAGE_MEMORY_EVENTS = TraceEvent::All;
template<typename Pixel_T>
struct ComponentTracer<Image<Pixel_T>> {
	static constexpr bool enabled = true;
	static inline TraceEvent active_events = TRACE_IMAGE_MEMORY_EVENTS;
};
#else
template<typename Pixel_T>
struct ComponentTracer<Image<Pixel_T>> {
	static constexpr bool enabled = false;
	static inline TraceEvent active_events = TraceEvent::NoTrace;
};
#endif

// Base class to hold the static instance counter
class ImageCounter {
protected:
	// The static counter is defined in a .cpp file to ensure a single instance.
	static QAtomicInteger<quint64> _image_instance_counter;
};

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
void imageDataCleanupHandler(void* info);

template <typename Pixel_T>
class Image : private ImageCounter
{
public:
	using pixel_type = Pixel_T;

	Image();
	Image(int width, int height);
	///
	/// Constructor for an image with specified width and height
	///
	/// @param width The width of the image
	/// @param height The height of the image
	/// @param background The color of the image
	///
	Image(int width, int height, const Pixel_T background);

	// Copy constructor (Shallow Copy)
	Image(const Image& other);

	// Move constructor
	Image(Image&& src) noexcept;

	// Destructor
	~Image();

	// Copy assignment operator (Shallow Copy)
	Image& operator=(const Image& other);

	// Move assignment operator
	Image& operator=(Image&& other) noexcept;

	void swap(Image& other) noexcept;

	// Check if the data is unique
	bool isDetached() const;

	///
	/// Returns the width of the image
	///
	/// @return The width of the image
	///
	int width() const;

	///
	/// Returns the height of the image
	///
	/// @return The height of the image
	///
	int height() const;

	uint8_t red(unsigned pixel) const;

	uint8_t green(unsigned pixel) const;

	///
	/// Returns a const reference to a specified pixel in the image
	///
	/// @param x The x index
	/// @param y The y index
	///
	/// @return const reference to specified pixel
	///
	uint8_t blue(int pixel) const;

	///
	/// Returns a reference to a specified pixel in the image
	///
	/// @param x The x index
	/// @param y The y index
	const Pixel_T& operator()(int x, int y) const;

	///
	/// @return reference to specified pixel
	///
	Pixel_T& operator()(int x, int y);

	/// Resize the image
	/// @param width The width of the image
	/// @param height The height of the image
	void resize(int width, int height);

	///
	/// Returns a memory pointer to the first pixel in the image
	/// @return The memory pointer to the first pixel
	///
	Pixel_T* memptr();

	///
	/// Returns a const memory pointer to the first pixel in the image
	/// @return The const memory pointer to the first pixel
	///
	const Pixel_T* memptr() const;

	///
	/// Convert image of any color order to a RGB image.
	///
	/// @param[out] image  The image that buffers the output
	///
	void toRgb(Image<ColorRgb>& image) const;

	///
	/// Get size of buffer
	///
	ssize_t size() const;

	///
	/// Clear the image, i.e. fill the image with default background color
	///
	void clear();

	///
	/// Reset the image to 1x1 with default background color
	///
	void reset();

	quint64 id() const;

	///
	/// Returns a const QImage that shares data with this Image object.
	/// No data is copied. The returned QImage is read-only.
	///
	QImage toQImage() const;

	///
	/// Returns a modifiable QImage that shares data with this Image object.
	/// This may trigger a deep copy (detach) if the data is currently shared,
	/// preserving copy-on-write semantics.
	///
	QImage toQImage();

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
	int toIndex(int x, int y) const;

	QSharedDataPointer<ImageData<pixel_type>> _d_ptr;
	quint64 _instanceId; // Unique ID for this C++ object handle
};

#endif // IMAGE_H
