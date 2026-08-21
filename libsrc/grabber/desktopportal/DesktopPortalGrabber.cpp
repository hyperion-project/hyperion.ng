#include <grabber/desktopportal/DesktopPortalGrabber.h>

#include <QCoreApplication>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusUnixFileDescriptor>
#include <QDBusVariant>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QObject>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <mutex>
#include <sys/mman.h>
#include <unistd.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include <spa/utils/dict.h>
#pragma GCC diagnostic pop

namespace {
	constexpr auto RETRY_INTERVAL = std::chrono::milliseconds(2000);

	// See onStreamProcess()'s comment on the CPU cost of reading DMA-BUF/GPU memory from the
	// CPU: only every Nth row/column is actually read out of the (slow) source buffer. Shared
	// at file scope so onStreamParamChanged() can report the resulting (smaller) dimensions as
	// this grabber's actual image size, consistently with what onStreamProcess() produces -
	// reporting the true negotiated (native) size here while capturing at a reduced size would
	// make transferFrame() resize its destination Image to the native resolution and force
	// ImageResampler to upscale from the smaller captured buffer on every single frame, which
	// costs about as much as the DMA-BUF read this is meant to avoid.
	constexpr int READ_STRIDE_FACTOR = 4;

	const QString PORTAL_SERVICE = QStringLiteral("org.freedesktop.portal.Desktop");
	const QString PORTAL_PATH = QStringLiteral("/org/freedesktop/portal/desktop");
	const QString SCREENCAST_IFACE = QStringLiteral("org.freedesktop.portal.ScreenCast");
	const QString SESSION_IFACE = QStringLiteral("org.freedesktop.portal.Session");
	const QString REQUEST_IFACE = QStringLiteral("org.freedesktop.portal.Request");

	QString newHandleToken()
	{
		static std::atomic<int> counter{0};
		return QStringLiteral("hyperion_dp_%1_%2")
			.arg(QCoreApplication::applicationPid())
			.arg(counter.fetch_add(1));
	}

	// Start()'s "streams" result is "a(ua{sv})" - an array of (node id, properties) tuples.
	// Manually navigating this with QDBusArgument::beginStructure()/operator>> (the pattern
	// used for e.g. a plain a{sv}) reliably crashed here: beginStructure() didn't actually
	// advance the demarshaller's cursor into the struct's fields (confirmed via
	// currentType()/currentSignature() - both stayed at the outer struct's own signature), so
	// the following "arg >> nodeId" read hit libdbus's raw iterator still positioned on the
	// struct container itself, aborting with "type struct not a basic type". Registering a
	// proper custom type via Q_DECLARE_METATYPE + qDBusRegisterMetaType and reading through
	// qdbus_cast (Qt's documented pattern for arrays of structs) sidesteps this entirely - it
	// lets QDBusArgument's own registered-type machinery walk the struct instead of manual
	// begin/end calls at the call site.
	struct PortalStream
	{
		uint32_t nodeId{0};
		QVariantMap properties;
	};
}

Q_DECLARE_METATYPE(PortalStream)

namespace {

	QDBusArgument& operator<<(QDBusArgument& argument, const PortalStream& stream)
	{
		argument.beginStructure();
		argument << stream.nodeId << stream.properties;
		argument.endStructure();
		return argument;
	}

	const QDBusArgument& operator>>(const QDBusArgument& argument, PortalStream& stream)
	{
		argument.beginStructure();
		argument >> stream.nodeId >> stream.properties;
		argument.endStructure();
		return argument;
	}

	void ensurePortalStreamTypeRegistered()
	{
		static std::once_flag once;
		std::call_once(once, [] {
			qDBusRegisterMetaType<PortalStream>();
			qDBusRegisterMetaType<QList<PortalStream>>();
		});
	}

	// The first (and, since we always request multiple=false, only) stream's node id out of
	// Start()'s "streams" response entry.
	uint32_t extractFirstStreamNodeId(const QVariant& streamsVariant)
	{
		ensurePortalStreamTypeRegistered();

		const QList<PortalStream> streams = qdbus_cast<QList<PortalStream>>(streamsVariant);
		return streams.isEmpty() ? 0 : streams.first().nodeId;
	}
}

///
/// Private QObject helper: subscribes to a single org.freedesktop.portal.Request's Response
/// signal and quits its own QEventLoop once it fires. Defined here (not in the header) and
/// picked up by AUTOMOC via the #include "DesktopPortalGrabber.moc" at the bottom of this file -
/// the standard Qt idiom for a QObject that's purely a .cpp implementation detail.
///
class DesktopPortalRequestWatcher : public QObject
{
	Q_OBJECT
public:
	QEventLoop loop;
	uint32_t response{2};
	QVariantMap results;

public slots:
	void onResponse(uint response_, const QVariantMap& results_)
	{
		response = response_;
		results = results_;
		loop.quit();
	}
};

