#pragma once

#include <QObject>
#include <cstdint>

#include <utils/ColorRgb.h>
#include <utils/Image.h>
#include <utils/VideoMode.h>
#include <utils/ImageResampler.h>
#include <utils/Logger.h>


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

	virtual void setCropping(unsigned cropLeft, unsigned cropRight, unsigned cropTop, unsigned cropBottom);

	/// gets resulting height of image
	virtual const int getImageWidth() { return _width; };

	/// gets resulting width of image
	virtual const int getImageHeight() { return _height; };

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
