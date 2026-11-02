#include <grabber/pipewire/GamescopeGrabber.h>

#include "PipeWireFrameSource.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QSize>

#include <grabber/GrabberTrace.h>

namespace
{
	// Gamescope is short-lived (launched per game). Retry quickly after a stream
	// end so captures resume the moment the next game starts.
	constexpr int RETRY_AFTER_END_MS = 2000;
	// Minimum and maximum delays for reconnect attempts when the node is absent.
	constexpr int RETRY_MIN_MS = 5000;
	constexpr int RETRY_MAX_MS = 30000;

	// PipeWire node.name Gamescope registers under.
	constexpr const char* GAMESCOPE_NODE_NAME = "gamescope";
}

GamescopeGrabber::GamescopeGrabber(int cropLeft, int cropRight, int cropTop, int cropBottom)
    : Grabber("GRABBER-GAMESCOPE", cropLeft, cropRight, cropTop, cropBottom)
{
	TRACK_SCOPE() << "Creating Gamescope grabber";
	// Use a placeholder until the real frame size is known.
	_width = 64;
	_height = 36;

	const QJsonArray fpsSupported = {{1, 5, 10, 15, 20, 25, 30, 40, 50, 60, 75, 90, 120, 144, 165, 240}};
	setFpsSupported(fpsSupported);
}

GamescopeGrabber::~GamescopeGrabber()
{
	TRACK_SCOPE();
	close();
}

bool GamescopeGrabber::isAvailable(bool logError)
{
	_isAvailable = false;

	qCDebug(grabber_screen_flow) << "Checking Gamescope grabber availability"
	                             << "XDG_RUNTIME_DIR:" << qgetenv("XDG_RUNTIME_DIR")
	                             << "WAYLAND_DISPLAY:" << qgetenv("WAYLAND_DISPLAY");

	// We need a reachable PipeWire socket in XDG_RUNTIME_DIR.
	const QByteArray runtimeDir = qgetenv("XDG_RUNTIME_DIR");
	if (runtimeDir.isEmpty())
	{
		ErrorIf(logError, _log, "Gamescope grabber requires XDG_RUNTIME_DIR to be set");
		return false;
	}

	const QString pwSocket = QString::fromUtf8(runtimeDir) + QStringLiteral("/pipewire-0");
	if (!QFile::exists(pwSocket))
	{
		ErrorIf(logError, _log,
		        "Gamescope grabber requires a running PipeWire daemon (%s not found)",
		        QSTRING_CSTR(pwSocket));
		return false;
	}

	_isAvailable = true;
	return true;
}

bool GamescopeGrabber::setInput(int nodeId)
{
	if (_configuredNodeId == nodeId)
	{
		return true;
	}

	_configuredNodeId = nodeId;

	// Drop the existing stream so the next grabFrame() reconnects to the new node.
	if (_source != nullptr)
	{
		Info(_log, "Gamescope node changed to %d, reconnecting", nodeId);
		close();
		_retryDelayMs = 0;
		_retryTimer.invalidate();
	}
	return true;
}

bool GamescopeGrabber::open()
{
	qCDebug(grabber_screen_flow) << "Opening Gamescope grabber";
	if (_source != nullptr)
	{
		qCDebug(grabber_screen_flow) << "Gamescope grabber already opened";
		return true;
	}

	if (!isAvailable(true))
	{
		return false;
	}

	// Discover all active Gamescope video nodes in PipeWire.
	const QList<GamescopeNodeInfo> nodes = PipeWireFrameSource::discoverGamescopeNodes();

	uint32_t nodeId = 0;
	QString nodeName;

	if (!nodes.isEmpty())
	{
		// Try the node the user selected.
		if (_configuredNodeId > 0)
		{
			for (const GamescopeNodeInfo& node : nodes)
			{
				if (static_cast<int>(node.id) == _configuredNodeId)
				{
					nodeId = node.id;
					nodeName = node.displayName;
					break;
				}
			}
			if (nodeId == 0)
			{
				Warning(_log, "Configured Gamescope node %d not found, falling back to first discovered node",
				        _configuredNodeId);
			}
		}

		// Fall back to the first discovered node.
		if (nodeId == 0)
		{
			Info(_log, "Using the first discovered Gamescope node");
			nodeId = nodes.first().id;
			nodeName = nodes.first().displayName;
		}
	}

	_source = std::make_unique<PipeWireFrameSource>();
	_source->setMinFrameInterval(frameIntervalMs());

	QString error;
	bool ok = false;

	if (nodeId > 0)
	{
		qCDebug(grabber_screen_flow) << "Connecting to Gamescope node by ID: " << nodeId;
		// Direct connection by numeric node ID — no session manager required.
		ok = _source->startDirect(nodeId, nodeName, error);
	}
	else
	{
		// No nodes discovered yet: fall back to name-based auto-connect so
		// the stream will be linked by WirePlumber once Gamescope appears.
		nodeName = GAMESCOPE_NODE_NAME;
		qCDebug(grabber_screen_flow) << "Connecting to Gamescope node by name: " << nodeName;
		ok = _source->startDirect(nodeName, error);
		if (ok)
		{
			Info(_log, "No Gamescope node found yet, connecting by name for auto-link");
		}
	}

	if (!ok)
	{
		Error(_log, "Failed to connect to Gamescope PipeWire node: %s", QSTRING_CSTR(error));
		_source.reset();
		return false;
	}

	if (nodeId > 0)
	{
		Info(_log, "Connecting to Gamescope node \"%s\" (PipeWire ID %u)", QSTRING_CSTR(nodeName), nodeId);
	}
	return true;
}