DesktopPortalGrabber::DesktopPortalGrabber(int cropLeft, int cropRight, int cropTop, int cropBottom)
	: Grabber("DESKTOP-PORTAL", cropLeft, cropRight, cropTop, cropBottom)
{
	// Always available: the actual portal session is negotiated lazily and can come and go
	// (permission revoked, portal restarted, etc.) - see GamescopeGrabber for why this grabber
	// shape reports availability unconditionally rather than deciding it once at construction.
	_isAvailable = true;

	_thread.emplace(&DesktopPortalGrabber::portalThreadMain, this);
}

DesktopPortalGrabber::~DesktopPortalGrabber()
{
	stop();
}

void DesktopPortalGrabber::stop()
{
	_stopping = true;
	_retryCv.notify_all();

	if (_loop)
	{
		pw_main_loop_quit(_loop);
	}

	if (_thread.has_value())
	{
		_thread->join();
		_thread.reset();
	}
}

int DesktopPortalGrabber::grabFrame(Image<ColorRgb>& image, bool /*forceUpdate*/)
{
	if (!_connected.load())
	{
		return -1;
	}

	std::lock_guard<std::mutex> lock(_bufferMutex);
	if (!_frontBuffer.valid)
	{
		return -1;
	}

	_imageResampler.processImage(_frontBuffer.data.data(), _frontBuffer.width, _frontBuffer.height,
		_frontBuffer.stride, _frontBuffer.pixelFormat, image);

	return 0;
}

QJsonObject DesktopPortalGrabber::discover(const QJsonObject& /*params*/)
{
	QJsonObject inputsDiscovered;
	inputsDiscovered["device"] = "desktop-portal";
	inputsDiscovered["device_name"] = "Desktop Portal";
	inputsDiscovered["type"] = "screen";

	// Same rationale as GamescopeGrabber::discover(): this exists purely so the grabber shows
	// up as a selectable entry in the "Device discovered" UI, not to enumerate multiple
	// screens - the portal's own picker UI is where the user actually chooses a monitor.
	QSize size = getScreenSize();
	if (size.width() == 0 || size.height() == 0)
	{
		size = QSize(1920, 1080);
	}

	QJsonObject resolution;
	resolution["width"] = size.width();
	resolution["height"] = size.height();
	resolution["fps"] = getFpsSupported();

	QJsonArray resolutionArray;
	resolutionArray.append(resolution);

	QJsonObject format;
	format["resolutions"] = resolutionArray;

	QJsonArray formats;
	formats.append(format);

	QJsonObject input;
	input["name"] = "Desktop Portal";
	input["inputIdx"] = 0;
	input["formats"] = formats;

	QJsonArray videoInputs;
	videoInputs.append(input);
	inputsDiscovered["video_inputs"] = videoInputs;

	QJsonObject resolutionDefault;
	resolutionDefault["fps"] = _fps;

	QJsonObject videoInputDefault;
	videoInputDefault["resolution"] = resolutionDefault;
	videoInputDefault["inputIdx"] = 0;

	QJsonObject defaults;
	defaults["video_input"] = videoInputDefault;
	inputsDiscovered["default"] = defaults;

	return inputsDiscovered;
}

QString DesktopPortalGrabber::restoreTokenPath() const
{
	return QDir::homePath() + QStringLiteral("/.hyperion/desktop-portal-restore-token");
}

QString DesktopPortalGrabber::readRestoreToken() const
{
	QFile file(restoreTokenPath());
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		return {};
	}

	return QString::fromUtf8(file.readAll()).trimmed();
}

void DesktopPortalGrabber::writeRestoreToken(const QString& token) const
{
	QFile file(restoreTokenPath());
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
	{
		Warning(_log, "[desktop-portal] Failed to save restore token to %s", QSTRING_CSTR(restoreTokenPath()));
		return;
	}

	file.write(token.toUtf8());
}

