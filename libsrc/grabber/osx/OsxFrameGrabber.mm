// STL includes
#include <algorithm>
#include <chrono>
#include <mutex>

// Header
#include <grabber/osx/OsxFrameGrabber.h>

// ScreenCaptureKit
#if defined(SDK_15_AVAILABLE)
#include <ScreenCaptureKit/ScreenCaptureKit.h>
#include <CoreVideo/CoreVideo.h>
#endif

//Qt
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

namespace
{
	/// Timeout for asynchronous ScreenCaptureKit calls
	constexpr int64_t SCK_CALL_TIMEOUT_MS = 5000;

	/// Minimum interval between two attempts to establish the continuous capture session
	constexpr int64_t SCK_STREAM_RETRY_INTERVAL_MS = 5000;

	/// Time to wait for the first frame after the continuous capture session was started
	constexpr int64_t SCK_STREAM_START_GRACE_MS = 2000;

	///
	/// @brief Current steady clock time in milliseconds
	///
	int64_t currentTimeMs()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(
				   std::chrono::steady_clock::now().time_since_epoch())
			.count();
	}

#if defined(SDK_15_AVAILABLE)
	///
	/// @brief Creates a stream/screenshot configuration for the given display.
	///        Depending on gpuScaling, the display is either captured in its native resolution
	///        (down scaling remains with the ImageResampler) or scaled down to the analysis
	///        resolution by the capture session (GPU).
	///
	SCStreamConfiguration* createStreamConfiguration(CGDirectDisplayID displayId, int fps, bool gpuScaling, int pixelDecimation, QSize& streamSize)
	{
		SCStreamConfiguration* config = [[SCStreamConfiguration alloc] init];

		double scaleFactor = 1.0;
		CGDisplayModeRef modeRef = CGDisplayCopyDisplayMode(displayId);
		if (modeRef != nullptr)
		{
			const size_t pointWidth = CGDisplayModeGetWidth(modeRef);
			if (pointWidth > 0)
			{
				scaleFactor = static_cast<double>(CGDisplayModeGetPixelWidth(modeRef)) / static_cast<double>(pointWidth);
			}
			CGDisplayModeRelease(modeRef);
		}

		const CGRect displayBounds = CGDisplayBounds(displayId);
		config.sourceRect = displayBounds;

		const int captureWidth = std::max(1, static_cast<int>(displayBounds.size.width * scaleFactor));
		const int captureHeight = std::max(1, static_cast<int>(displayBounds.size.height * scaleFactor));
		const int decimation = std::max(1, pixelDecimation);

		if (gpuScaling)
		{
			// The GPU scales the display down to the analysis resolution, i.e. the ImageResampler
			// converts the image 1:1 without touching the full resolution frame buffer.
			// Same calculation as ImageResampler::processImage to keep the analysis resolution stable.
			streamSize = QSize(std::max(1, (captureWidth - (decimation >> 1) + decimation - 1) / decimation),
							   std::max(1, (captureHeight - (decimation >> 1) + decimation - 1) / decimation));
			config.captureResolution = SCCaptureResolutionAutomatic;
		}
		else
		{
			streamSize = QSize(captureWidth, captureHeight);
			config.captureResolution = SCCaptureResolutionBest;
		}

		config.width = static_cast<size_t>(streamSize.width());
		config.height = static_cast<size_t>(streamSize.height());
		config.scalesToFit = NO;
		// Required, as the frame data is interpreted as BGR32 by the ImageResampler
		config.pixelFormat = kCVPixelFormatType_32BGRA;
		config.queueDepth = 5;
		config.minimumFrameInterval = CMTimeMake(1, fps > 0 ? fps : 25);

		return config;
	}
#endif
}

