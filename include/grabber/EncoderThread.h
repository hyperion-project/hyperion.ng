#pragma once

// Qt includes
#include <QThread>

// util includes
#include <utils/PixelFormat.h>
#include <utils/ImageResampler.h>

// Determine the cmake options
#include <HyperionConfig.h>

// Turbo JPEG decoder
#ifdef HAVE_TURBO_JPEG
	#include <turbojpeg.h>
#endif

/// Encoder thread for USB devices
class EncoderThread : public QObject
{
	Q_OBJECT
public:
	explicit EncoderThread();
	~EncoderThread();

	void setup(
		PixelFormat pixelFormat, uint8_t* sharedData,
		int size, int width, int height, int lineLength,
		unsigned cropLeft, unsigned cropTop, unsigned cropBottom, unsigned cropRight,
		VideoMode videoMode, FlipMode flipMode, int pixelDecimation);

	void process();

	bool isBusy() { return _busy; }
	QAtomicInt _busy = false;

signals:
	void newFrame(const Image<ColorRgb>& data);

private:
	PixelFormat			_pixelFormat;
	uint8_t*			_localData,
						*_flipBuffer;
	int					_scalingFactorsCount,
						_width,
						_height,
						_lineLength,
						_currentFrame,
						_pixelDecimation;
	unsigned long		_size;
	unsigned			_cropLeft,
						_cropTop,
						_cropBottom,
						_cropRight;
	FlipMode			_flipMode;
	ImageResampler		_imageResampler;

#ifdef HAVE_TURBO_JPEG
	tjhandle			_transform, _decompress;
	tjscalingfactor*	_scalingFactors;
	tjtransform*		_xform;

	void processImageMjpeg();
#endif
};

template <typename TThread> class Thread : public QThread
{
public:
	TThread *_thread;
	explicit Thread(TThread *thread, QObject *parent = nullptr)
		: QThread(parent)
		, _thread(thread)
	{
		_thread->moveToThread(this);
		start();
	}

	~Thread()
	{
		quit();
		wait();
	}

	EncoderThread* thread() const { return qobject_cast<EncoderThread*>(_thread); }

	void setup(
		PixelFormat pixelFormat, uint8_t* sharedData,
		int size, int width, int height, int lineLength,
		unsigned cropLeft, unsigned cropTop, unsigned cropBottom, unsigned cropRight,
		VideoMode videoMode, FlipMode flipMode, int pixelDecimation)
	{
		auto encThread = qobject_cast<EncoderThread*>(_thread);
		if (encThread != nullptr)
			encThread->setup(pixelFormat, sharedData,
				size, width, height, lineLength,
				cropLeft, cropTop, cropBottom, cropRight,
				videoMode, flipMode, pixelDecimation);
	}

	bool isBusy()
	{
		auto encThread = qobject_cast<EncoderThread*>(_thread);
		if (encThread != nullptr)
			return encThread->isBusy();

		return true;
	}

	void process()
	{
		auto encThread = qobject_cast<EncoderThread*>(_thread);
		if (encThread != nullptr)
			encThread->process();
	}

protected:
	void run() override
	{
		QThread::run();
		delete _thread;
	}
};

class EncoderThreadManager : public QObject
{
    Q_OBJECT
public:
	explicit EncoderThreadManager(QObject *parent = nullptr)
		: QObject(parent)
		, _threadCount(qMax(QThread::idealThreadCount(), 1))
		, _threads(nullptr)
	{
		_threads = new Thread<EncoderThread>*[_threadCount];
		for (int i = 0; i < _threadCount; i++)
		{
			_threads[i] = new Thread<EncoderThread>(new EncoderThread, this);
			_threads[i]->setObjectName("Encoder " + i);
		}
	}

	~EncoderThreadManager()
	{
		if (_threads != nullptr)
		{
			for(int i = 0; i < _threadCount; i++)
			{
				_threads[i]->deleteLater();
				_threads[i] = nullptr;
			}

			delete[] _threads;
			_threads = nullptr;
		}
	}

	void start()
	{
		if (_threads != nullptr)
			for (int i = 0; i < _threadCount; i++)
				connect(_threads[i]->thread(), &EncoderThread::newFrame, this, &EncoderThreadManager::newFrame);
	}

	void stop()
	{
		if (_threads != nullptr)
			for(int i = 0; i < _threadCount; i++)
				disconnect(_threads[i]->thread(), nullptr, nullptr, nullptr);
	}

	int					_threadCount;
	Thread<EncoderThread>**	_threads;

signals:
	void newFrame(const Image<ColorRgb>& data);
};