DesktopPortalGrabber::PortalResponse DesktopPortalGrabber::callPortalRequest(const QString& method, const QVariantList& args, QVariantMap options)
{
	QDBusConnection bus = QDBusConnection::sessionBus();

	const QString handleToken = newHandleToken();
	options["handle_token"] = handleToken;

	// Predict the Request object path from our own unique bus name + handle_token (per the
	// xdg-desktop-portal spec) and subscribe to its Response signal *before* issuing the call,
	// to avoid a race against the signal firing before we're listening. This assumes the
	// portal implementation returns the predicted path (true for xdg-desktop-portal-kde,
	// version 4) rather than a different one - spec-compliant but not reconciled if it ever
	// isn't, to keep this simple for a single-target build.
	QString sender = bus.baseService();
	sender.remove(0, 1); // strip leading ':'
	sender.replace('.', '_');
	const QString requestPath = QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2").arg(sender, handleToken);

	DesktopPortalRequestWatcher watcher;
	const bool connected = bus.connect(PORTAL_SERVICE, requestPath, REQUEST_IFACE, QStringLiteral("Response"),
		&watcher, SLOT(onResponse(uint,QVariantMap)));
	Debug(_log, "[desktop-portal] %s: predicted request path %s, subscribe ok=%d",
		QSTRING_CSTR(method), QSTRING_CSTR(requestPath), connected ? 1 : 0);

	QDBusMessage msg = QDBusMessage::createMethodCall(PORTAL_SERVICE, PORTAL_PATH, SCREENCAST_IFACE, method);
	QVariantList fullArgs = args;
	fullArgs << QVariant(options);
	msg.setArguments(fullArgs);

	QDBusMessage reply = bus.call(msg);
	if (reply.type() == QDBusMessage::ErrorMessage)
	{
		Warning(_log, "[desktop-portal] %s call failed: %s", QSTRING_CSTR(method), QSTRING_CSTR(reply.errorMessage()));
		bus.disconnect(PORTAL_SERVICE, requestPath, REQUEST_IFACE, QStringLiteral("Response"), &watcher, SLOT(onResponse(uint,QVariantMap)));
		return {};
	}

	if (!reply.arguments().isEmpty())
	{
		Debug(_log, "[desktop-portal] %s: actual returned request path %s",
			QSTRING_CSTR(method), QSTRING_CSTR(reply.arguments().at(0).value<QDBusObjectPath>().path()));
	}

	watcher.loop.exec();

	PortalResponse pr;
	pr.ok = (watcher.response == 0);
	pr.code = watcher.response;
	pr.results = watcher.results;
	return pr;
}

bool DesktopPortalGrabber::negotiatePortalSession()
{
	QDBusConnection bus = QDBusConnection::sessionBus();
	if (!bus.isConnected())
	{
		Warning(_log, "[desktop-portal] Session D-Bus is not connected");
		return false;
	}

	PortalResponse createResp = callPortalRequest(QStringLiteral("CreateSession"), {},
		{{"session_handle_token", newHandleToken()}});
	if (!createResp.ok || !createResp.results.contains("session_handle"))
	{
		Warning(_log, "[desktop-portal] CreateSession failed or was denied (response code %u)", createResp.code);
		return false;
	}
	_sessionHandle = createResp.results.value("session_handle").toString();

	QVariantMap selectOptions{
		{"types", QVariant::fromValue<uint32_t>(1)},      // MONITOR
		{"multiple", false},
		{"cursor_mode", QVariant::fromValue<uint32_t>(1)}, // HIDDEN - ambient lighting doesn't need the cursor
		{"persist_mode", QVariant::fromValue<uint32_t>(2)}, // persist until explicitly revoked
	};
	const QString savedToken = readRestoreToken();
	if (!savedToken.isEmpty())
	{
		selectOptions["restore_token"] = savedToken;
	}

	PortalResponse selectResp = callPortalRequest(QStringLiteral("SelectSources"),
		{QVariant::fromValue(QDBusObjectPath(_sessionHandle))}, selectOptions);
	if (!selectResp.ok)
	{
		Warning(_log, "[desktop-portal] SelectSources failed (response code %u)", selectResp.code);
		closePortalSession();
		return false;
	}

	// Start() is what triggers the one-time permission dialog (skipped by the portal if
	// savedToken above was accepted) - can block here for an arbitrarily long time waiting on
	// the user, which is fine since this all runs on our own background thread.
	PortalResponse startResp = callPortalRequest(QStringLiteral("Start"),
		{QVariant::fromValue(QDBusObjectPath(_sessionHandle)), QString()}, {});
	if (!startResp.ok || !startResp.results.contains("streams"))
	{
		Warning(_log, "[desktop-portal] Start failed or was denied (response code %u)", startResp.code);
		closePortalSession();
		return false;
	}

	_portalNodeId = extractFirstStreamNodeId(startResp.results.value("streams"));
	if (startResp.results.contains("restore_token"))
	{
		writeRestoreToken(startResp.results.value("restore_token").toString());
	}

	QDBusMessage openMsg = QDBusMessage::createMethodCall(PORTAL_SERVICE, PORTAL_PATH, SCREENCAST_IFACE, QStringLiteral("OpenPipeWireRemote"));
	openMsg.setArguments({QVariant::fromValue(QDBusObjectPath(_sessionHandle)), QVariant(QVariantMap())});
	QDBusMessage openReply = bus.call(openMsg);
	if (openReply.type() != QDBusMessage::ReplyMessage || openReply.arguments().isEmpty())
	{
		Warning(_log, "[desktop-portal] OpenPipeWireRemote failed: %s", QSTRING_CSTR(openReply.errorMessage()));
		closePortalSession();
		return false;
	}

	QDBusUnixFileDescriptor ufd = openReply.arguments().at(0).value<QDBusUnixFileDescriptor>();
	if (!ufd.isValid())
	{
		Warning(_log, "[desktop-portal] OpenPipeWireRemote returned an invalid file descriptor");
		closePortalSession();
		return false;
	}

	// pw_context_connect_fd() takes ownership of the fd it's given and closes it itself once
	// the connection ends; ufd closes its own copy when it goes out of scope at the end of
	// this function, so PipeWire must get a dup(), not the original fd, or we'd double-close.
	_pipewireFd = ::dup(ufd.fileDescriptor());
	if (_pipewireFd < 0)
	{
		Warning(_log, "[desktop-portal] Failed to dup() the Pipewire remote fd");
		closePortalSession();
		return false;
	}

	return true;
}