#if defined(SDK_15_AVAILABLE)
	///
	/// @brief Receives the frames delivered by the ScreenCaptureKit stream and keeps the most
	///        recent complete one.
	///        Frames are delivered on the stream's own dispatch queue, i.e. access to the
	///        retained frame is guarded by a mutex.
	///
	@interface OsxStreamOutput : NSObject <SCStreamOutput, SCStreamDelegate>
	{
	@public
		/// Most recent complete frame (retained)
		CVPixelBufferRef _latestFrame;

		/// Guards the access to _latestFrame, _streamStopped and _stopReason
		std::mutex _frameMutex;

		/// Set to true when the stream stopped unexpectedly
		bool _streamStopped;

		/// Reason why the stream stopped
		NSString* _stopReason;

		/// Queue on which the frame callbacks are delivered
		dispatch_queue_t _frameQueue;
	}
	@end

	@implementation OsxStreamOutput

	- (instancetype)init
	{
		if ((self = [super init]) != nil)
		{
			_latestFrame = nullptr;
			_streamStopped = false;
			_stopReason = nil;
			_frameQueue = nullptr;
		}
		return self;
	}

	- (void)dealloc
	{
		[self releaseLatestFrame];
		[_stopReason release];
		if (_frameQueue != nullptr)
		{
			dispatch_release(_frameQueue);
			_frameQueue = nullptr;
		}
		[super dealloc];
	}

	///
	/// @brief Provides the queue used to receive the frames
	///
	- (dispatch_queue_t)frameQueue
	{
		if (_frameQueue == nullptr)
		{
			_frameQueue = dispatch_queue_create("org.hyperion-project.hyperion.screencapture", DISPATCH_QUEUE_SERIAL);
		}
		return _frameQueue;
	}

	///
	/// @brief Releases the most recent frame
	///
	- (void)releaseLatestFrame
	{
		CVPixelBufferRef frame = nullptr;
		{
			std::lock_guard<std::mutex> lock(_frameMutex);
			frame = _latestFrame;
			_latestFrame = nullptr;
		}

		if (frame != nullptr)
		{
			CVPixelBufferRelease(frame);
		}
	}

	///
	/// @brief Provides the most recent complete frame
	/// @return The most recent frame or nullptr, if no complete frame was received yet.
	///         The caller owns the returned frame.
	///
	- (CVPixelBufferRef)copyLatestFrame
	{
		std::lock_guard<std::mutex> lock(_frameMutex);
		if (_latestFrame != nullptr)
		{
			CVPixelBufferRetain(_latestFrame);
		}
		return _latestFrame;
	}

	///
	/// @brief Determine whether the stream stopped unexpectedly
	///
	- (bool)hasStopped
	{
		std::lock_guard<std::mutex> lock(_frameMutex);
		return _streamStopped;
	}

	///
	/// @brief Provides the reason why the stream stopped
	///
	- (NSString*)stopReason
	{
		std::lock_guard<std::mutex> lock(_frameMutex);
		return [[_stopReason retain] autorelease];
	}

	#pragma mark - SCStreamOutput

	- (void)stream:(SCStream*)stream didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer ofType:(SCStreamOutputType)type
	{
		(void)stream;

		@autoreleasepool
		{
			if (type != SCStreamOutputTypeScreen || !CMSampleBufferIsValid(sampleBuffer))
			{
				return;
			}

			// Only complete frames provide an image, e.g. idle or blank frames do not
			NSArray* attachments = (NSArray*)CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, false);
			if (attachments.count == 0)
			{
				return;
			}

			NSDictionary* frameInfo = attachments[0];
			NSNumber* frameStatus = frameInfo[SCStreamFrameInfoStatus];
			if (frameStatus == nil)
			{
				return;
			}

			// "Complete" frames contain a new image. "Started" is the first frame after the
			// stream was started, which is used until the screen changes again. Idle/blank or
			// suspended frames do not provide a new image and are ignored.
			const SCFrameStatus status = static_cast<SCFrameStatus>(frameStatus.integerValue);
			if (status != SCFrameStatusComplete && status != SCFrameStatusStarted)
			{
				return;
			}

			CVPixelBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
			if (pixelBuffer == nullptr)
			{
				return;
			}

			CVPixelBufferRetain(pixelBuffer);

			CVPixelBufferRef previousFrame = nullptr;
			{
				std::lock_guard<std::mutex> lock(_frameMutex);
				previousFrame = _latestFrame;
				_latestFrame = pixelBuffer;
			}

			if (previousFrame != nullptr)
			{
				CVPixelBufferRelease(previousFrame);
			}
		}
	}

	#pragma mark - SCStreamDelegate

	- (void)stream:(SCStream*)stream didStopWithError:(NSError*)error
	{
		(void)stream;

		@autoreleasepool
		{
			std::lock_guard<std::mutex> lock(_frameMutex);
			_streamStopped = true;
			[_stopReason release];
			_stopReason = [[error localizedDescription] copy];
		}
	}

	@end
