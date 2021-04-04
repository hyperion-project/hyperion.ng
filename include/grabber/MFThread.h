#pragma once

// Qt includes
#include <QThread>

// util includes
#include <utils/PixelFormat.h>
#include <utils/ImageResampler.h>

// TurboJPEG decoder
#ifdef HAVE_TURBO_JPEG
	#include <QImage>
	#include <QColor>
	#include <turbojpeg.h>
#endif

/// Encoder thread for USB devices
class MFThread : public QObject
{
	Q_OBJECT
public:
	explicit MFThread();
	~MFThread();

	void setup(
		PixelFormat pixelFormat, uint8_t* sharedData,
		int size, int width, int height, int lineLength,
		int subsamp, unsigned cropLeft, unsigned cropTop, unsigned cropBottom, unsigned cropRight,
		VideoMode videoMode, FlipMode flipMode, int pixelDecimation);

	void process();

	bool isBusy() { return _busy; }
	QAtomicInt _busy = false;

signals:
	void newFrame(const Image<ColorRgb>& data);

private:
	void processImageMjpeg();

#ifdef HAVE_TURBO_JPEG
	tjhandle			_transform,
			 			_decompress;
	tjscalingfactor*	_scalingFactors;
	tjtransform*		_xform;
#endif

	PixelFormat			_pixelFormat;
	uint8_t*			_localData,
						*_flipBuffer;
	int					_scalingFactorsCount,
						_width,
						_height,
						_lineLength,
						_subsamp,
						_currentFrame,
						_pixelDecimation;
	unsigned long		_size;
	unsigned			_cropLeft,
						_cropTop,
						_cropBottom,
						_cropRight;
	FlipMode			_flipMode;
	ImageResampler		_imageResampler;
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

	MFThread* thread() const { return qobject_cast<MFThread*>(_thread); }

	void setup(
		PixelFormat pixelFormat, uint8_t* sharedData,
		int size, int width, int height, int lineLength,
		int subsamp, unsigned cropLeft, unsigned cropTop, unsigned cropBottom, unsigned cropRight,
		VideoMode videoMode, FlipMode flipMode, int pixelDecimation)
	{
		auto mfthread = qobject_cast<MFThread*>(_thread);
		if (mfthread != nullptr)
			mfthread->setup(pixelFormat, sharedData,
				size, width, height, lineLength,
				subsamp, cropLeft, cropTop, cropBottom, cropRight,
				videoMode, flipMode, pixelDecimation);
	}

	bool isBusy()
	{
		auto mfthread = qobject_cast<MFThread*>(_thread);
		if (mfthread != nullptr)
			return mfthread->isBusy();

		return true;
	}

	void process()
	{
		auto mfthread = qobject_cast<MFThread*>(_thread);
		if (mfthread != nullptr)
			mfthread->process();
	}

protected:
	void run() override
	{
		QThread::run();
		delete _thread;
	}
};

class MFThreadManager : public QObject
{
    Q_OBJECT
public:
	explicit MFThreadManager(QObject *parent = nullptr)
		: QObject(parent)
		, _threadCount(qMax(QThread::idealThreadCount(), 1))
		, _threads(nullptr)
	{
		_threads = new Thread<MFThread>*[_threadCount];
		for (int i = 0; i < _threadCount; i++)
		{
			_threads[i] = new Thread<MFThread>(new MFThread, this);
			_threads[i]->setObjectName("MFThread " + i);
		}
	}

	~MFThreadManager()
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
				connect(_threads[i]->thread(), &MFThread::newFrame, this, &MFThreadManager::newFrame);
	}

	void stop()
	{
		if (_threads != nullptr)
			for(int i = 0; i < _threadCount; i++)
				disconnect(_threads[i]->thread(), nullptr, nullptr, nullptr);
	}

	int					_threadCount;
	Thread<MFThread>**	_threads;

signals:
	void newFrame(const Image<ColorRgb>& data);
};