void DesktopPortalGrabber::closePortalSession()
{
	if (!_sessionHandle.isEmpty())
	{
		QDBusMessage closeMsg = QDBusMessage::createMethodCall(PORTAL_SERVICE, _sessionHandle, SESSION_IFACE, QStringLiteral("Close"));
		QDBusConnection::sessionBus().call(closeMsg, QDBus::NoBlock);
		_sessionHandle.clear();
	}
	_portalNodeId = 0;
}

void DesktopPortalGrabber::onCoreError(void* userdata, uint32_t id, int seq, int res, const char* message)
{
	auto* self = static_cast<DesktopPortalGrabber*>(userdata);
	Error(self->_log, "[desktop-portal] Error id: %d seq: %d res: %d (%s) %s", id, seq, res, strerror(res), message);
}

void DesktopPortalGrabber::onStreamStateChanged(void* userdata, enum pw_stream_state /*old*/, enum pw_stream_state state, const char* error)
{
	auto* self = static_cast<DesktopPortalGrabber*>(userdata);

	if (state == PW_STREAM_STATE_ERROR || state == PW_STREAM_STATE_UNCONNECTED)
	{
		if (error != nullptr)
		{
			Warning(self->_log, "[desktop-portal] stream ended: %s", error);
		}

		self->_connected = false;
		self->_width = 0;
		self->_height = 0;

		{
			std::lock_guard<std::mutex> lock(self->_bufferMutex);
			self->_frontBuffer.valid = false;
		}

		// Unblock runStream()'s pw_main_loop_run() so the outer retry loop can close this
		// session and renegotiate a fresh one.
		if (self->_loop)
		{
			pw_main_loop_quit(self->_loop);
		}
	}
}

void DesktopPortalGrabber::onStreamParamChanged(void* userdata, uint32_t id, const struct spa_pod* param)
{
	auto* self = static_cast<DesktopPortalGrabber*>(userdata);

	if (param == nullptr || id != SPA_PARAM_Format)
	{
		return;
	}

	if (spa_format_parse(param, &self->_format.media_type, &self->_format.media_subtype) < 0)
	{
		return;
	}

	if (self->_format.media_type != SPA_MEDIA_TYPE_video || self->_format.media_subtype != SPA_MEDIA_SUBTYPE_raw)
	{
		return;
	}

	if (spa_format_video_raw_parse(param, &self->_format.info.raw) < 0)
	{
		return;
	}

	// Report the *sampled* size (see READ_STRIDE_FACTOR above), not the raw negotiated one -
	// this is what onStreamProcess() actually produces into _frontBuffer, and getImageWidth()/
	// getImageHeight() (backed by _width/_height) is what GrabberWrapper::transferFrame() uses
	// to size its destination Image before calling grabFrame(). Reporting the native size here
	// while capturing at a reduced size would force an expensive upscale on every frame.
	self->_width = std::max(1, static_cast<int>(self->_format.info.raw.size.width) / READ_STRIDE_FACTOR);
	self->_height = std::max(1, static_cast<int>(self->_format.info.raw.size.height) / READ_STRIDE_FACTOR);
	self->_connected = true;

	// One-time note of which format alternative won and whether it's a tiled/compressed
	// DMA-BUF - useful to know on a GPU/driver combo nobody's tested this against yet, since
	// the two alternatives above behave very differently (see the comment there).
	Info(self->_log, "[desktop-portal] Negotiated %ux%u, modifier=%lld",
		 self->_format.info.raw.size.width, self->_format.info.raw.size.height,
		 static_cast<long long>(self->_format.info.raw.modifier));
}