#endif

OsxFrameGrabber::OsxFrameGrabber(int display)
	: Grabber("GRABBER-OSX")
	, _screenIndex(display)
	, _display(kCGDirectMainDisplay)
	, _stream(nullptr)
	, _streamOutput(nullptr)
	, _contentFilter(nullptr)
	, _lastStreamAttemptMs(0)
	, _streamStartedMs(0)
	, _gpuScalingMode(false)
	, _streamSize()
{
	_isEnabled = false;
	_useImageResampler = true;
}

OsxFrameGrabber::~OsxFrameGrabber()
{
	stopStream();
}

bool OsxFrameGrabber::setupDisplay()
{
#if defined(SDK_15_AVAILABLE)
	if (!CGPreflightScreenCaptureAccess())
	{
		if(!CGRequestScreenCaptureAccess())
		{
			Error(_log, "Screen capture permission required to start the grabber");
			return false;
		}
	}
#endif

	return setDisplayIndex(_screenIndex);
}

bool OsxFrameGrabber::isStreamActive()
{
#if defined(SDK_15_AVAILABLE)
	if (_stream == nullptr || _streamOutput == nullptr)
	{
		return false;
	}

	return ![static_cast<OsxStreamOutput*>(_streamOutput) hasStopped];
#else
	return false;
#endif
}

bool OsxFrameGrabber::setupCaptureSession()
{
#if defined(SDK_15_AVAILABLE)
	__block SCShareableContent* shareableContent = nil;

	// Enumerate the shareable content once. Doing this per frame is a ScreenCaptureKit
	// anti-pattern, as it forces the WindowServer to enumerate all displays, windows and
	// applications for every single frame.
	dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
	[SCShareableContent getShareableContentWithCompletionHandler:^(SCShareableContent* content, NSError* error)
	{
		@autoreleasepool
		{
			if (error == nil && content != nil)
			{
				shareableContent = [content retain];
			}
			dispatch_semaphore_signal(semaphore);
		}
	}];
	dispatch_semaphore_wait(semaphore, dispatch_time(DISPATCH_TIME_NOW, SCK_CALL_TIMEOUT_MS * NSEC_PER_MSEC));
	dispatch_release(semaphore);

	if (shareableContent == nil)
	{
		Error(_log, "Failed to retrieve the shareable content to capture display %u", _display);
		return false;
	}

	SCDisplay* targetDisplay = nil;
	for (SCDisplay* display in shareableContent.displays)
	{
		if (display.displayID == _display)
		{
			targetDisplay = display;
			break;
		}
	}

	if (targetDisplay == nil)
	{
		[shareableContent release];
		Error(_log, "Display %u is not available for capture", _display);
		return false;
	}

	if (_contentFilter != nullptr)
	{
		[static_cast<SCContentFilter*>(_contentFilter) release];
	}
	_contentFilter = [[SCContentFilter alloc] initWithDisplay:targetDisplay excludingWindows:@[]];

	[shareableContent release];

	return true;
#else
	return false;
#endif
}

void OsxFrameGrabber::teardownStream()
{
#if defined(SDK_15_AVAILABLE)
	_streamStartedMs = 0;

	if (_stream != nullptr)
	{
		SCStream* stream = static_cast<SCStream*>(_stream);
		_stream = nullptr;

		const bool streamStopped = _streamOutput != nullptr && [static_cast<OsxStreamOutput*>(_streamOutput) hasStopped];
		if (!streamStopped)
		{
			dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
			[stream stopCaptureWithCompletionHandler:^(NSError* /*error*/)
			{
				dispatch_semaphore_signal(semaphore);
			}];
			dispatch_semaphore_wait(semaphore, dispatch_time(DISPATCH_TIME_NOW, SCK_CALL_TIMEOUT_MS * NSEC_PER_MSEC));
			dispatch_release(semaphore);
		}

		[stream release];
	}

	if (_streamOutput != nullptr)
	{
		[static_cast<OsxStreamOutput*>(_streamOutput) release];
		_streamOutput = nullptr;
	}
#endif
}