bool GamescopeGrabber::close()
{
	qCDebug(grabber_screen_flow) << "Closing Gamescope grabber";
	_source.reset();
	_sourceWidth = 0;
	_sourceHeight = 0;
	return true;
}

void GamescopeGrabber::scheduleRetry(int delayMs)
{
	_source.reset();
	_sourceWidth = 0;
	_sourceHeight = 0;
	_retryDelayMs = delayMs;
	_retryTimer.start();
}

int GamescopeGrabber::frameIntervalMs() const
{
	return qMax(1, 1000 / qMax(1, _fps));
}

bool GamescopeGrabber::setFramerate(int fps)
{
	const bool changed = Grabber::setFramerate(fps);
	if (_source != nullptr)
	{
		_source->setMinFrameInterval(frameIntervalMs());
	}
	return changed;
}

void GamescopeGrabber::setEnabled(bool enable)
{
	if (enable)
	{
		// Clear any back-off so the next grabFrame() reconnects immediately.
		_retryDelayMs = 0;
		_retryTimer.invalidate();
	}
	Grabber::setEnabled(enable);
}

void GamescopeGrabber::setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom)
{
	if (_sourceWidth > 0 && (cropLeft + cropRight >= _sourceWidth || cropTop + cropBottom >= _sourceHeight))
	{
		Error(_log, "Rejecting invalid crop values: left:%d right:%d top:%d bottom:%d, frame is %dx%d",
		      cropLeft, cropRight, cropTop, cropBottom, _sourceWidth, _sourceHeight);
		return;
	}

	_cropLeft = cropLeft;
	_cropRight = cropRight;
	_cropTop = cropTop;
	_cropBottom = cropBottom;
	_imageResampler.setCropping(cropLeft, cropRight, cropTop, cropBottom);

	if (_sourceWidth > 0)
	{
		updateImageSize(_sourceWidth, _sourceHeight);
	}
}

void GamescopeGrabber::updateImageSize(int frameWidth, int frameHeight)
{
	_width = qMax(1, (frameWidth - _cropLeft - _cropRight) / _pixelDecimation);
	_height = qMax(1, (frameHeight - _cropTop - _cropBottom) / _pixelDecimation);
}