void DesktopPortalGrabber::onStreamProcess(void* userdata)
{
	auto* self = static_cast<DesktopPortalGrabber*>(userdata);

	struct pw_buffer* pwBuffer = pw_stream_dequeue_buffer(self->_stream);
	if (pwBuffer == nullptr)
	{
		return;
	}

	// KWin's screencast implementation delivers frames as fast as this consumer keeps up with -
	// it doesn't honor the negotiated SPA_FORMAT_VIDEO_framerate max at all (confirmed: capping
	// that to 30fps had zero effect on delivered rate or CPU; separately, cutting per-frame
	// processing cost in half just made KWin deliver frames roughly twice as often instead of
	// reducing CPU, since faster consumption directly invites faster production). So the actual
	// throttle has to live here, on the consumer side: skip (immediately requeue, no format/
	// mapping/copy work at all) any buffer that arrives before enough time has passed since the
	// last one actually processed, targeting Hyperion's own configured capture rate (self->_fps)
	// - there's no benefit processing faster than the rate grabFrame() is actually polled at,
	// since the double-buffer's contents would just be overwritten before the main thread ever
	// reads them. See FINDINGS.md's CPU-cost section for the full before/after numbers.
	static auto lastProcessed = std::chrono::steady_clock::now() - std::chrono::seconds(1);
	const int targetFps = self->_fps > 0 ? self->_fps : 30;
	const auto minInterval = std::chrono::microseconds(1000000 / targetFps);
	const auto throttleNow = std::chrono::steady_clock::now();
	if (throttleNow - lastProcessed < minInterval)
	{
		pw_stream_queue_buffer(self->_stream, pwBuffer);
		return;
	}
	lastProcessed = throttleNow;

	spa_buffer* spaBuffer = pwBuffer->buffer;

	const auto width = static_cast<int>(self->_format.info.raw.size.width);
	const auto height = static_cast<int>(self->_format.info.raw.size.height);
	const auto spaFormat = self->_format.info.raw.format;

	auto* chunk = spaBuffer->datas[0].chunk;

	PixelFormat pixelFormat;
	switch (spaFormat)
	{
		case SPA_VIDEO_FORMAT_BGRx:
		case SPA_VIDEO_FORMAT_BGRA:
			pixelFormat = PixelFormat::BGR32;
			break;
		case SPA_VIDEO_FORMAT_RGBx:
		case SPA_VIDEO_FORMAT_RGBA:
			pixelFormat = PixelFormat::RGB32;
			break;
		default:
			pixelFormat = PixelFormat::NO_CHANGE;
			break;
	}

	if (width == 0 || height == 0 || chunk == nullptr || chunk->size == 0 || pixelFormat == PixelFormat::NO_CHANGE)
	{
		pw_stream_queue_buffer(self->_stream, pwBuffer);
		return;
	}

	const size_t stride = chunk->stride > 0
		? static_cast<size_t>(chunk->stride)
		: static_cast<size_t>(width) * 4;

	// Same defensive DMA-BUF handling as GamescopeGrabber - a normal desktop compositor's
	// composited output can just as easily hand back GPU-native buffers as gamescope's does -
	// but unlike gamescope's typically-short-lived sessions, a desktop-portal stream can run
	// for a long time at full desktop resolution, and DMA-BUF fds are stable for the stream's
	// whole lifetime (PipeWire cycles a small fixed buffer pool, not a fresh allocation per
	// frame). mmap()ing and munmap()ing a large region on every single frame measured at
	// ~110%+ sustained CPU for a 3840x1600 stream; caching the mapping per fd (torn down only
	// on disconnect, via clearDmaBufMappings()) removes that syscall/page-fault churn entirely.
	// One-time note of the actual delivered buffer type - see the params[] comment in
	// runStream(): this can legitimately be DmaBuf (AMD/Mesa) or MemFd (seen on NVIDIA, via the
	// no-modifier fallback format), and both are handled below.
	{
		static bool loggedBufferType = false;
		if (!loggedBufferType)
		{
			loggedBufferType = true;
			Info(self->_log, "[desktop-portal] Buffer data type=%u (MemFd=%d DmaBuf=%d MemPtr=%d)",
				 spaBuffer->datas[0].type, SPA_DATA_MemFd, SPA_DATA_DmaBuf, SPA_DATA_MemPtr);
		}
	}

	void* readPtr = spaBuffer->datas[0].data;
	const bool needsMapping = readPtr == nullptr
		&& (spaBuffer->datas[0].type == SPA_DATA_MemFd || spaBuffer->datas[0].type == SPA_DATA_DmaBuf)
		&& spaBuffer->datas[0].fd >= 0;

	if (needsMapping)
	{
		const int fd = static_cast<int>(spaBuffer->datas[0].fd);
		auto it = self->_dmaBufMappings.find(fd);
		if (it != self->_dmaBufMappings.end())
		{
			readPtr = it->second.first;
		}
		else
		{
			const size_t mapSize = static_cast<size_t>(spaBuffer->datas[0].maxsize) + chunk->offset;
			void* mapped = mmap(nullptr, mapSize, PROT_READ, MAP_SHARED, fd, 0);
			if (mapped != MAP_FAILED)
			{
				self->_dmaBufMappings.emplace(fd, std::make_pair(mapped, mapSize));
				readPtr = mapped;
			}
		}
	}

	if (readPtr == nullptr)
	{
		pw_stream_queue_buffer(self->_stream, pwBuffer);
		return;
	}

	const auto* src = static_cast<const uint8_t*>(readPtr) + chunk->offset;

	// Reading DMA-BUF-backed compositor memory from the CPU is dramatically slower than a
	// plain RAM-to-RAM copy (measured ~115ms for a full-frame sequential copy of a 3840x1600
	// frame - roughly 200MB/s effective, vs multi-GB/s for normal memcpy - almost certainly
	// because it's mapped write-combined/uncached, optimized for GPU access, not CPU reads).
	// Requesting a smaller resolution at the SPA_FORMAT_VIDEO_size negotiation failed outright
	// here ("no more input formats") - KWin's screencast implementation only offers native
	// resolution, no server-side downscaling.
	//
	// A first attempt read every Nth row *and* column directly out of the slow source (a
	// scattered/strided access pattern) - measured almost no improvement (~55ms for 1/16th the
	// pixels) despite touching far fewer bytes. Write-combined/uncached memory is optimized for
	// sequential burst access; a strided read defeats that and pays close to full per-cache-line
	// latency for every touch regardless of how little of each line is used. Reading every Nth
	// ROW but the FULL WIDTH of each selected row - i.e. staying sequential within each touch of
	// the slow memory - and only subsampling COLUMNS afterward, on the fast in-RAM copy, is what
	// actually cuts cost: bytes touched in slow memory drop by READ_STRIDE_FACTOR (rows skipped
	// only, not columns), but every touch is sequential.
	const int sampledWidth = std::max(1, width / READ_STRIDE_FACTOR);
	const int sampledHeight = std::max(1, height / READ_STRIDE_FACTOR);
	const size_t sampledStride = static_cast<size_t>(sampledWidth) * 4;

	static thread_local std::vector<uint8_t> rowStaging;
	rowStaging.resize(static_cast<size_t>(width) * 4);

	{
		std::lock_guard<std::mutex> lock(self->_bufferMutex);
		self->_frontBuffer.data.resize(sampledStride * static_cast<size_t>(sampledHeight));

		for (int y = 0; y < sampledHeight; ++y)
		{
			const auto* srcRow = src + static_cast<size_t>(y) * READ_STRIDE_FACTOR * stride;
			// Sequential full-row read from the slow source into fast RAM.
			std::memcpy(rowStaging.data(), srcRow, static_cast<size_t>(width) * 4);

			// Column subsampling now happens entirely on the already-fast staging buffer.
			const auto* stagingRow32 = reinterpret_cast<const uint32_t*>(rowStaging.data());
			auto* dstRow32 = reinterpret_cast<uint32_t*>(self->_frontBuffer.data.data() + static_cast<size_t>(y) * sampledStride);
			for (int x = 0; x < sampledWidth; ++x)
			{
				dstRow32[x] = stagingRow32[static_cast<size_t>(x) * READ_STRIDE_FACTOR];
			}
		}

		self->_frontBuffer.width = sampledWidth;
		self->_frontBuffer.height = sampledHeight;
		self->_frontBuffer.stride = sampledStride;
		self->_frontBuffer.pixelFormat = pixelFormat;
		self->_frontBuffer.valid = true;
	}

	pw_stream_queue_buffer(self->_stream, pwBuffer);
}