bool OsxFrameGrabber::startStream()
{
#if defined(SDK_15_AVAILABLE)
	if (isStreamActive())
	{
		return true;
	}

	const int64_t now = currentTimeMs();
	if (_lastStreamAttemptMs > 0 && (now - _lastStreamAttemptMs) < SCK_STREAM_RETRY_INTERVAL_MS)
	{
		// Do not hammer the system with repeated start attempts
		return false;
	}
	_lastStreamAttemptMs = now;

	// Remove a stream that is no longer active. The capture session is kept, so that a stream
	// can be established without another shareable content enumeration
	teardownStream();

	// Screen capturing requires the user's consent
	if (!CGPreflightScreenCaptureAccess())
	{
		// Ask for the permission only once, subsequent calls just return the current state
		static bool permissionRequested = false;
		if (!permissionRequested)
		{
			permissionRequested = true;
			CGRequestScreenCaptureAccess();
		}
		Error(_log, "Screen capture permission required to start the grabber");
		return false;
	}

	if (_contentFilter == nullptr && !setupCaptureSession())
	{
		return false;
	}

	const bool gpuScaling = useGpuScaling();
	QSize streamSize;
	SCStreamConfiguration* config = createStreamConfiguration(_display, _fps, gpuScaling, _pixelDecimation, streamSize);

	OsxStreamOutput* streamOutput = [[OsxStreamOutput alloc] init];

	NSError* error = nil;
	SCStream* stream = [[SCStream alloc] initWithFilter:static_cast<SCContentFilter*>(_contentFilter) configuration:config delegate:streamOutput];
	const bool outputAdded = stream != nil && [stream addStreamOutput:streamOutput type:SCStreamOutputTypeScreen sampleHandlerQueue:[streamOutput frameQueue] error:&error];
	[config release];

	if (!outputAdded)
	{
		Error(_log, "Failed to register the screen capture output: %s", error != nil ? [[error localizedDescription] UTF8String] : "unknown error");
		[stream release];
		[streamOutput release];
		return false;
	}

	__block NSError* startError = nil;
	dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
	[stream startCaptureWithCompletionHandler:^(NSError* captureError)
	{
		if (captureError != nil)
		{
			startError = [captureError retain];
		}
		dispatch_semaphore_signal(semaphore);
	}];
	const long waitResult = dispatch_semaphore_wait(semaphore, dispatch_time(DISPATCH_TIME_NOW, SCK_CALL_TIMEOUT_MS * NSEC_PER_MSEC));
	dispatch_release(semaphore);

	if (waitResult != 0 || startError != nil)
	{
		Error(_log, "Failed to start the screen capture stream: %s", startError != nil ? [[startError localizedDescription] UTF8String] : "timeout");
		[startError release];
		[stream release];
		[streamOutput release];
		return false;
	}

	_stream = stream;
	_streamOutput = streamOutput;
	_lastStreamAttemptMs = 0;
	_streamStartedMs = currentTimeMs();
	_gpuScalingMode = gpuScaling;
	_streamSize = streamSize;

	Info(_log, "Screen capture stream for display %u started, resolution: %dx%d (%s), analysis resolution: %dx%d",
		 _display, streamSize.width(), streamSize.height(),
		 gpuScaling ? "GPU scaled" : "native", analysisSize().width(), analysisSize().height());

	return true;
#else
	return false;
#endif
}

void OsxFrameGrabber::stopStream()
{
	teardownStream();

#if defined(SDK_15_AVAILABLE)
	if (_contentFilter != nullptr)
	{
		[static_cast<SCContentFilter*>(_contentFilter) release];
		_contentFilter = nullptr;
	}
#endif

	_lastStreamAttemptMs = 0;
}

bool OsxFrameGrabber::useGpuScaling() const
{
	// Cropping and the 3D modes are applied by the ImageResampler, which requires the display to
	// be captured in its native resolution
	return _cropLeft == 0 && _cropRight == 0 && _cropTop == 0 && _cropBottom == 0 && _videoMode == VideoMode::VIDEO_2D;
}

