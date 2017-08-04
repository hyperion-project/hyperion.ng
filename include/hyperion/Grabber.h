#pragma once

#include <QObject>
#include <cstdint>

#include <utils/Image.h>
#include <utils/VideoMode.h>
#include <utils/GrabbingMode.h>
#include <utils/ImageResampler.h>
#include <utils/Logger.h>


class Grabber : public QObject
{
	Q_OBJECT

public:
	Grabber(QString grabberName, int width=0, int height=0, int cropLeft=0, int cropRight=0, int cropTop=0, int cropBottom=0);
	~Grabber();
	
	///
	/// Set the video mode (2D/3D)
	/// @param[in] mode The new video mode
	///
	void setVideoMode(VideoMode mode);

	/// gets resulting height of image
	const int getImageWidth() { return _width; };

	/// gets resulting width of image
	const int getImageHeight() { return _height; };


protected:
	ImageResampler _imageResampler;

	/// the selected VideoMode
	VideoMode    _videoMode;

	/// With of the captured snapshot [pixels]
	int _width;
	
	/// Height of the captured snapshot [pixels]
	int _height;

	// number of pixels to crop after capturing
	int _cropLeft, _cropRight, _cropTop, _cropBottom;


	/// logger instance
	Logger * _log;

};
