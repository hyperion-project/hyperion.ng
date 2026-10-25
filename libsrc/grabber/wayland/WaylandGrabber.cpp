#include <grabber/wayland/WaylandGrabber.h>

#include "PipeWireFrameSource.h"
#include "PortalScreenCast.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QSize>
#include <QStandardPaths>

#include <grabber/GrabberTrace.h>

namespace {
// A failed portal session (e.g. portal not ready at login, sharing stopped from the desktop) is
// retried with a doubling delay between these bounds.
constexpr int RETRY_MIN_MS = 5000;
// An ended stream usually means an output went away: resume quickly, the portal is up.
constexpr int RETRY_AFTER_END_MS = 1000;

// The portal decides what is captured, so the real size is only known with the first frame. The UI
// needs a resolution to offer capture frequencies and to limit the crop fields: use the largest
// preferred mode among the connected outputs.
QSize nominalScreenSize()
{
	QSize best(1920, 1080);
	qint64 bestArea = 0;
	const QDir drm(QStringLiteral("/sys/class/drm"));
	const QStringList connectors = drm.entryList(QStringList{QStringLiteral("card*-*")}, QDir::Dirs | QDir::NoDotAndDotDot);
	for (const QString& connector : connectors)
	{
		QFile status(drm.filePath(connector + QStringLiteral("/status")));
		QFile modes(drm.filePath(connector + QStringLiteral("/modes")));
		if (!status.open(QIODevice::ReadOnly) || status.readAll().trimmed() != "connected" || !modes.open(QIODevice::ReadOnly))
		{
			continue;
		}

		// the first line is the preferred mode, e.g. "2160x1440"
		const QList<QByteArray> parts = modes.readLine().trimmed().split('x');
		const QSize size(parts.value(0).toInt(), parts.value(1).toInt());
		const qint64 area = static_cast<qint64>(size.width()) * size.height();
		if (area > bestArea)
		{
			bestArea = area;
			best = size;
		}
	}
	return best;
}
constexpr int RETRY_MAX_MS = 5 * 60 * 1000;
}

WaylandGrabber::WaylandGrabber(int cropLeft, int cropRight, int cropTop, int cropBottom)
	: Grabber("GRABBER-WAYLAND", cropLeft, cropRight, cropTop, cropBottom)
{
	TRACK_SCOPE() << "Creating Wayland grabber";
	// Real size is known with the first frame; keep it non-zero until then.
	_width = 64;
	_height = 36;

	QJsonArray fpsSupported = {{ 1, 5, 10, 15, 20, 25, 30, 40, 50, 60, 75, 90, 120, 144, 165, 240 }};
	setFpsSupported(fpsSupported);
}

WaylandGrabber::~WaylandGrabber()
{
	TRACK_SCOPE();
}