QSize OsxFrameGrabber::captureSize() const
{
	double scaleFactor = 1.0;
	CGDisplayModeRef modeRef = CGDisplayCopyDisplayMode(_display);
	if (modeRef != nullptr)
	{
		const size_t pointWidth = CGDisplayModeGetWidth(modeRef);
		if (pointWidth > 0)
		{
			scaleFactor = static_cast<double>(CGDisplayModeGetPixelWidth(modeRef)) / static_cast<double>(pointWidth);
		}
		CGDisplayModeRelease(modeRef);
	}

	const CGRect displayBounds = CGDisplayBounds(_display);
	return QSize(std::max(1, static_cast<int>(displayBounds.size.width * scaleFactor)),
				 std::max(1, static_cast<int>(displayBounds.size.height * scaleFactor)));
}

QSize OsxFrameGrabber::analysisSize() const
{
	const QSize capture = captureSize();
	const int decimation = std::max(1, _pixelDecimation);
	// Same calculation as ImageResampler::processImage (without cropping)
	return QSize(std::max(1, (capture.width() - (decimation >> 1) + decimation - 1) / decimation),
				 std::max(1, (capture.height() - (decimation >> 1) + decimation - 1) / decimation));
}

void OsxFrameGrabber::applyCaptureMode()
{
	const bool gpuScaling = useGpuScaling();
	const QSize streamSize = gpuScaling ? analysisSize() : captureSize();

	// In GPU scaling mode the capture session provides the analysis resolution already
	_imageResampler.setPixelDecimation(gpuScaling ? 1 : _pixelDecimation);

	if (_gpuScalingMode == gpuScaling && _streamSize == streamSize)
	{
		return;
	}

	_gpuScalingMode = gpuScaling;
	_streamSize = streamSize;

	if (isStreamActive())
	{
		// Re-create the capture session to apply the new capture configuration
		stopStream();
		startStream();
	}
}

bool OsxFrameGrabber::setPixelDecimation(int decimation)
{
	const bool changed = Grabber::setPixelDecimation(decimation);
	applyCaptureMode();
	return changed;
}

void OsxFrameGrabber::setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom)
{
	Grabber::setCropping(cropLeft, cropRight, cropTop, cropBottom);
	applyCaptureMode();
}

void OsxFrameGrabber::setVideoMode(VideoMode mode)
{
	Grabber::setVideoMode(mode);
	applyCaptureMode();
}

int OsxFrameGrabber::convertCGImage(CGImageRef imageRef, Image<ColorRgb> & image)
{
	int rc {-1};

	if (imageRef != nullptr)
	{
		CFDataRef imgData = CGDataProviderCopyData(CGImageGetDataProvider(imageRef));
		if (imgData != nullptr)
		{
			_imageResampler.processImage(static_cast<const uint8_t*>(CFDataGetBytePtr(imgData)),
										 static_cast<int>(CGImageGetWidth(imageRef)),
										 static_cast<int>(CGImageGetHeight(imageRef)),
										 static_cast<size_t>(CGImageGetBytesPerRow(imageRef)),
										 PixelFormat::BGR32, image);
			CFRelease(imgData);
			rc = 0;
		}
	}

	return rc;
}

int OsxFrameGrabber::grabSnapshot(Image<ColorRgb> & image)
{
#if defined(SDK_15_AVAILABLE)
	if (_contentFilter == nullptr)
	{
		return -1;
	}

	QSize streamSize;
	SCStreamConfiguration* config = createStreamConfiguration(_display, _fps, useGpuScaling(), _pixelDecimation, streamSize);

	__block CGImageRef snapshot = nil;
	dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
	[SCScreenshotManager captureImageWithFilter:static_cast<SCContentFilter*>(_contentFilter)
								  configuration:config
							  completionHandler:^(CGImageRef imageRef, NSError* error)
	{
		@autoreleasepool
		{
			if (error == nil && imageRef != nullptr)
			{
				snapshot = CGImageRetain(imageRef);
			}
			dispatch_semaphore_signal(semaphore);
		}
	}];
	dispatch_semaphore_wait(semaphore, dispatch_time(DISPATCH_TIME_NOW, SCK_CALL_TIMEOUT_MS * NSEC_PER_MSEC));
	dispatch_release(semaphore);
	[config release];

	if (snapshot == nullptr)
	{
		return -1;
	}

	const int rc = convertCGImage(snapshot, image);
	CGImageRelease(snapshot);

	return rc;
#else
	return -1;
#endif
}

