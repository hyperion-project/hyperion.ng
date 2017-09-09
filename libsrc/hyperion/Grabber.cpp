#include <hyperion/Grabber.h>


Grabber::Grabber(QString grabberName, int width, int height, int cropLeft, int cropRight, int cropTop, int cropBottom)
	: _imageResampler()
	, _useImageResampler(true)
	, _videoMode(VIDEO_2D)
	, _width(width)
	, _height(height)
	, _cropLeft(0)
	, _cropRight(0)
	, _cropTop(0)
	, _cropBottom(0)
	, _enabled(true)
	, _log(Logger::getInstance(grabberName))

{
	setVideoMode(VIDEO_2D);
	setCropping(cropLeft, cropRight, cropTop, cropBottom);
}

Grabber::~Grabber()
{
}

void Grabber::setEnabled(bool enable)
{
	_enabled = enable;
}

void Grabber::setVideoMode(VideoMode mode)
{
	Debug(_log,"setvideomode %d", mode);
	_videoMode = mode;
	if ( _useImageResampler )
	{
		_imageResampler.setVideoMode(_videoMode);
	}
}

void Grabber::setCropping(unsigned cropLeft, unsigned cropRight, unsigned cropTop, unsigned cropBottom)
{
	if (_width>0 && _height>0)
	{
		if (cropLeft + cropRight >= (unsigned)_width || cropTop + cropBottom >= (unsigned)_height)
		{
			Error(_log, "Rejecting invalid crop values: left: %d, right: %d, top: %d, bottom: %d", cropLeft, cropRight, cropTop, cropBottom);
			return;
		}
	}

	_cropLeft   = cropLeft;
	_cropRight  = cropRight;
	_cropTop    = cropTop;
	_cropBottom = cropBottom;

	if ( _useImageResampler )
	{
		_imageResampler.setCropping(cropLeft, cropRight, cropTop, cropBottom);
	}
	else
	{
		_imageResampler.setCropping(0, 0, 0, 0);
	}

	if (cropLeft > 0 || cropRight > 0 || cropTop > 0 || cropBottom > 0)
	{
		Info(_log, "Cropping image: width=%d height=%d; crop: left=%d right=%d top=%d bottom=%d ", _width, _height, cropLeft, cropRight, cropTop, cropBottom);
	}
}
