#pragma once

#include <QObject>
#include <QJsonObject>

#include <hyperion/Grabber.h>
#include <utils/ColorRgb.h>

#include <d3d11.h>

class DDAGrabberImpl;

class DDAGrabber : public Grabber
{
public:
	DDAGrabber(int display = 0, int cropLeft = 0, int cropRight = 0, int cropTop = 0, int cropBottom = 0);

	virtual ~DDAGrabber();

	///
	/// Captures a single snapshot of the display and writes the data to the given image. The
	/// provided image should have the same dimensions as the configured values (_width and _height)
	///
	/// @param[out] image  The snapped screenshot
	///
	int grabFrame(Image<ColorRgb> &image);

	///
	/// @brief Set a new video mode
	///
	void setVideoMode(VideoMode mode) override;

	///
	/// @brief Apply new width/height values, overwrite Grabber.h implementation
	///
	bool setWidthHeight(int /* width */, int /*height*/) override
	{
		return true;
	}

	///
	/// @brief Apply new pixelDecimation
	///
	bool setPixelDecimation(int pixelDecimation) override;

	///
	/// Set the crop values
	/// @param  cropLeft    Left pixel crop
	/// @param  cropRight   Right pixel crop
	/// @param  cropTop     Top pixel crop
	/// @param  cropBottom  Bottom pixel crop
	///
	void setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom);

	///
	/// @brief Apply display index
	///
	bool setDisplayIndex(int index) override;

	/// @brief Discover QT screens available (for configuration).
	///
	/// @param[in] params Parameters used to overwrite discovery default behaviour
	///
	/// @return A JSON structure holding a list of devices found
	///
	QJsonObject discover(const QJsonObject &params);

	///
	/// @brief Opens the input device.
	///
	/// @return Zero, on success (i.e. device is ready), else negative
	///
	bool open();

private:

	///
	/// @brief Setup a new capture display, will free the previous one
	/// @return True on success, false if no display is found
	///
	bool restartCapture();

	bool resetDeviceAndCapture();

	void computeCropBox(const D3D11_TEXTURE2D_DESC& desc, D3D11_BOX& box) const;
	void copyMappedToImage(const D3D11_MAPPED_SUBRESOURCE& mapped, Image<ColorRgb>& image) const;

private:
	std::unique_ptr<DDAGrabberImpl> d;
};