int OsxFrameGrabber::grabFrame(Image<ColorRgb> & image, bool /*forceUpdate*/)
{
	if (_isDeviceInError)
    {
        Error(_log, "Cannot grab frame, device is in error state");
        return -1;
    }

    if (!_isEnabled)
    {
        return -1;
    }

	if (image.isNull())
	{
		// cannot grab into a null image
		return -1;
	}

#if defined(SDK_15_AVAILABLE)
	if (_streamOutput != nullptr)
	{
		OsxStreamOutput* streamOutput = static_cast<OsxStreamOutput*>(_streamOutput);

		if ([streamOutput hasStopped])
		{
			NSString* reason = [streamOutput stopReason];
			Error(_log, "Screen capture stream stopped: %s", reason != nil ? [reason UTF8String] : "unknown error");

			// Recreate the capture session on the next attempt, as the display might have
			// changed or got disconnected
			stopStream();
			_lastStreamAttemptMs = currentTimeMs();
		}
		else
		{
			CVPixelBufferRef frame = [streamOutput copyLatestFrame];
			if (frame != nullptr)
			{
				// The most recent frame is re-used until a new one is delivered. This keeps the
				// image update rate stable and prevents the screen capture source from being
				// flagged as inactive while the screen does not change
				int rc {-1};
				if (CVPixelBufferLockBaseAddress(frame, kCVPixelBufferLock_ReadOnly) == kCVReturnSuccess)
				{
					_imageResampler.processImage(static_cast<const uint8_t*>(CVPixelBufferGetBaseAddress(frame)),
												 static_cast<int>(CVPixelBufferGetWidth(frame)),
												 static_cast<int>(CVPixelBufferGetHeight(frame)),
												 static_cast<size_t>(CVPixelBufferGetBytesPerRow(frame)),
												 PixelFormat::BGR32, image);
					CVPixelBufferUnlockBaseAddress(frame, kCVPixelBufferLock_ReadOnly);
					rc = 0;
				}
				CVPixelBufferRelease(frame);

				if (rc == 0)
				{
					return 0;
				}
			}
			else if (_streamStartedMs > 0 && (currentTimeMs() - _streamStartedMs) > SCK_STREAM_START_GRACE_MS)
			{
				// The stream did not deliver a single frame since it was started.
				// Drop it and fall back to single snapshots (below)
				Warning(_log, "Screen capture stream did not deliver any frame, falling back to snapshots");
				stopStream();
				_lastStreamAttemptMs = currentTimeMs();
			}
			else
			{
				// No complete frame received (yet), nothing to convert
				return -1;
			}
		}
	}

	// No continuous capture session available, e.g. the stream could not be established yet.
	// Try to establish it (rate limited) and fall back to a single snapshot in the meantime.
	startStream();

	return grabSnapshot(image);
#else
	CGImageRef dispImage = CGDisplayCreateImageForRect(_display, CGDisplayBounds(_display));

	// display lost, use main
	if (dispImage == nullptr && _display != kCGDirectMainDisplay)
	{
		dispImage = CGDisplayCreateImageForRect(kCGDirectMainDisplay, CGDisplayBounds(kCGDirectMainDisplay));
	}

	// no displays connected, return
	if (dispImage == nullptr)
	{
		Error(_log, "No display connected...");
		return -1;
	}

	convertCGImage(dispImage, image);
	CGImageRelease(dispImage);
#endif

	return 0;
}

bool OsxFrameGrabber::setFramerate(int fps)
{
	if (Grabber::setFramerate(fps))
	{
		if (isStreamActive())
		{
			// Re-create the continuous capture session to apply the new frame interval
			stopStream();
			startStream();
		}
		return true;
	}

	return false;
}

