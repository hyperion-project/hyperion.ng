#pragma once

// Qt includes
#include <QThread>
#include <QSemaphore>

// util includes
#include <utils/PixelFormat.h>
#include <utils/ImageResampler.h>

// TurboJPEG decoder
#ifdef HAVE_TURBO_JPEG
	#include <QImage>
	#include <QColor>
	#include <turbojpeg.h>
#endif

// Forward class declaration
class MFThreadManager;

/// Encoder thread for USB devices
class MFThread : public QThread
{
	Q_OBJECT
	friend class MFThreadManager;

public:
	MFThread();
	~MFThread();

	void setup(
		unsigned int threadIndex, PixelFormat pixelFormat, uint8_t* sharedData,
		int size, int width, int height, int lineLength,
		int subsamp, unsigned cropLeft, unsigned cropTop, unsigned cropBottom, unsigned cropRight,
		VideoMode videoMode, int currentFrame, int pixelDecimation);
	void run();

	bool isBusy();
	void noBusy();

signals:
	void newFrame(unsigned int threadIndex, const Image<ColorRgb>& data, unsigned int sourceCount);

private:
	void processImageMjpeg();

#ifdef HAVE_TURBO_JPEG
	tjhandle 	_decompress;
	tjscalingfactor* _scalingFactors;
#endif

	static volatile bool _isActive;
	volatile bool        _isBusy;
	QSemaphore           _semaphore;
	unsigned int         _workerIndex;
	PixelFormat          _pixelFormat;
	uint8_t*             _localData;
	int                  _localDataSize;
	int                  _scalingFactorsCount;
	int                  _size;
	int                  _width;
	int                  _height;
	int                  _lineLength;
	int                  _subsamp;
	unsigned             _cropLeft;
	unsigned             _cropTop;
	unsigned             _cropBottom;
	unsigned             _cropRight;
	int                  _currentFrame;
	int                  _pixelDecimation;
	ImageResampler       _imageResampler;
};

class MFThreadManager : public QObject
{
	Q_OBJECT

public:
	MFThreadManager() : _threads(nullptr)
	{
		_maxThreads = qBound(1, (QThread::idealThreadCount() > 4 ? (QThread::idealThreadCount() - 1) : QThread::idealThreadCount()), 8);
	}

	~MFThreadManager()
	{
		if (_threads != nullptr)
		{
			for(unsigned i=0; i < _maxThreads; i++)
				if (_threads[i] != nullptr)
				{
					_threads[i]->deleteLater();
					_threads[i] = nullptr;
				}

			delete[] _threads;
			_threads = nullptr;
		}
	}

	void initThreads()
	{
		if (_maxThreads >= 1)
		{
			_threads = new MFThread*[_maxThreads];
			for (unsigned i=0; i < _maxThreads; i++)
				_threads[i] = new MFThread();
		}
	}

	void start() { MFThread::_isActive = true; }
	bool isActive() { return MFThread::_isActive; }

	void stop()
	{
		MFThread::_isActive = false;

		if (_threads != nullptr)
		{
			for(unsigned i=0; i < _maxThreads; i++)
				if (_threads[i] != nullptr)
				{
					_threads[i]->quit();
					_threads[i]->wait();
				}
		}
	}

	unsigned int    _maxThreads;
	MFThread** _threads;
};
