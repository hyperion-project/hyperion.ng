#include "grabber/EncoderThread.h"

#include <QDebug>

EncoderThread::EncoderThread()
	: _localData(nullptr)
	, _scalingFactorsCount(0)
	, _doTransform(false)
	,_imageResampler()
	#ifdef HAVE_TURBO_JPEG
	, _tjInstance(nullptr)
	, _scalingFactors(nullptr)
	, _xform(nullptr)
	#endif
{
#ifdef HAVE_TURBO_JPEG
	_scalingFactors = tjGetScalingFactors(&_scalingFactorsCount);
#endif
}

EncoderThread::~EncoderThread()
{
#ifdef HAVE_TURBO_JPEG
	if (_tjInstance)
		tjDestroy(_tjInstance);
#endif

	if (_localData != nullptr)
	{
#ifdef HAVE_TURBO_JPEG
		tjFree(_localData);
#else
		delete[] _localData;
#endif
		_localData = nullptr;
	}
}

void EncoderThread::setup(
		PixelFormat pixelFormat, uint8_t* sharedData,
		int size, int width, int height, int lineLength,
		int cropLeft, int cropTop, int cropBottom, int cropRight,
		VideoMode videoMode, FlipMode flipMode, int pixelDecimation)
{
	_lineLength = lineLength;
	_pixelFormat = pixelFormat;
	_size = static_cast<unsigned long>(size);
	_width = width;
	_height = height;
	_cropLeft = cropLeft;
	_cropTop = cropTop;
	_cropBottom = cropBottom;
	_cropRight = cropRight;
	_flipMode = flipMode;
	_videoMode = videoMode;
	_pixelDecimation = pixelDecimation;

	bool needTransform {false};

	if (_cropLeft > 0 || _cropTop > 0 || _cropBottom > 0 || _cropRight > 0 ||
		_flipMode != FlipMode::NO_CHANGE ||
		_videoMode !=  VideoMode::VIDEO_2D)
	{
		needTransform = true;
	}
	else
	{
		needTransform = false;
	}

#ifdef HAVE_TURBO_JPEG
	if (_doTransform != needTransform )
	{
		if (_tjInstance != nullptr)
		{
			tjDestroy(_tjInstance);
			_tjInstance = nullptr;
		}
		_doTransform = needTransform;
	}
#endif

	_imageResampler.setVideoMode(_videoMode);
	_imageResampler.setFlipMode(_flipMode);
	_imageResampler.setCropping(_cropLeft, _cropRight, _cropTop, _cropBottom);
	_imageResampler.setHorizontalPixelDecimation(_pixelDecimation);
	_imageResampler.setVerticalPixelDecimation(_pixelDecimation);

#ifdef HAVE_TURBO_JPEG
	if (_localData != nullptr)
	{
		tjFree(_localData);
		_localData = nullptr;
	}
	_localData = static_cast<uint8_t*>(tjAlloc(size + 1));
#else
	if (_localData != nullptr)
	{
		delete[] _localData;
		_localData = nullptr;
	}

	_localData = new uint8_t[size];
#endif

	if (_localData != nullptr)
	{
		memcpy(_localData, sharedData, static_cast<size_t>(size));
	}
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
	int inSubsamp {0};
	int inColorspace {0};

	if (_doTransform)
	{
		if (!_tjInstance)
		{
			_tjInstance = tjInitTransform();
			_xform = new tjtransform();
		}

		if (tjDecompressHeader3(_tjInstance, _localData, _size, &_width, &_height, &inSubsamp, &inColorspace) < 0)
		{
			if (onError("_doTransform - tjDecompressHeader3"))
			{
				return;
			}
		}

		int transformedWidth {_width};
		int transformedHeight {_height};

		// handle 3D mode
		switch (_videoMode)
		{
		case VideoMode::VIDEO_3DSBS:
			transformedWidth = transformedWidth >> 1;
			_cropLeft = _cropLeft >> 1;
			_cropRight = _cropRight >> 1;
			break;
		case VideoMode::VIDEO_3DTAB:
			transformedHeight = transformedHeight >> 1;
			_cropTop = _cropTop >> 1;
			_cropBottom = _cropBottom >> 1;
			break;
		default:
			break;
		}

		if (_cropLeft > 0 || _cropTop > 0 || _cropBottom > 0 || _cropRight > 0)
		{
			int mcuWidth = tjMCUWidth[inSubsamp];
			int mcuHeight = tjMCUHeight[inSubsamp];

			_cropLeft = _cropLeft - _cropLeft % mcuWidth;
			_cropTop = _cropTop - _cropTop % mcuHeight;

			int croppedWidth = transformedWidth - _cropLeft - _cropRight;
			int croppeddHeight = transformedHeight - _cropTop - _cropBottom;

			if (croppedWidth >= 0)
			{
				transformedWidth = croppedWidth;
			}

			if (croppeddHeight >= 0)
			{
				transformedHeight = croppeddHeight;
			}

		}

		if ( transformedWidth != _width ||  transformedHeight != _height  )
		{
			_xform->options = TJXOPT_CROP;
			_xform->r = tjregion {_cropLeft,_cropTop,transformedWidth,transformedHeight};
			//qDebug() << "processImageMjpeg() | _doTransform - Image cropped: transformedWidth: " << transformedWidth << " transformedHeight: " << transformedHeight;
		}
		else
		{
			_xform->options = 0;
			_xform->r = tjregion {0,0,_width,_height};
			//qDebug() << "processImageMjpeg() | _doTransform - Image not cropped: _width: " << _width << " _height: " << _height;
		}
		_xform->options |= TJXOPT_TRIM;

		switch (_flipMode) {
		case FlipMode::HORIZONTAL:
			_xform->op = TJXOP_HFLIP;
			break;
		case FlipMode::VERTICAL:
			_xform->op = TJXOP_VFLIP;
			break;
		case FlipMode::BOTH:
			_xform->op = TJXOP_ROT180;
			break;
		default:
			_xform->op = TJXOP_NONE;
			break;
		}

		unsigned char *dstBuf = nullptr;  /* Dynamically allocate the JPEG buffer */
		unsigned long dstSize = 0;

		if(tjTransform(_tjInstance, _localData, _size, 1, &dstBuf, &dstSize, _xform, TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE) < 0 )
		{
			if (onError("_doTransform - tjTransform"))
			{
				return;
			}
		}

		tjFree(_localData);
		_localData = dstBuf;
		_size = dstSize;
	}
	else
	{
		if (!_tjInstance)
		{
			_tjInstance = tjInitDecompress();
		}
	}

	if (_doTransform)
	{
		if (tjDecompressHeader3(_tjInstance, _localData, _size, &_width, &_height,	&inSubsamp, &inColorspace) < 0)
		{
			if (onError("get image details - tjDecompressHeader3"))
			{
				return;
			}
		}
	}
	else
	{
		if (tjDecompressHeader2(_tjInstance, _localData, _size, &_width, &_height, &inSubsamp) < 0)
		{
			if (onError("get image details - tjDecompressHeader2"))
			{
				return;
			}
		}
	}

	if(_scalingFactors != nullptr && _pixelDecimation > 1)
	{
		//Scaling factors will not do map 1:1 to pixel decimation, but do a best match
		//In case perfect pixel decimation is required, code needs to make use of a interim image.

		bool scaleingFactorFound {false};
		for (int i = 0; i < _scalingFactorsCount ; i++)
		{
			const int tempWidth = TJSCALED(_width, _scalingFactors[i]);
			const int tempHeight = TJSCALED(_height, _scalingFactors[i]);

			if (tempWidth <= _width/_pixelDecimation && tempHeight <= _height/_pixelDecimation)
			{
				_width = tempWidth;
				_height = tempHeight;
				scaleingFactorFound = true;
				break;
			}
		}

		if (!scaleingFactorFound)
		{
			//Set to smallest scaling factor
			_width = TJSCALED(_width, _scalingFactors[_scalingFactorsCount-1]);
			_height = TJSCALED(_height, _scalingFactors[_scalingFactorsCount-1]);
		}
	}

	Image<ColorRgb> srcImage(static_cast<unsigned>(_width), static_cast<unsigned>(_height));

	if (tjDecompress2(_tjInstance, _localData , _size,
					  reinterpret_cast<unsigned char*>(srcImage.memptr()), _width, 0, _height,
					  TJPF_RGB, TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE)
		< 0)
	{
		if (onError("get final image - tjDecompress2"))
		{
			return;
		}
	}
	emit newFrame(srcImage);
}
#endif

#ifdef HAVE_TURBO_JPEG
bool EncoderThread::onError(const QString context) const
{
	bool treatAsError {false};

#if LIBJPEG_TURBO_VERSION_NUMBER > 2000000
	if (tjGetErrorCode(_tjInstance) == TJERR_FATAL)
	{
		//qDebug() << context << "Error: " << QString(tjGetErrorStr2(_tjInstance));
		treatAsError = true;
	}
#else
	//qDebug() << context << "Error: " << QString(tjGetErrorStr());
	treatAsError = true;
#endif

return treatAsError;
}
#endif