bool WaylandGrabber::isAvailable(bool logError)
{
	_isAvailable = false;

	qCDebug(grabber_screen_flow) << "Checking Wayland grabber availability" << "WAYLAND_DISPLAY:" << qgetenv("WAYLAND_DISPLAY") << "XDG_SESSION_TYPE:" << qgetenv("XDG_SESSION_TYPE") << "DBUS_SESSION_BUS_ADDRESS:" << qgetenv("DBUS_SESSION_BUS_ADDRESS");

	// The dialog is drawn by the portal, so no GUI application is needed: this also runs as a service.
	if (qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY") && qgetenv("XDG_SESSION_TYPE") != "wayland")
	{
		ErrorIf(logError, _log, "Wayland grabber requires a Wayland session (neither WAYLAND_DISPLAY nor XDG_SESSION_TYPE=wayland is set)");
	}
	else if (qEnvironmentVariableIsEmpty("DBUS_SESSION_BUS_ADDRESS"))
	{
		ErrorIf(logError, _log, "Wayland grabber requires the D-Bus session bus (DBUS_SESSION_BUS_ADDRESS is not set)");
	}
	else
	{
		_isAvailable = true;
	}

	return _isAvailable;
}

bool WaylandGrabber::open()
{
	qCDebug(grabber_screen_flow) << "Opening Wayland grabber";
	if (_portal != nullptr)
	{
		qCDebug(grabber_screen_flow) << "Wayland grabber already opened";
		return true;
	}

	if (!isAvailable(true))
	{
		return false;
	}

	_portal = std::make_unique<PortalScreenCast>(stateFile(QStringLiteral("wayland-restore-token")));
	_source = std::make_unique<PipeWireFrameSource>();
	_source->setMinFrameInterval(frameIntervalMs());
	connect(_portal.get(), &PortalScreenCast::started, this, &WaylandGrabber::onPortalStarted);
	connect(_portal.get(), &PortalScreenCast::failed, this, &WaylandGrabber::onPortalFailed);

	Info(_log, "Requesting screen capture from the desktop portal (a dialog appears if no stored choice is valid)");
	_portal->start();
	return true;
}

// Ends the portal session and the stream.
void WaylandGrabber::releaseSession()
{
	_source.reset();
	_portal.reset();
	_sourceWidth = 0;
	_sourceHeight = 0;
}

bool WaylandGrabber::close()
{
	qCDebug(grabber_screen_flow) << "Closing Wayland grabber";
	releaseSession();
	return true;
}

void WaylandGrabber::onPortalStarted(int pipewireFd, quint32 nodeId)
{
	qCDebug(grabber_screen_flow) << "Portal started with pipewireFd:" << pipewireFd << "nodeId:" << nodeId;
	QString error;
	if (!_source->start(pipewireFd, nodeId, error))
	{
		Error(_log, "%s", QSTRING_CSTR(error));
		teardownAndScheduleRetry(false);
		return;
	}
	Info(_log, "Screen capture stream started (PipeWire node %u)", nodeId);
}

void WaylandGrabber::onPortalFailed(const QString& reason, bool userCancelled)
{
	if (userCancelled)
	{
		Warning(_log, "%s: screen capture stays off until the grabber is enabled again", QSTRING_CSTR(reason));
	}
	else
	{
		Error(_log, "Screen capture request failed: %s", QSTRING_CSTR(reason));
	}
	teardownAndScheduleRetry(userCancelled);
}

// Drops the failed session so grabFrame() opens a new one after the backoff. Called from the
// portal's own signal, hence the deferred delete.
void WaylandGrabber::teardownAndScheduleRetry(bool userCancelled)
{
	if (_portal != nullptr)
	{
		_portal.release()->deleteLater();
	}
	_source.reset();
	_sourceWidth = 0;
	_sourceHeight = 0;

	_userCancelled = userCancelled;
	_retryDelayMs = _retryDelayMs == 0 ? RETRY_MIN_MS : qMin(_retryDelayMs * 2, RETRY_MAX_MS);
	_retryTimer.start();
}

QString WaylandGrabber::stateFile(const QString& name)
{
	return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1Char('/') + name;
}

int WaylandGrabber::frameIntervalMs() const
{
	return qMax(1, 1000 / qMax(1, _fps));
}

bool WaylandGrabber::setFramerate(int fps)
{
	const bool changed = Grabber::setFramerate(fps);
	if (_source != nullptr)
	{
		_source->setMinFrameInterval(frameIntervalMs());
	}
	return changed;
}

void WaylandGrabber::setEnabled(bool enable)
{
	if (enable)
	{
		_userCancelled = false;
		_retryDelayMs = 0;
		_retryTimer.invalidate();
	}
	Grabber::setEnabled(enable);
}

void WaylandGrabber::setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom)
{
	// Grabber::setCropping compares against _width/_height, which here are already decimated.
	if (_sourceWidth > 0 && (cropLeft + cropRight >= _sourceWidth || cropTop + cropBottom >= _sourceHeight))
	{
		Error(_log, "Rejecting invalid crop values: left: %d, right: %d, top: %d, bottom: %d, frame is %dx%d",
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

void WaylandGrabber::updateImageSize(int frameWidth, int frameHeight)
{
	// Same output size ImageResampler::processImage computes, so the wrapper pre-sizes correctly.
	_width = qMax(1, (frameWidth - _cropLeft - _cropRight) / _pixelDecimation);
	_height = qMax(1, (frameHeight - _cropTop - _cropBottom) / _pixelDecimation);
}

int WaylandGrabber::grabFrame(Image<ColorRgb>& image, bool /*forceUpdate*/)
{
	if (_isDeviceInError || !_isEnabled)
	{
		return -1;
	}

	if (_userCancelled)
	{
		qCDebug(grabber_screen_flow) << "User cancelled, not grabbing frame";
		return -1;
	}

	if (_source != nullptr && _source->hasFailed())
	{
		Warning(_log, "Screen capture stream ended, restarting");
		releaseSession();
		_retryDelayMs = RETRY_AFTER_END_MS;
		_retryTimer.start();
		qCDebug(grabber_screen_flow) << "Retrying after end delay" << _retryDelayMs << "ms";
	}

	if (_portal == nullptr)
	{
		if (_retryTimer.isValid() && !_retryTimer.hasExpired(_retryDelayMs))
		{
			qCDebug(grabber_screen_flow) << "Retrying after delay" << _retryDelayMs << "ms";
			return -1;
		}
		if (!open())
		{
			qCDebug(grabber_screen_flow) << "Failed to open portal, will retry";
			return -1;
		}
	}

	CapturedFrame frame;
	if (_source == nullptr || !_source->latest(frame))
	{
		// waiting for the portal (or the user, on the first run)
		qCDebug(grabber_screen_flow) << "Waiting for the portal or user action";
		return -1;
	}

	if (frame.width != _sourceWidth || frame.height != _sourceHeight)
	{
		Info(_log, "Receiving screen frames [%dx%d]", frame.width, frame.height);
		_retryDelayMs = 0;
		_sourceWidth = frame.width;
		_sourceHeight = frame.height;
		updateImageSize(_sourceWidth, _sourceHeight);
	}

	_imageResampler.processImage(reinterpret_cast<const uint8_t*>(frame.data.constData()),
	                             frame.width, frame.height,
	                             frame.stride, frame.pixelFormat, image);

	qCDebug(grabber_screen_capture) << "Frame grabbed into image with size:" << image.width() << "x" << image.height();								 
	return 0;
}

QJsonObject WaylandGrabber::discover(const QJsonObject& /*params*/)
{
	QJsonObject inputsDiscovered;
	if (!isAvailable(false))
	{
		return inputsDiscovered;
	}

	inputsDiscovered["device"] = "wayland";
	inputsDiscovered["device_name"] = "Wayland";
	inputsDiscovered["type"] = "screen";

	// The screen is chosen in the portal dialog, so there is a single logical input.
	const QSize size = nominalScreenSize();

	QJsonObject resolution;
	resolution["width"] = size.width();
	resolution["height"] = size.height();
	resolution["fps"] = getFpsSupported();

	QJsonObject format;
	format["resolutions"] = QJsonArray{resolution};

	QJsonObject in;
	in["name"] = "Portal selected screen";
	in["inputIdx"] = 0;
	in["formats"] = QJsonArray{format};

	QJsonArray inputs;
	inputs.append(in);
	inputsDiscovered["video_inputs"] = inputs;

	QJsonObject resolution_default;
	resolution_default["fps"] = _fps;
	QJsonObject video_input_default;
	video_input_default["inputIdx"] = 0;
	video_input_default["resolution"] = resolution_default;
	QJsonObject defaults;
	defaults["video_input"] = video_input_default;
	inputsDiscovered["default"] = defaults;

	return inputsDiscovered;
}
