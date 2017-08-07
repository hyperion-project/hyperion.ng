#include <hyperion/Grabber.h>


Grabber::Grabber(QString grabberName, int width, int height, int cropLeft, int cropRight, int cropTop, int cropBottom)
	: _imageResampler()
	, _videoMode(VIDEO_2D)
	, _width(width)
	, _height(height)
	, _cropLeft(cropLeft)
	, _cropRight(cropRight)
	, _cropTop(cropTop)
	, _cropBottom(cropBottom)
	, _log(Logger::getInstance(grabberName))

{
	setVideoMode(VIDEO_2D);
}

Grabber::~Grabber()
{
}


void Grabber::setVideoMode(VideoMode mode)
{
	Debug(_log,"setvideomode");
	_videoMode = mode;
	_imageResampler.set3D(_videoMode);
}
