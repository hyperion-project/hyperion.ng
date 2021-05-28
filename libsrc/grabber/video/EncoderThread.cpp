#include "grabber/EncoderThread.h"

EncoderThread::EncoderThread()
	: _localData(nullptr)
	, _scalingFactorsCount(0)
	, _imageResampler()
#ifdef HAVE_TURBO_JPEG
	, _transform(nullptr)
	, _decompress(nullptr)
	, _scalingFactors(nullptr)
	, _xform(nullptr)
#endif
{}

EncoderThread::~EncoderThread()
{
#ifdef HAVE_TURBO_JPEG
	if (_transform)
		tjDestroy(_transform);

	if (_decompress)
		tjDestroy(_decompress);
#endif

	if (_localData)
#ifdef HAVE_TURBO_JPEG
		tjFree(_localData);
#else
		delete[] _localData;
#endif
}

void EncoderThread::setup(
	PixelFormat pixelFormat, uint8_t* sharedData,
	int size, int width, int height, int lineLength,
	unsigned cropLeft, unsigned cropTop, unsigned cropBottom, unsigned cropRight,
	VideoMode videoMode, FlipMode flipMode, int pixelDecimation)
{
	_lineLength = lineLength;
	_pixelFormat = pixelFormat;
	_size = (unsigned long) size;
	_width = width;
	_height = height;
	_cropLeft = cropLeft;
	_cropTop = cropTop;
	_cropBottom = cropBottom;
	_cropRight = cropRight;
	_flipMode = flipMode;
	_pixelDecimation = pixelDecimation;

	_imageResampler.setVideoMode(videoMode);
	_imageResampler.setFlipMode(_flipMode);
	_imageResampler.setCropping(cropLeft, cropRight, cropTop, cropBottom);
	_imageResampler.setHorizontalPixelDecimation(_pixelDecimation);
	_imageResampler.setVerticalPixelDecimation(_pixelDecimation);

#ifdef HAVE_TURBO_JPEG
	if (_localData)
		tjFree(_localData);

	_localData = (uint8_t*)tjAlloc(size + 1);
#else
	delete[] _localData;
	_localData = nullptr;
	_localData = new uint8_t(size + 1);
#endif

	memcpy(_localData, sharedData, size);
}

void EncoderThread::process()
{
	_busy = true;
	if (_width > 0 && _height > 0)
	{
#ifdef HAVE_TURBO_JPEG
		if (_pixelFormat == PixelFormat::MJPEG)
		{
			processImageMjpeg();
		}
		else
#endif
		{
			if (_pixelFormat == PixelFormat::BGR24)
			{
				if (_flipMode == FlipMode::NO_CHANGE)
					_imageResampler.setFlipMode(FlipMode::HORIZONTAL);
				else if (_flipMode == FlipMode::HORIZONTAL)
					_imageResampler.setFlipMode(FlipMode::NO_CHANGE);
				else if (_flipMode == FlipMode::VERTICAL)
					_imageResampler.setFlipMode(FlipMode::BOTH);
				else if (_flipMode == FlipMode::BOTH)
					_imageResampler.setFlipMode(FlipMode::VERTICAL);
			}

			Image<ColorRgb> image = Image<ColorRgb>();
			_imageResampler.processImage(
				_localData,
				_width,
				_height,
				_lineLength,
#if defined(ENABLE_V4L2)
				_pixelFormat,
#else
				PixelFormat::BGR24,
#endif
				image
			);

			emit newFrame(image);
		}
	}
	_busy = false;
}

#ifdef HAVE_TURBO_JPEG
void EncoderThread::processImageMjpeg()
{
	if (!_transform && _flipMode != FlipMode::NO_CHANGE)
	{
		_transform = tjInitTransform();
		_xform = new tjtransform();
	}

	if (_flipMode == FlipMode::BOTH || _flipMode == FlipMode::HORIZONTAL)
	{
		_xform->op = TJXOP_HFLIP;
		tjTransform(_transform, _localData, _size, 1, &_localData, &_size, _xform, TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE);
	}

	if (_flipMode == FlipMode::BOTH || _flipMode == FlipMode::VERTICAL)
	{
		_xform->op = TJXOP_VFLIP;
		tjTransform(_transform, _localData, _size, 1, &_localData, &_size, _xform, TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE);
	}

	if (!_decompress)
	{
		_decompress = tjInitDecompress();
		_scalingFactors = tjGetScalingFactors(&_scalingFactorsCount);
	}

	int subsamp = 0;
	if (tjDecompressHeader2(_decompress, _localData, _size, &_width, &_height, &subsamp) != 0)
		return;

	int scaledWidth = _width, scaledHeight = _height;
	if(_scalingFactors != nullptr && _pixelDecimation > 1)
	{
		for (int i = 0; i < _scalingFactorsCount ; i++)
		{
			const int tempWidth = TJSCALED(_width, _scalingFactors[i]);
			const int tempHeight = TJSCALED(_height, _scalingFactors[i]);
			if (tempWidth <= _width/_pixelDecimation && tempHeight <= _height/_pixelDecimation)
			{
				scaledWidth = tempWidth;
				scaledHeight = tempHeight;
				break;
			}
		}

		if (scaledWidth == _width && scaledHeight == _height)
		{
			scaledWidth = TJSCALED(_width, _scalingFactors[_scalingFactorsCount-1]);
			scaledHeight = TJSCALED(_height, _scalingFactors[_scalingFactorsCount-1]);
		}
	}

	Image<ColorRgb> srcImage(scaledWidth, scaledHeight);

	if (tjDecompress2(_decompress, _localData , _size, (unsigned char*)srcImage.memptr(), scaledWidth, 0, scaledHeight, TJPF_RGB, TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE) != 0)
			return;

	// got image, process it
	if (!(_cropLeft > 0 || _cropTop > 0 || _cropBottom > 0 || _cropRight > 0))
		emit newFrame(srcImage);
	else
    {
		// calculate the output size
		int outputWidth = (_width - _cropLeft - _cropRight);
		int outputHeight = (_height - _cropTop - _cropBottom);

		if (outputWidth <= 0 || outputHeight <= 0)
			return;

		Image<ColorRgb> destImage(outputWidth, outputHeight);

		for (unsigned int y = 0; y < destImage.height(); y++)
		{
			unsigned char* source = (unsigned char*)srcImage.memptr() + (y + _cropTop)*srcImage.width()*3 + _cropLeft*3;
			unsigned char* dest = (unsigned char*)destImage.memptr() + y*destImage.width()*3;
			memcpy(dest, source, destImage.width()*3);
			free(source);
			source = nullptr;
			free(dest);
			dest = nullptr;
		}

    	// emit
		emit newFrame(destImage);
	}
}
#endif
