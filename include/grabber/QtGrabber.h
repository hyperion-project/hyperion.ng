#pragma once

#include <QObject>

// Hyperion-utils includes
#include <utils/ColorRgb.h>
#include <hyperion/Grabber.h>

class QScreen;

///
/// @brief The platform capture implementation based on QT API
///
class QtGrabber : public Grabber
{
public:

	QtGrabber(int cropLeft, int cropRight, int cropTop, int cropBottom, int pixelDecimation, int display);

	virtual ~QtGrabber();

	///
	/// Captures a single snapshot of the display and writes the data to the given image. The
	/// provided image should have the same dimensions as the configured values (_width and
	/// _height)
	///
	/// @param[out] image  The snapped screenshot (should be initialized with correct width and
	/// height)
	///
	virtual int grabFrame(Image<ColorRgb> & image);

	///
	/// @brief Set a new video mode
	///
	virtual void setVideoMode(VideoMode mode);

	///
	/// @brief Apply new width/height values, overwrite Grabber.h implementation as qt doesn't use width/height, just pixelDecimation to calc dimensions
	///
	virtual bool setWidthHeight(int width, int height) { return true; };

	///
	/// @brief Apply new pixelDecimation
	///
	virtual void setPixelDecimation(int pixelDecimation);

	///
	/// Set the crop values
	/// @param  cropLeft    Left pixel crop
	/// @param  cropRight   Right pixel crop
	/// @param  cropTop     Top pixel crop
	/// @param  cropBottom  Bottom pixel crop
	///
	virtual void setCropping(unsigned cropLeft, unsigned cropRight, unsigned cropTop, unsigned cropBottom);

	///
	/// @brief Apply display index
	///
	virtual void setDisplayIndex(int index);

private slots:
	///
	/// @brief is called whenever the current _screen changes it's geometry
	/// @param geo   The new geometry
	///
	void geometryChanged(const QRect &geo);

private:
	///
	/// @brief Setup a new capture display, will free the previous one
	/// @return True on success, false if no display is found
	///
	const bool setupDisplay();

	///
	/// @brief Is called whenever we need new screen dimension calculations based on window geometry
	///
	int updateScreenDimensions(const bool& force);

	///
	/// @brief free the _screen pointer
	///
	void freeResources();

private:

	unsigned _display;
	int _pixelDecimation;
	unsigned _screenWidth;
	unsigned _screenHeight;
	unsigned _src_x;
	unsigned _src_y;
	unsigned _src_x_max;
	unsigned _src_y_max;
	QScreen* _screen;
};
