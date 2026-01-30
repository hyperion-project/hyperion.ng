#include <utils/Image.h>

#include <utils/ImageData.h>
#include <utils/ColorRgb.h>
#include <utils/ColorRgba.h>
#include <utils/Logger.h>

// The static instance counter needs to be defined in a .cpp file.
QAtomicInteger<quint64> ImageCounter::_image_instance_counter(0);

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
Image<Pixel_T>::Image() :
	Image(0, 0, pixel_type())
{
}

template <typename Pixel_T>
Image<Pixel_T>::Image(int width, int height) :
	Image(width, height, pixel_type())
{
}

template <typename Pixel_T>
Image<Pixel_T>::Image(int width, int height, const Pixel_T background) :
	_d_ptr(new ImageData<Pixel_T>(width, height, background)),
	_instanceId(++_image_instance_counter)
{
	qCDebug(image_create).noquote() << QString("|Image| CREATE: Creating Image [%1] of size %2x%3").arg(_instanceId).arg(width).arg(height);
}

template <typename Pixel_T>
Image<Pixel_T>::Image(const Image& other) :
	_d_ptr(other._d_ptr), // This just increments the ref-counter
	_instanceId(++_image_instance_counter)
{
	qCDebug(image_copy).noquote() << QString("|Image| COPY (SHALLOW): Image handle [%1] created, sharing data with handle [%2].").arg(_instanceId).arg(other._instanceId);
}

template <typename Pixel_T>
Image<Pixel_T>::Image(Image&& src) noexcept :
	_d_ptr(std::move(src._d_ptr)),
	_instanceId(src._instanceId)
{
	src._instanceId = 0; // Invalidate moved-from handle
	qCDebug(image_move).noquote() << QString("|Image| MOVE: Image handle [%1] has been moved into a new instance.").arg(_instanceId);
}

template <typename Pixel_T>
Image<Pixel_T>::~Image()
{
	if (_instanceId == 0)
	{
		return;
	}

	if (isDetached())
	{
		qCDebug(image_destroy).noquote() << QString("|Image| DESTROY (HANDLE): Image handle [%1] destroyed. This was the last handle.").arg(_instanceId);
	}
	else
	{
		qCDebug(image_destroy).noquote() << QString("|Image| DESTROY (HANDLE): Image handle [%1] destroyed. Other handles still exist.").arg(_instanceId);
	}
}

template <typename Pixel_T>
Image<Pixel_T>& Image<Pixel_T>::operator=(const Image& other)
{
	if (this != &other)
	{
		_d_ptr = other._d_ptr;
		qCDebug(image_assign).noquote() << QString("|Image| ASSIGN (SHALLOW HANDLE): Image handle [%1] now shares data with handle [%2].").arg(_instanceId).arg(other._instanceId);
	}
	return *this;
}

template <typename Pixel_T>
Image<Pixel_T>& Image<Pixel_T>::operator=(Image&& other) noexcept
{
	if (this != &other)
	{
		_d_ptr = std::move(other._d_ptr);
		_instanceId = other._instanceId;
		other._instanceId = 0;
		qCDebug(image_assign).noquote() << QString("|Image| ASSIGN (MOVE): Image handle [%1] has taken ownership from another handle.").arg(_instanceId);
	}
	return *this;
}

template <typename Pixel_T>
void Image<Pixel_T>::swap(Image& other) noexcept
{
	std::swap(this->_d_ptr, other._d_ptr);
	std::swap(this->_instanceId, other._instanceId);
}

template <typename Pixel_T>
bool Image<Pixel_T>::isDetached() const
{
	return _d_ptr != nullptr && _d_ptr->refCount() == 1;
}

template <typename Pixel_T>
int Image<Pixel_T>::width() const
{
	return _d_ptr->width();
}

template <typename Pixel_T>
int Image<Pixel_T>::height() const
{
	return _d_ptr->height();
}

template <typename Pixel_T>
uint8_t Image<Pixel_T>::red(unsigned pixel) const
{
	return _d_ptr->red(pixel);
}

template <typename Pixel_T>
uint8_t Image<Pixel_T>::green(unsigned pixel) const
{
	return _d_ptr->green(pixel);
}

template <typename Pixel_T>
uint8_t Image<Pixel_T>::blue(int pixel) const
{
	return _d_ptr->blue(pixel);
}

template <typename Pixel_T>
const typename Image<Pixel_T>::pixel_type& Image<Pixel_T>::operator()(int x, int y) const
{
	return _d_ptr->operator()(x, y);
}

