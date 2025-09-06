#include <utils/ImageData.h>
#include <utils/ColorRgb.h>
#include <utils/ColorRgba.h>
#include <utils/Logger.h>

// The static instance counter needs to be defined in a .cpp file.
QAtomicInteger<quint64> ImageDataCounter::_imageData_instance_counter(0);

template <typename Pixel_T>
ImageData<Pixel_T>::ImageData(int width, int height, const pixel_type background) :
	_width(width),
	_height(height),
	_pixels(new pixel_type[static_cast<size_t>(width) * static_cast<size_t>(height)]),
	_instanceId(++_imageData_instance_counter)
{
	std::fill(_pixels, _pixels + static_cast<size_t>(width) * height, background);

	DebugIf(is_tracing<ImageData<pixel_type>>(TraceEvent::Alloc), Logger::getInstance("MEMORY-ImageData"), "ALLOC (DATA): New ImageData [%d] created (%dx%d).", _instanceId, width, height);
}

template <typename Pixel_T>
ImageData<Pixel_T>::ImageData(const ImageData& other) :
	_width(other._width),
	_height(other._height),
	_pixels(new pixel_type[static_cast<size_t>(other._width) * static_cast<size_t>(other._height)]),
	_instanceId(++_imageData_instance_counter)
{
	memcpy(_pixels, other._pixels, static_cast<size_t>(other._width) * static_cast<size_t>(other._height) * sizeof(Pixel_T));

	DebugIf(is_tracing<ImageData<pixel_type>>(TraceEvent::Deep), Logger::getInstance("MEMORY-ImageData"), "COPY (DEEP DATA): New ImageData [%d] created as a deep copy of [%d].", _instanceId, other._instanceId);
}

template <typename Pixel_T>
ImageData<Pixel_T>::~ImageData()
{
	delete[] _pixels;
	DebugIf(is_tracing<ImageData<pixel_type>>(TraceEvent::Release), Logger::getInstance("MEMORY-ImageData"), "RELEASE (DATA): ImageData [%d] destroyed and memory freed.", _instanceId);
}

template <typename Pixel_T>
ImageData<Pixel_T>& ImageData<Pixel_T>::operator=(ImageData rhs)
{
	rhs.swap(*this);
	return *this;
}

template <typename Pixel_T>
void ImageData<Pixel_T>::swap(ImageData& src) noexcept
{
	using std::swap;
	swap(this->_width, src._width);
	swap(this->_height, src._height);
	swap(this->_pixels, src._pixels);
	swap(this->_instanceId, src._instanceId);
}

template <typename Pixel_T>
ImageData<Pixel_T>::ImageData(ImageData&& src) noexcept
: _width(src._width)
, _height(src._height)
, _pixels(src._pixels)
, _instanceId(src._instanceId)
{
	src._width = 0;
	src._height = 0;
	src._pixels = nullptr;
	src._instanceId = 0;
}

template <typename Pixel_T>
int ImageData<Pixel_T>::refCount() const { return this->ref.loadRelaxed(); }

template <typename Pixel_T>
int ImageData<Pixel_T>::width() const
{
	return _width;
}

template <typename Pixel_T>
int ImageData<Pixel_T>::height() const
{
	return _height;
}

template <typename Pixel_T>
uint8_t ImageData<Pixel_T>::red(int pixel) const
{
	return (_pixels + pixel)->red;
}

template <typename Pixel_T>
uint8_t ImageData<Pixel_T>::green(int pixel) const
{
	return (_pixels + pixel)->green;
}

template <typename Pixel_T>
uint8_t ImageData<Pixel_T>::blue(int pixel) const
{
	return (_pixels + pixel)->blue;
}

template <typename Pixel_T>
const typename ImageData<Pixel_T>::pixel_type& ImageData<Pixel_T>::operator()(int x, int y) const
{
	return _pixels[y * _width + x];
}

template <typename Pixel_T>
typename ImageData<Pixel_T>::pixel_type& ImageData<Pixel_T>::operator()(int x, int y)
{
	return _pixels[y * _width + x];
}

template <typename Pixel_T>
void ImageData<Pixel_T>::resize(int width, int height)
{
	if (width == _width && height == _height)
	{
		return;
	}

	// Allocate a new buffer without initializing the content
	auto newPixels = new pixel_type[static_cast<size_t>(width) * static_cast<size_t>(height)];

	// Release the old buffer without copying data
	delete[] _pixels;

	// Update the pointer to the new buffer
	_pixels = newPixels;

	_width = width;
	_height = height;
}

template <typename Pixel_T>
typename ImageData<Pixel_T>::pixel_type* ImageData<Pixel_T>::memptr()
{
	return _pixels;
}

template <typename Pixel_T>
const typename ImageData<Pixel_T>::pixel_type* ImageData<Pixel_T>::memptr() const
{
	return _pixels;
}

template <typename Pixel_T>
void ImageData<Pixel_T>::toRgb(ImageData<ColorRgb>& image) const
{
	if (image.width() != _width || image.height() != _height)
	{
		image.resize(_width, _height);
	}

	const int imageSize = _width * _height;
	for (int idx = 0; idx < imageSize; idx++)
	{
		const pixel_type& color = _pixels[idx];
		image.memptr()[idx] = ColorRgb{ color.red, color.green, color.blue };
	}
}

template <typename Pixel_T>
ssize_t ImageData<Pixel_T>::size() const
{
	return  static_cast<ssize_t>(_width) * static_cast<ssize_t>(_height) * sizeof(pixel_type);
}

template <typename Pixel_T>
void ImageData<Pixel_T>::clear()
{
	// Fill the entire existing pixel buffer with the default-constructed pixel value
	std::fill(_pixels, _pixels + (static_cast<size_t>(_width) * _height), pixel_type());
}

template <typename Pixel_T>
void ImageData<Pixel_T>::reset()
{
	if (_width != 1 || _height != 1)
	{
		resize(1, 1);
	}
	// Set the single pixel to the default background
	_pixels[0] = pixel_type();
}

template <typename Pixel_T>
int ImageData<Pixel_T>::toIndex(int x, int y) const
{
	return y * _width + x;
}

// Explicit template instantiations
template class ImageData<ColorRgb>;
template class ImageData<ColorRgba>;