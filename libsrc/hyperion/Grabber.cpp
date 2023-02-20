#include <hyperion/Grabber.h>
#include <hyperion/GrabberWrapper.h>

Grabber::Grabber(const QString& grabberName, int cropLeft, int cropRight, int cropTop, int cropBottom)
	: _grabberName(grabberName)
	, _log(Logger::getInstance(_grabberName.toUpper()))
	, _useImageResampler(true)
	, _videoMode(VideoMode::VIDEO_2D)
	, _videoStandard(VideoStandard::NO_CHANGE)
	, _pixelDecimation(GrabberWrapper::DEFAULT_PIXELDECIMATION)
	, _flipMode(FlipMode::NO_CHANGE)
	, _width(0)
	, _height(0)
	, _fps(GrabberWrapper::DEFAULT_RATE_HZ)
	, _fpsSoftwareDecimation(0)
	, _input(-1)
	, _cropLeft(0)
	, _cropRight(0)
	, _cropTop(0)
	, _cropBottom(0)
	, _isEnabled(true)
	, _isDeviceInError(false)
{
	Grabber::setCropping(cropLeft, cropRight, cropTop, cropBottom);
}

void Grabber::setEnabled(bool enable)
{
	Info(_log,"Capture interface is now %s", enable ? "enabled" : "disabled");
	_isEnabled = enable;
}

void Grabber::setInError(const QString& errorMsg)
{
	_isDeviceInError = true;
	_isEnabled = false;

	Error(_log, "Grabber disabled, device '%s' signals error: '%s'", QSTRING_CSTR(_grabberName), QSTRING_CSTR(errorMsg));
}

void Grabber::setVideoMode(VideoMode mode)
{
	Info(_log,"Set videomode to %s", QSTRING_CSTR(videoMode2String(mode)));
	_videoMode = mode;
	if ( _useImageResampler )
	{
		_imageResampler.setVideoMode(_videoMode);
	}
}

void Grabber::setVideoStandard(VideoStandard videoStandard)
{
	if (_videoStandard != videoStandard) {
		_videoStandard = videoStandard;
	}
}

bool Grabber::setPixelDecimation(int pixelDecimation)
{
	if (_pixelDecimation != pixelDecimation)
	{
		Info(_log,"Set image size decimation to %d", pixelDecimation);
		_pixelDecimation = pixelDecimation;
		if ( _useImageResampler )
		{
			_imageResampler.setHorizontalPixelDecimation(pixelDecimation);
			_imageResampler.setVerticalPixelDecimation(pixelDecimation);
		}

		return true;
	}

	return false;
}

void Grabber::setFlipMode(FlipMode mode)
{
	Info(_log,"Set flipmode to %s", QSTRING_CSTR(flipModeToString(mode)));
	_flipMode = mode;
	if ( _useImageResampler )
	{
		_imageResampler.setFlipMode(_flipMode);
	}
}

void Grabber::setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom)
{
	if (_width>0 && _height>0)
	{
		if (cropLeft + cropRight >= _width || cropTop + cropBottom >= _height)
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
	bool rc (false);
	// eval changes with crop
	if ( (width>0 && height>0) && (_width != width || _height != height) )
	{
		if (_cropLeft + _cropRight >= width || _cropTop + _cropBottom >= height)
		{
			Error(_log, "Rejecting invalid width/height values as it collides with image cropping: width: %d, height: %d", width, height);
			rc = false;
		}
		else
		{
			Debug(_log, "Set new width: %d, height: %d for capture", width, height);
			_width = width;
			_height = height;
			rc = true;
		}
	}
	return rc;
}

bool Grabber::setFramerate(int fps)
{
	Debug(_log,"Set new frames per second to: %i fps, current fps: %i", fps, _fps);

	if((fps > 0) && (_fps != fps))
	{
		Info(_log,"Set new frames per second to: %i fps", fps);
		_fps = fps;
		return true;
	}

	return false;
}

void Grabber::setFpsSoftwareDecimation(int decimation)
{
	if((_fpsSoftwareDecimation != decimation))
	{
		_fpsSoftwareDecimation = decimation;
		if(decimation > 0){
			Debug(_log,"Skip %i frame per second", decimation);
		}
	}
}
