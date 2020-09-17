#pragma once

#include <QSharedDataPointer>

#include <utils/ImageData.h>

template <typename Pixel_T>
class Image
{
public:
	typedef Pixel_T pixel_type;

	Image() :
		Image(1, 1, Pixel_T())
	{
	}

	Image(unsigned width, unsigned height) :
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
	Image(unsigned width, unsigned height, const Pixel_T background) :
		_d_ptr(new ImageData<Pixel_T>(width, height, background))
	{
	}

	///
	/// Copy constructor for an image
	/// @param other The image which will be copied
	///
	Image(const Image & other)
	{
		_d_ptr = other._d_ptr;
	}

	Image& operator=(Image rhs)
	{
		// Define assignment operator in terms of the copy constructor
		// More to read: https://stackoverflow.com/questions/255612/dynamically-allocating-an-array-of-objects?answertab=active#tab-top
		_d_ptr = rhs._d_ptr;
		return *this;
	}

	void swap(Image& s)
	{
		std::swap(this->_d_ptr, s._d_ptr);
	}

	Image(Image&& src) noexcept
	{
		std::swap(this->_d_ptr, src._d_ptr);
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
	}

	///
	/// Returns the width of the image
	///
	/// @return The width of the image
	///
	inline unsigned width() const
	{
		return _d_ptr->width();
	}

	///
	/// Returns the height of the image
	///
	/// @return The height of the image
	///
	inline unsigned height() const
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
	uint8_t blue(unsigned pixel) const
	{
		return _d_ptr->blue(pixel);
	}

	///
	/// Returns a reference to a specified pixel in the image
	///
	/// @param x The x index
	/// @param y The y index
	const Pixel_T& operator()(unsigned x, unsigned y) const
	{
		return _d_ptr->operator()(x, y);
	}

	///
	/// @return reference to specified pixel
	///
	Pixel_T& operator()(unsigned x, unsigned y)
	{
		return _d_ptr->operator()(x, y);
	}

	/// Resize the image
	/// @param width The width of the image
	/// @param height The height of the image
	void resize(unsigned width, unsigned height)
	{
		_d_ptr->resize(width, height);
	}

	///
	/// Returns a memory pointer to the first pixel in the image
	/// @return The memory pointer to the first pixel
	///
	Pixel_T* memptr()
	{
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
	inline unsigned toIndex(unsigned x, unsigned y) const
	{
		return _d_ptr->toIndex(x, y);
	}

private:
	QSharedDataPointer<ImageData<Pixel_T>>  _d_ptr;
};

