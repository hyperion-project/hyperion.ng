#pragma once

#include <QObject>
#include <cstdint>

#include <utils/ColorRgb.h>
#include <utils/Image.h>
#include <utils/VideoMode.h>
#include <grabber/VideoStandard.h>
#include <utils/ImageResampler.h>
#include <utils/Logger.h>

///
/// @brief The Grabber class is responsible to apply image resizes (with or without ImageResampler)
/// Overwrite the videoMode with setVideoMode()
/// Overwrite setCropping()
class Grabber : public QObject
{
	Q_OBJECT

public:
	Grabber(QString grabberName, int width=0, int height=0, int cropLeft=0, int cropRight=0, int cropTop=0, int cropBottom=0);
	virtual ~Grabber();

	///
	/// Set the video mode (2D/3D)
	/// @param[in] mode The new video mode
	///
	virtual void setVideoMode(VideoMode mode);

	///
	/// @brief Apply new crop values, on errors reject the values
	///
	virtual void setCropping(unsigned cropLeft, unsigned cropRight, unsigned cropTop, unsigned cropBottom);

	///
	/// @brief Apply new width/height values, on errors (collide with cropping) reject the values
	///
	virtual void setWidthHeight(int width, int height);

	///
	/// @brief Apply new pixelDecimation (used from x11)
	///
	virtual void setPixelDecimation(int pixelDecimation) {};

	///
	/// @brief Apply new signalThreshold (used from v4l)
	///
	virtual void setSignalThreshold(
					double redSignalThreshold,
					double greenSignalThreshold,
					double blueSignalThreshold,
					int noSignalCounterThreshold = 50) {};
	///
	/// @brief Apply new SignalDetectionOffset  (used from v4l)
	///
	virtual void setSignalDetectionOffset(
					double verticalMin,
					double horizontalMin,
					double verticalMax,
					double horizontalMax) {};

	///
	/// @brief Apply SignalDetectionEnable (used from v4l)
	///
	virtual void setSignalDetectionEnable(bool enable) {};

	///
	/// @brief Apply input and videoStanded (used from v4l)
	///
	virtual void setInputVideoStandard(int input, VideoStandard videoStandard) {};

	///
	/// @brief Apply display index (used from x11)
	///
	virtual void setDisplayIndex(int index) {};

	///
	/// @brief Apply path for device (used from framebuffer)
	///
	virtual void setDevicePath(const QString& path) {};

	///
	/// @brief get current resulting height of image (after crop)
	///
	virtual const int getImageWidth() { return _width; };

	///
	/// @brief get current resulting width of image (after crop)
	///
	virtual const int getImageHeight() { return _height; };

	///
	/// @brief Prevent the real capture implementation from capturing if disabled
	///
	void setEnabled(bool enable);

protected:
	ImageResampler _imageResampler;

	bool _useImageResampler;

	/// the selected VideoMode
	VideoMode    _videoMode;

	/// With of the captured snapshot [pixels]
	int _width;

	/// Height of the captured snapshot [pixels]
	int _height;

	// number of pixels to crop after capturing
	int _cropLeft, _cropRight, _cropTop, _cropBottom;

	bool _enabled;

	/// logger instance
	Logger * _log;

};
