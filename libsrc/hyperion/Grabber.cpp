#include <hyperion/Grabber.h>

Grabber::Grabber(const QString& grabberName, int width, int height, int cropLeft, int cropRight, int cropTop, int cropBottom)
	: _imageResampler()
	, _useImageResampler(true)
	, _videoMode(VideoMode::VIDEO_2D)
	, _width(width)
	, _height(height)
	, _fps(15)
	, _input(-1)
	, _cropLeft(0)
	, _cropRight(0)
	, _cropTop(0)
	, _cropBottom(0)
	, _enabled(true)
	, _log(Logger::getInstance(grabberName.toUpper()))
{
	Grabber::setVideoMode(VideoMode::VIDEO_2D);
	Grabber::setCropping(cropLeft, cropRight, cropTop, cropBottom);
}

void Grabber::setEnabled(bool enable)
{
	Info(_log,"Capture interface is now %s", enable ? "enabled" : "disabled");
	_enabled = enable;
}

void Grabber::setVideoMode(VideoMode mode)
{
	Debug(_log,"Set videomode to %d", mode);
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
			Error(_log, "Rejecting invalid crop values: left: %d, right: %d, top: %d, bottom: %d, higher than height/width %d/%d", cropLeft, cropRight, cropTop, cropBottom, _height, _width);
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

bool Grabber::setInput(int input)
{
	if((input >= 0) && (_input != input))
	{
		_input = input;
		return true;
	}

	return false;
}

bool Grabber::setWidthHeight(int width, int height)
{
	// eval changes with crop
	if ( (width>0 && height>0) && (_width != width || _height != height) )
	{
		if (_cropLeft + _cropRight >= width || _cropTop + _cropBottom >= height)
		{
			Error(_log, "Rejecting invalid width/height values as it collides with image cropping: width: %d, height: %d", width, height);
			return false;
		}
		Debug(_log, "Set new width: %d, height: %d for capture", width, height);
		_width = width;
		_height = height;
		return true;
	}
	return false;
}

bool Grabber::setFramerate(int fps)
{
	if((fps > 0) && (_fps != fps))
	{
		_fps = fps;
		return true;
	}

	return false;
}
