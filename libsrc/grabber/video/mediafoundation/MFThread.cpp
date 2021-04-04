#include "grabber/MFThread.h"

MFThread::MFThread()
	: _localData(nullptr)
	, _scalingFactorsCount(0)
	, _scalingFactors(nullptr)
	, _transform(nullptr)
	, _decompress(nullptr)
	, _xform(nullptr)
	, _imageResampler()
{}

MFThread::~MFThread()
{
	if (_transform)
		tjDestroy(_transform);

	if (_decompress)
		tjDestroy(_decompress);

	if (_localData)
		tjFree(_localData);
}

void MFThread::setup(
	PixelFormat pixelFormat, uint8_t* sharedData,
	int size, int width, int height, int lineLength,
	int subsamp, unsigned cropLeft, unsigned cropTop, unsigned cropBottom, unsigned cropRight,
	VideoMode videoMode, FlipMode flipMode, int pixelDecimation)
{
	_lineLength = lineLength;
	_pixelFormat = pixelFormat;
	_size = (unsigned long) size;
	_width = width;
	_height = height;
	_subsamp = subsamp;
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

	if (_localData)
		tjFree(_localData);

	_localData = (uint8_t*)tjAlloc(size + 1);
	memcpy(_localData, sharedData, size);
}

void MFThread::process()
{
	_busy = true;
	if (_width > 0 && _height > 0)
	{
		if (_pixelFormat == PixelFormat::MJPEG)
		{
			processImageMjpeg();
		}
		else
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
			_imageResampler.processImage(_localData, _width, _height, _lineLength, PixelFormat::BGR24, image);
			emit newFrame(image);
		}
	}
	_busy = false;
}

void MFThread::processImageMjpeg()
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