bool OsxFrameGrabber::setDisplayIndex(int index)
{
	bool rc (true);
	if(_screenIndex != index || !_isEnabled)
	{
		_screenIndex = index;

		// get list of displays
		CGDisplayCount dspyCnt = 0 ;
		CGDisplayErr err;
		err = CGGetActiveDisplayList(0, nullptr, &dspyCnt);
		if (err == kCGErrorSuccess && dspyCnt > 0)
		{
			CGDirectDisplayID *activeDspys = new CGDirectDisplayID [dspyCnt] ;
			err = CGGetActiveDisplayList(dspyCnt, activeDspys, &dspyCnt) ;
			if (err == kCGErrorSuccess)
			{
				if (_screenIndex + 1 > static_cast<int>(dspyCnt))
				{
					Error(_log, "Display with index %d is not available.", _screenIndex);
					rc = false;
				}
				else
				{
					const CGDirectDisplayID display = activeDspys[_screenIndex];
					const bool displayChanged = (_display != display);
					const bool captureRunning = isStreamActive();
					_display = display;

					CGDisplayModeRef modeRef = CGDisplayCopyDisplayMode(_display);
					if (modeRef != nullptr)
					{
						Info(_log, "Display [%u] opened with resolution: %zux%zu", _display,
							 CGDisplayModeGetPixelWidth(modeRef), CGDisplayModeGetPixelHeight(modeRef));
						CGDisplayModeRelease(modeRef);
					}

					setEnabled(true);

					if (displayChanged && captureRunning)
					{
						// Re-create the capture session for the new display
						stopStream();
						startStream();
					}
				}
			}
		delete[] activeDspys;
		}
		else
		{
			rc=false;
		}
	}
	return rc;
}

QJsonObject OsxFrameGrabber::discover(const QJsonObject& params)
{
	QJsonObject inputsDiscovered;

	// get list of displays
	CGDisplayCount dspyCnt = 0 ;
	CGDisplayErr err;
	err = CGGetActiveDisplayList(0, nullptr, &dspyCnt);
	if (err == kCGErrorSuccess && dspyCnt > 0)
	{
		CGDirectDisplayID *activeDspys = new CGDirectDisplayID [dspyCnt] ;
		err = CGGetActiveDisplayList(dspyCnt, activeDspys, &dspyCnt) ;
		if (err == kCGErrorSuccess)
		{
			inputsDiscovered["device"] = "osx";
			inputsDiscovered["device_name"] = "OSX";
			inputsDiscovered["type"] = "screen";

			QJsonArray video_inputs;
			for (int i = 0; i < static_cast<int>(dspyCnt); ++i)
			{
				QJsonObject in;

				CGDirectDisplayID did = activeDspys[i];

				QString displayName;
				displayName = QString("Display:%1").arg(did);

				in["name"] = displayName;
				in["inputIdx"] = i;

				QJsonArray formats;
				QJsonObject format;

				QJsonArray resolutionArray;

				QJsonObject resolution;


				CGDisplayModeRef dispMode = CGDisplayCopyDisplayMode(did);
				CGRect rect = CGDisplayBounds(did);
				resolution["width"] = static_cast<int>(rect.size.width);
				resolution["height"] = static_cast<int>(rect.size.height);
				CGDisplayModeRelease(dispMode);

				resolution["fps"] = getFpsSupported();

				resolutionArray.append(resolution);

				format["resolutions"] = resolutionArray;
				formats.append(format);

				in["formats"] = formats;
				video_inputs.append(in);
			}
			inputsDiscovered["video_inputs"] = video_inputs;

			QJsonObject defaults, video_inputs_default, resolution_default;
			resolution_default["fps"] = _fps;
			video_inputs_default["resolution"] = resolution_default;
			video_inputs_default["inputIdx"] = 0;
			defaults["video_input"] = video_inputs_default;
			inputsDiscovered["default"] = defaults;
		}
		delete[] activeDspys;
	}

	if (inputsDiscovered.isEmpty())
	{
		qCDebug(grabber_screen_properties) << "No displays found to capture from!";
	}

	return inputsDiscovered;
}