bool DesktopPortalGrabber::connectCore()
{
	// _coreEvents is a member (not a local) because Pipewire's listener registration stores a
	// pointer to it rather than copying it - see GamescopeGrabber's connectCore() for the full
	// explanation (a real crash was hit and fixed there before this grabber existed).
	_coreEvents.version = PW_VERSION_CORE_EVENTS;
	_coreEvents.error = &DesktopPortalGrabber::onCoreError;

	_loop = pw_main_loop_new(nullptr);
	_context = pw_context_new(pw_main_loop_get_loop(_loop), nullptr, 0);

	const int fd = _pipewireFd;
	_pipewireFd = -1; // pw_context_connect_fd() always takes ownership of the fd, win or lose
	_core = pw_context_connect_fd(_context, fd, nullptr, 0);

	if (_core == nullptr)
	{
		Error(_log, "[desktop-portal] Failed to connect to the portal's Pipewire remote");
		pw_context_destroy(_context);
		_context = nullptr;
		pw_main_loop_destroy(_loop);
		_loop = nullptr;
		return false;
	}

	pw_core_add_listener(_core, &_coreListener, &_coreEvents, this);
	return true;
}

void DesktopPortalGrabber::runStream()
{
	pw_stream_events streamEvents{};
	streamEvents.version = PW_VERSION_STREAM_EVENTS;
	streamEvents.param_changed = &DesktopPortalGrabber::onStreamParamChanged;
	streamEvents.process = &DesktopPortalGrabber::onStreamProcess;
	streamEvents.state_changed = &DesktopPortalGrabber::onStreamStateChanged;

	pw_properties* props = pw_properties_new(
		PW_KEY_MEDIA_TYPE, "Video",
		PW_KEY_MEDIA_CATEGORY, "Capture",
		PW_KEY_MEDIA_ROLE, "Screen",
		nullptr
	);

	_stream = pw_stream_new_simple(pw_main_loop_get_loop(_loop), "HyperionDesktopPortalStream", props, &streamEvents, this);

	uint8_t buffer[1024];
	spa_pod_builder podBuilder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

	spa_rectangle rectMin = SPA_RECTANGLE(1, 1);
	spa_rectangle rectDefault = SPA_RECTANGLE(320, 240);
	// Tried constraining this down (e.g. to 960x540) to get the compositor to scale down before
	// handing us the buffer - KWin's screencast implementation refused entirely ("no more input
	// formats"), meaning it only offers native resolution here. Reverted to accepting whatever
	// native size is offered; see onStreamProcess()'s row/column-strided read below for the
	// actual fix to the CPU cost this causes.
	spa_rectangle rectMax = SPA_RECTANGLE(4096, 4096);

	spa_fraction rateMin = SPA_FRACTION(0, 1);
	spa_fraction rateDefault = SPA_FRACTION(25, 1);
	// Capped at 30fps (matching GrabberWrapper::DEFAULT_MAX_GRAB_RATE_HZ), unlike
	// GamescopeGrabber's effectively-unbounded 1000fps ceiling: gamescope's own output is
	// already naturally capped near the game's target framerate, but a normal desktop
	// compositor will happily negotiate up to full native display refresh rate (confirmed:
	// left uncapped, this pinned a full CPU core continuously copying frames onStreamProcess()
	// never needs for ambient lighting) if given the room to.
	spa_fraction rateMax = SPA_FRACTION(30, 1);

	// Two format alternatives, most-preferred first. params[0] is the original DMA-BUF request
	// with a mandatory LINEAR modifier - cheap for this code to decode, and what AMD/Mesa
	// (Framework desktop, Strix Halo APU - see FINDINGS.md) happily grants. On NVIDIA's
	// proprietary driver, KWin's screencast source only ever offers its own tiled/compressed
	// modifiers for that path, never LINEAR, so params[0] never negotiates there - confirmed via
	// `pw.link: negotiating -> error no more input formats (-22)` in the journal every retry.
	// params[1] is the same format/size/framerate constraints with NO modifier property at all,
	// which tells PipeWire this client will also accept a non-DMA-BUF (plain memory) buffer.
	// Confirmed via `pw-dump` after adding this: on the NVIDIA machine, negotiation now succeeds
	// through params[1] - the resulting Format param has no "modifier" key at all (plain BGRx,
	// 3840x1600), and the PipeWire link shows state "active", not stuck retrying. So KWin's
	// NVIDIA-backed screencast implementation *does* support handing back a non-tiled buffer,
	// it just won't do it unless a client explicitly signals it'll accept one.
	uint8_t buffer2[1024];
	spa_pod_builder podBuilder2 = SPA_POD_BUILDER_INIT(buffer2, sizeof(buffer2));

	const spa_pod* params[2] = {
		static_cast<spa_pod*>(spa_pod_builder_add_object(&podBuilder,
			SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
			SPA_FORMAT_mediaType,    SPA_POD_Id(SPA_MEDIA_TYPE_video),
			SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
			SPA_FORMAT_VIDEO_format, SPA_POD_CHOICE_ENUM_Id(4,
				SPA_VIDEO_FORMAT_BGRx,
				SPA_VIDEO_FORMAT_BGRx,
				SPA_VIDEO_FORMAT_RGBx,
				SPA_VIDEO_FORMAT_BGRA
			),
			SPA_FORMAT_VIDEO_size, SPA_POD_CHOICE_RANGE_Rectangle(&rectDefault, &rectMin, &rectMax),
			SPA_FORMAT_VIDEO_framerate, SPA_POD_CHOICE_RANGE_Fraction(&rateDefault, &rateMin, &rateMax),
			// Same LINEAR-modifier requirement as GamescopeGrabber - a normal compositor's
			// composited output can be a tiled/compressed DMA-BUF just as easily as
			// gamescope's, and this code has no way to decode that layout.
			SPA_POD_Propf(SPA_FORMAT_VIDEO_modifier, SPA_POD_PROP_FLAG_MANDATORY, SPA_POD_CHOICE_ENUM_Long(2, 0L, 0L))
		)),
		static_cast<spa_pod*>(spa_pod_builder_add_object(&podBuilder2,
			SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
			SPA_FORMAT_mediaType,    SPA_POD_Id(SPA_MEDIA_TYPE_video),
			SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
			SPA_FORMAT_VIDEO_format, SPA_POD_CHOICE_ENUM_Id(4,
				SPA_VIDEO_FORMAT_BGRx,
				SPA_VIDEO_FORMAT_BGRx,
				SPA_VIDEO_FORMAT_RGBx,
				SPA_VIDEO_FORMAT_BGRA
			),
			SPA_FORMAT_VIDEO_size, SPA_POD_CHOICE_RANGE_Rectangle(&rectDefault, &rectMin, &rectMax),
			SPA_FORMAT_VIDEO_framerate, SPA_POD_CHOICE_RANGE_Fraction(&rateDefault, &rateMin, &rateMax)
			// Deliberately no modifier property - see the comment above params[] for why.
		))
	};

	pw_stream_connect(_stream, PW_DIRECTION_INPUT, _portalNodeId,
		static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS),
		params, 2);

	// Blocks until either the stream errors/disconnects (onStreamStateChanged quits the loop
	// so we can renegotiate) or stop() quits it for good.
	pw_main_loop_run(_loop);

	pw_stream_disconnect(_stream);
	pw_stream_destroy(_stream);
	_stream = nullptr;

	// The buffer pool (and its fds) this stream owned is gone now - drop the cached mappings
	// so a fresh negotiation's buffers (almost certainly different fds) start with a clean
	// cache instead of accumulating stale entries across reconnects.
	clearDmaBufMappings();

	_connected = false;
}

