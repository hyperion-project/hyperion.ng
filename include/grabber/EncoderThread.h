#ifndef ENCODERTHREAD_H
#define ENCODERTHREAD_H

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
	#include <jconfig.h>
#endif

constexpr int DEFAULT_THREAD_COUNT {1};

/// Encoder thread for USB devices
class EncoderThread : public QObject
{
	Q_OBJECT
public:
	explicit EncoderThread();
	~EncoderThread() override;

	void setup(
		PixelFormat pixelFormat, uint8_t* sharedData,
		int size, int width, int height, int lineLength,
		int cropLeft, int cropTop, int cropBottom, int cropRight,
		VideoMode videoMode, FlipMode flipMode, int pixelDecimation);

	void process();

	bool isBusy() { return _busy; }
	QAtomicInt _busy = false;

signals:
	void newFrame(const Image<ColorRgb>& data);

private:
	PixelFormat _pixelFormat;
	uint8_t* _localData;
	int	_scalingFactorsCount;
	int	_width;
	int	_height;
	int	_lineLength;
	int	_currentFrame;
	int	_pixelDecimation;
	unsigned long _size;
	int	_cropLeft;
	int _cropTop;
	int _cropBottom;
	int _cropRight;

	FlipMode _flipMode;
	VideoMode _videoMode;
	bool _doTransform;

	ImageResampler		_imageResampler;

#ifdef HAVE_TURBO_JPEG
	tjhandle			_tjInstance;
	tjscalingfactor*	_scalingFactors;
	tjtransform*		_xform;

	void processImageMjpeg();
	bool onError(const QString context) const;
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

	~Thread() override
	{
		quit();
		wait();
	}

	EncoderThread* thread() const { return qobject_cast<EncoderThread*>(_thread); }

	void setup(
		PixelFormat pixelFormat, uint8_t* sharedData,
		int size, int width, int height, int lineLength,
		int cropLeft, int cropTop, int cropBottom, int cropRight,
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
		, _threadCount(static_cast<unsigned long>(qMax(QThread::idealThreadCount(), DEFAULT_THREAD_COUNT)))
		, _threads(nullptr)
	{
		_threads = new Thread<EncoderThread>*[_threadCount];
		for (unsigned long i = 0; i < _threadCount; i++)
		{
			_threads[i] = new Thread<EncoderThread>(new EncoderThread, this);
			_threads[i]->setObjectName("Encoder " + QString::number(i));
		}
	}

	~EncoderThreadManager() override
	{
		if (_threads != nullptr)
		{
			for(unsigned long  i = 0; i < _threadCount; i++)
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
			for (unsigned long  i = 0; i < _threadCount; i++)
				connect(_threads[i]->thread(), &EncoderThread::newFrame, this, &EncoderThreadManager::newFrame);
	}

	void stop()
	{
		if (_threads != nullptr)
			for(unsigned long  i = 0; i < _threadCount; i++)
				disconnect(_threads[i]->thread(), nullptr, nullptr, nullptr);
	}

	unsigned long _threadCount;
	Thread<EncoderThread>**	_threads;

signals:
	void newFrame(const Image<ColorRgb>& data);
};

#endif //ENCODERTHREAD_H
