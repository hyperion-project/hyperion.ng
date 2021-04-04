#pragma once

#include <QObject>
#include <cstdint>

#include <utils/ColorRgb.h>
#include <utils/Image.h>
#include <utils/VideoMode.h>
#include <utils/VideoStandard.h>
#include <utils/ImageResampler.h>
#include <utils/Logger.h>
#include <utils/Components.h>

#include <QMultiMap>

///
/// @brief The Grabber class is responsible to apply image resizes (with or without ImageResampler)

class Grabber : public QObject
{
	Q_OBJECT

public:
	Grabber(const QString& grabberName = "", int width=0, int height=0, int cropLeft=0, int cropRight=0, int cropTop=0, int cropBottom=0);

	///
	/// Set the video mode (2D/3D)
	/// @param[in] mode The new video mode
	///
	virtual void setVideoMode(VideoMode mode);

	///
	/// Apply new flip mode (vertical/horizontal/both)
	/// @param[in] mode The new flip mode
	///
	virtual void setFlipMode(FlipMode mode);

	///
	/// @brief Apply new crop values, on errors reject the values
	///
	virtual void setCropping(unsigned cropLeft, unsigned cropRight, unsigned cropTop, unsigned cropBottom);

	///
	/// @brief Apply new video input (used from v4l2/MediaFoundation)
	/// @param input device input
	///
	virtual bool setInput(int input);

	///
	/// @brief Apply new width/height values, on errors (collide with cropping) reject the values
	/// @return True on success else false
	///
	virtual bool setWidthHeight(int width, int height);

	///
	/// @brief Apply new framerate (used from v4l2/MediaFoundation)
	/// @param fps framesPerSecond
	///
	virtual bool setFramerate(int fps);

	///
	/// @brief Apply new framerate software decimation (used from v4l2/MediaFoundation)
	/// @param decimation how many frames per second to omit
	///
	virtual void setFpsSoftwareDecimation(int decimation);

	///
	/// @brief Apply videoStandard (used from v4l2)
	///
	virtual void setVideoStandard(VideoStandard videoStandard);

	///
	/// @brief  Apply new pixelDecimation (used from v4l2, MediaFoundation, x11, xcb and qt)
	///
	virtual bool setPixelDecimation(int pixelDecimation);

	///
	/// @brief Apply display index (used from qt)
	///
	virtual void setDisplayIndex(int index) {}

	///
	/// @brief Apply path for device (used from framebuffer)
	///
	virtual void setDevicePath(const QString& path) {}

	///
	/// @brief get current resulting height of image (after crop)
	///
	virtual int getImageWidth() { return _width; }

	///
	/// @brief get current resulting width of image (after crop)
	///
	virtual int getImageHeight() { return _height; }

	///
	/// @brief Prevent the real capture implementation from capturing if disabled
	///
	void setEnabled(bool enable);

	QString getGrabberName() const { return _grabberName; }

protected:

	QString _grabberName;

	ImageResampler _imageResampler;

	bool _useImageResampler;

	/// the selected VideoMode
	VideoMode _videoMode;

	/// the used video standard
	VideoStandard _videoStandard;

	/// Image size decimation
	int _pixelDecimation;

	/// the used Flip Mode
	FlipMode _flipMode;

	/// With of the captured snapshot [pixels]
	int _width;

	/// Height of the captured snapshot [pixels]
	int _height;

	/// frame per second
	int _fps;

	/// fps software decimation
	int _fpsSoftwareDecimation;

	/// device input
	int _input;

	/// number of pixels to crop after capturing
	int _cropLeft, _cropRight, _cropTop, _cropBottom;

	bool _enabled;

	/// logger instance
	Logger * _log;

};