void DesktopPortalGrabber::clearDmaBufMappings()
{
	for (auto& [fd, mapping] : _dmaBufMappings)
	{
		munmap(mapping.first, mapping.second);
	}
	_dmaBufMappings.clear();
}

void DesktopPortalGrabber::waitBeforeRetry()
{
	std::unique_lock<std::mutex> lock(_retryMutex);
	_retryCv.wait_for(lock, RETRY_INTERVAL, [this] { return _stopping.load(); });
}

void DesktopPortalGrabber::portalThreadMain()
{
	pw_init(nullptr, nullptr);

	while (!_stopping.load())
	{
		if (!negotiatePortalSession())
		{
			waitBeforeRetry();
			continue;
		}

		if (_stopping.load())
		{
			closePortalSession();
			break;
		}

		if (!connectCore())
		{
			closePortalSession();
			waitBeforeRetry();
			continue;
		}

		runStream();

		if (_core)
		{
			pw_core_disconnect(_core);
			_core = nullptr;
		}

		if (_context)
		{
			pw_context_destroy(_context);
			_context = nullptr;
		}

		if (_loop)
		{
			pw_main_loop_destroy(_loop);
			_loop = nullptr;
		}

		closePortalSession();

		if (_stopping.load())
		{
			break;
		}

		waitBeforeRetry();
	}

	pw_deinit();
}

#include "DesktopPortalGrabber.moc"
