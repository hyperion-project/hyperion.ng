#pragma once

#include <QObject>
#include <QRectF>

#include <grabber/video/v4l2/V4L2Grabber.h>
#include <utils/ColorRgb.h>
#include <utils/Image.h>
#include <utils/VideoMode.h>

#include "V4L2GrabberOptions.h"

///
/// Thin adapter that exposes the common screenInit()/setVideoMode()/getScreenshot()/
/// start()/stop()/sig_screenshot() interface shared by all other standalone grabbers
/// (see GrabberRunner.h) on top of V4L2Grabber, which natively exposes a
/// prepare()/start()/stop()/newFrame() interface plus a large set of device-specific
/// setters that are applied once at start-up.
///
/// This allows hyperion-v4l2 to reuse the same runFlatbufferScreenGrabber() run-loop
/// as the other grabbers for its "stream to a Hyperion server" mode. Taking a single
/// --screenshot is implemented using a nested event loop (see getScreenshot()) since,
/// unlike the other grabbers, V4L2Grabber only ever delivers frames asynchronously.
///
class V4L2Wrapper : public QObject
{
	Q_OBJECT

public:
	explicit V4L2Wrapper(const V4L2GrabberOptions& opts);

	bool screenInit();
	void setVideoMode(VideoMode mode);
	const Image<ColorRgb>& getScreenshot();
	void start();
	void stop();

signals:
	void sig_screenshot(const Image<ColorRgb>& image);

	// Emitted when the device fails to start, or starts but delivers no frame within
	// the bounded timeout (see getScreenshot()/start()). Callers are expected to wire
	// this to ErrorManager::errorOccurred.
	void sig_error(const QString& message);

private:
	V4L2Grabber _grabber;
	Image<ColorRgb> _screenshot;

	// Set once a matching device (path + enumerated name) was resolved in the
	// constructor; see the comment there for why this is required.
	bool _deviceResolved = false;

	// Upper bound for how long getScreenshot() waits for a single frame once the
	// device has started, so a device that opens but never delivers a frame cannot
	// hang the process indefinitely.
	static constexpr int NO_FRAME_TIMEOUT_MS = 5000;
};