template <typename Pixel_T>
typename Image<Pixel_T>::pixel_type& Image<Pixel_T>::operator()(int x, int y)
{
	return _d_ptr->operator()(x, y);
}

template <typename Pixel_T>
void Image<Pixel_T>::resize(int width, int height)
{
	_d_ptr->resize(width, height);
}

template <typename Pixel_T>
typename Image<Pixel_T>::pixel_type* Image<Pixel_T>::memptr()
{
	if (!isDetached())
	{
		qCDebug(image_copy).noquote() << QString("|Image| COPY (DEEP): memptr() on shared Image handle [%1] is causing a detach.").arg(_instanceId);
	}
	_d_ptr.detach();
	return _d_ptr->memptr();
}

template <typename Pixel_T>
const typename Image<Pixel_T>::pixel_type* Image<Pixel_T>::memptr() const
{
	return _d_ptr->memptr();
}

template <typename Pixel_T>
void Image<Pixel_T>::toRgb(Image<ColorRgb>& image) const
{
	_d_ptr->toRgb(*image._d_ptr);
}

template <typename Pixel_T>
ssize_t Image<Pixel_T>::size() const
{
	return _d_ptr->size();
}

template <typename Pixel_T>
bool Image<Pixel_T>::isNull() const
{
	return _d_ptr->size() == 0;
}

template <typename Pixel_T>
void Image<Pixel_T>::clear()
{
	_d_ptr->clear();
}

template <typename Pixel_T>
void Image<Pixel_T>::clear(const pixel_type background)
{
	_d_ptr->clear(background);
}

template <typename Pixel_T>
void Image<Pixel_T>::reset()
{
	_d_ptr->reset();
}

template <typename Pixel_T>
quint64 Image<Pixel_T>::id() const
{
	return _instanceId;
}

template <typename Pixel_T>
QImage Image<Pixel_T>::toQImage() const
{
	const ImageData<pixel_type>* d_ptr = _d_ptr.constData();
	if (d_ptr == nullptr)
	{
		return QImage();
	}

	// Manually increment the reference count for the shared data block.
	// This is necessary because we are creating a new QImage that will share the data.
	// QImage's cleanup function will later decrement it.
	const_cast<ImageData<pixel_type>*>(d_ptr)->ref.ref();

	return QImage(
		reinterpret_cast<const uchar*>(d_ptr->memptr()),
		d_ptr->width(),
		d_ptr->height(),
		d_ptr->width() * sizeof(pixel_type),
		PixelFormatTraits<pixel_type>::format,
		// Provide a custom cleanup function. QImage will call this when it's destroyed.
		imageDataCleanupHandler<pixel_type>,
		// Pass the ImageData pointer as the context for the cleanup handler.
		const_cast<ImageData<pixel_type>*>(d_ptr)
	);
}

template <typename Pixel_T>
QImage Image<Pixel_T>::toQImage()
{
	// First, ensure we have a unique copy of the data before allowing modification.
	// This is the core of copy-on-write. memptr() calls detach() internally.
	memptr();

	ImageData<Pixel_T>* d_ptr = _d_ptr.data();
	if (d_ptr == nullptr)
	{
		return QImage();
	}

	// Manually increment the reference count for the shared data block.
	// This is necessary because we are creating a new QImage that will share the data.
	// QImage's cleanup function will later decrement it.
	d_ptr->ref.ref();

	// Create a modifiable QImage wrapper
	return QImage(
		reinterpret_cast<uchar*>(d_ptr->memptr()),
		d_ptr->width(),
		d_ptr->height(),
		d_ptr->width() * sizeof(pixel_type),
		PixelFormatTraits<pixel_type>::format,
		// Provide a custom cleanup function. QImage will call this when it's destroyed.
		imageDataCleanupHandler<pixel_type>,
		// Pass the ImageData pointer as the context for the cleanup handler.
		d_ptr
	);
}

template <typename Pixel_T>
int Image<Pixel_T>::toIndex(int x, int y) const
{
	return _d_ptr->toIndex(x, y);
}

// Explicit template instantiations
template class Image<ColorRgb>;
template class Image<ColorRgba>;
template class Image<ColorBgr>;

template void imageDataCleanupHandler<ColorRgb>(void*);
template void imageDataCleanupHandler<ColorRgba>(void*);
template void imageDataCleanupHandler<ColorBgr>(void*);