int GamescopeGrabber::grabFrame(Image<ColorRgb>& image, bool /*forceUpdate*/)
{
	if (_isDeviceInError || !_isEnabled)
	{
		return -1;
	}

	// --- Stream failure: schedule a quick retry ---
	if (_source != nullptr && _source->hasFailed())
	{
		Warning(_log, "Gamescope PipeWire stream ended, will retry");
		scheduleRetry(RETRY_AFTER_END_MS);
	}

	// --- Not connected yet: try (re-)opening after the back-off ---
	if (_source == nullptr)
	{
		if (_retryTimer.isValid() && !_retryTimer.hasExpired(_retryDelayMs))
		{
			qCDebug(grabber_screen_flow) << "Retry back-off, remaining:"
			                             << (_retryDelayMs - _retryTimer.elapsed()) << "ms";
			return -1;
		}

		if (!open())
		{
			// open() failed: apply exponential back-off and try again later.
			const int nextDelay = _retryDelayMs == 0
			                          ? RETRY_MIN_MS
			                          : qMin(_retryDelayMs * 2, RETRY_MAX_MS);
			scheduleRetry(nextDelay);
			qCDebug(grabber_screen_flow) << "Gamescope not reachable, next retry in" << nextDelay << "ms";
			return -1;
		}
	}

	CapturedFrame frame;
	if (!_source->latest(frame))
	{
		// Waiting for the first frame from Gamescope (may not be running yet).
		qCDebug(grabber_screen_flow) << "Waiting for Gamescope frame";
		return -1;
	}

	if (frame.width <= 0 || frame.height <= 0)
	{
		Warning(_log, "Rejecting invalid Gamescope frame dimensions [%dx%d]", frame.width, frame.height);
		return -1;
	}

	if (frame.width != _sourceWidth || frame.height != _sourceHeight)
	{
		if (static_cast<qint64>(_cropLeft) + _cropRight >= frame.width ||
		    static_cast<qint64>(_cropTop) + _cropBottom >= frame.height)
		{
			Warning(_log, "Crop values exceed Gamescope frame dimensions [%dx%d]; resetting crop",
			        frame.width, frame.height);
			_cropLeft = 0;
			_cropRight = 0;
			_cropTop = 0;
			_cropBottom = 0;
			_imageResampler.setCropping(0, 0, 0, 0);
		}

		Info(_log, "Receiving Gamescope frames [%dx%d]", frame.width, frame.height);
		_retryDelayMs = 0;
		_sourceWidth = frame.width;
		_sourceHeight = frame.height;
		updateImageSize(_sourceWidth, _sourceHeight);
	}

	_imageResampler.processImage(reinterpret_cast<const uint8_t*>(frame.data.constData()),
	                             frame.width, frame.height,
	                             frame.stride, frame.pixelFormat, image);

	qCDebug(grabber_screen_capture) << "Gamescope frame grabbed:" << image.width() << "x" << image.height();
	return 0;
}

QJsonObject GamescopeGrabber::discover(const QJsonObject& /*params*/)
{
	QJsonObject inputsDiscovered;
	if (!isAvailable(false))
	{
		qCDebug(grabber_screen_properties) << "Gamescope is not available!";
		return inputsDiscovered;
	}

	const QList<GamescopeNodeInfo> nodes = PipeWireFrameSource::discoverGamescopeNodes();

	if (nodes.isEmpty())
	{
		qCDebug(grabber_screen_properties) << "No displays found to capture from!";
	}

	inputsDiscovered["device"] = "gamescope";
	inputsDiscovered["device_name"] = "Gamescope";
	inputsDiscovered["type"] = "screen";

	QJsonArray inputs;
	for (const GamescopeNodeInfo& node : nodes)
	{
		QJsonObject resolution;
		resolution["width"] = _sourceWidth > 0 ? _sourceWidth : 1920;
		resolution["height"] = _sourceHeight > 0 ? _sourceHeight : 1080;
		resolution["fps"] = getFpsSupported();

		QJsonObject format;
		format["resolutions"] = QJsonArray{resolution};

		QJsonObject in;
		in["name"] = node.displayName + " [Node:" + QString::number(node.id) + "]";
		in["inputIdx"] = static_cast<int>(node.id); // PipeWire node ID stored as inputIdx
		in["formats"] = QJsonArray{format};
		inputs.append(in);
	}

	// Always include an "auto" entry so the device is selectable even when
	// Gamescope is not currently running (it will connect once it starts).
	{
		QJsonObject resolution;
		resolution["width"] = _sourceWidth > 0 ? _sourceWidth : 1920;
		resolution["height"] = _sourceHeight > 0 ? _sourceHeight : 1080;
		resolution["fps"] = getFpsSupported();

		QJsonObject format;
		format["resolutions"] = QJsonArray{resolution};

		QJsonObject in;
		in["name"] = inputs.isEmpty() ? QStringLiteral("Gamescope (auto)") : QStringLiteral("Auto (first found)");
		in["inputIdx"] = 0;
		in["formats"] = QJsonArray{format};
		inputs.prepend(in); // put "auto" at the top
	}

	inputsDiscovered["video_inputs"] = inputs;

	QJsonObject resolution_default;
	resolution_default["fps"] = _fps;

	qCDebug(grabber_screen_properties) << "Default inputIdx: " << _configuredNodeId;

	QJsonObject video_input_default;
	video_input_default["inputIdx"] = 0;
	video_input_default["resolution"] = resolution_default;

	QJsonObject defaults;
	defaults["video_input"] = video_input_default;
	inputsDiscovered["default"] = defaults;

	return inputsDiscovered;
}
