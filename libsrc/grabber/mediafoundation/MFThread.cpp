#include "grabber/MFThread.h"

volatile bool MFThread::_isActive = false;

MFThread::MFThread()
	: _localData(nullptr)
	, _localDataSize(0)
	, _decompress(nullptr)
	, _scalingFactorsCount(0)
	, _scalingFactors(nullptr)
	, _isBusy(false)
	, _semaphore(1)
	, _imageResampler()
{
}

MFThread::~MFThread()
{
	if (_decompress == nullptr)
		tjDestroy(_decompress);

	if (_localData != NULL)
	{
		free(_localData);
		_localData = NULL;
		_localDataSize = 0;
	}
}

void MFThread::setup(
	unsigned int threadIndex, PixelFormat pixelFormat, uint8_t* sharedData,
	int size, int width, int height, int lineLength,
	int subsamp, unsigned cropLeft, unsigned cropTop, unsigned cropBottom, unsigned cropRight,
	VideoMode videoMode, int currentFrame, int pixelDecimation)
{
	_workerIndex = threadIndex;
	_lineLength = lineLength;
	_pixelFormat = pixelFormat;
	_size = size;
	_width = width;
	_height = height;
	_subsamp = subsamp;
	_cropLeft = cropLeft;
	_cropTop = cropTop;
	_cropBottom = cropBottom;
	_cropRight = cropRight;
	_currentFrame = currentFrame;
	_pixelDecimation = pixelDecimation;

	_imageResampler.setVideoMode(videoMode);
	_imageResampler.setCropping(cropLeft, cropRight, cropTop, cropBottom);
	_imageResampler.setHorizontalPixelDecimation(_pixelDecimation);
	_imageResampler.setVerticalPixelDecimation(_pixelDecimation);
	_imageResampler.setFlipMode(FlipMode::NO_CHANGE);

	if (size > _localDataSize)
	{
		if (_localData != NULL)
		{
			free(_localData);
			_localData = NULL;
			_localDataSize = 0;
		}
		_localData = (uint8_t *) malloc(size+1);
		_localDataSize = size;
	}
	memcpy(_localData, sharedData, size);
}

void MFThread::run()
{
	if (_isActive && _width > 0 && _height > 0)
	{
		if (_pixelFormat == PixelFormat::MJPEG)
		{
			processImageMjpeg();
		}
		else
		{
			Image<ColorRgb> image = Image<ColorRgb>();
			_imageResampler.processImage(_localData, _width, _height, _lineLength, PixelFormat::BGR24, image);
			emit newFrame(_workerIndex, image, _currentFrame);
		}
	}
}

bool MFThread::isBusy()
{
	bool temp;
	_semaphore.acquire();
	if (_isBusy)
		temp = true;
	else
	{
		temp = false;
		_isBusy = true;
	}
	_semaphore.release();
	return temp;
}

void MFThread::noBusy()
{
	_semaphore.acquire();
	_isBusy = false;
	_semaphore.release();
}

void MFThread::processImageMjpeg()
{
	if (_decompress == nullptr)
	{
		_decompress = tjInitDecompress();
		_scalingFactors = tjGetScalingFactors (&_scalingFactorsCount);
		tjhandle handle=NULL;
	}

	if (tjDecompressHeader2(_decompress, _localData, _size, &_width, &_height, &_subsamp) != 0)
	{
		if (tjGetErrorCode(_decompress) == TJERR_FATAL)
			return;
	}

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
	{
		if (tjGetErrorCode(_decompress) == TJERR_FATAL)
			return;
	}

	// got image, process it
	if ( !(_cropLeft > 0 || _cropTop > 0 || _cropBottom > 0 || _cropRight > 0))
		emit newFrame(_workerIndex, srcImage, _currentFrame);
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
			free(dest);
		}

    	// emit
		emit newFrame(_workerIndex, destImage, _currentFrame);
	}
}
