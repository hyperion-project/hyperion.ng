#include "PipeWireFrameSource.h"

#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>
#include <spa/param/video/type-info.h>

#include <fcntl.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>

#include <QElapsedTimer>

#include <grabber/GrabberTrace.h>
#include <utils/PixelFormat.h>

namespace {
	constexpr int RETRY_INTERVAL_MS = 2000;

	// Matches GrabberWrapper::DEFAULT_MAX_GRAB_RATE_HZ
	constexpr int DEFAULT_FPS = 30;
}

static QString getStreamState(enum pw_stream_state state)
{
	switch (state)
	{
	case PW_STREAM_STATE_ERROR:
		return "IN ERROR";
	case PW_STREAM_STATE_UNCONNECTED:
		return "UNCONNECTED";
	case PW_STREAM_STATE_CONNECTING:
		return "CONNECTING";
	case PW_STREAM_STATE_PAUSED:
		return "PAUSED";
	case PW_STREAM_STATE_STREAMING:
		return "STREAMING";
	default:
		break;
	}
	// Return a default string if the state is unrecognized
	return "UNKNOWN";
}

struct PipeWireFrameSource::Impl
{
	pw_thread_loop* loop = nullptr;
	pw_context* context = nullptr;
	pw_core* core = nullptr;
	pw_stream* stream = nullptr;
	spa_hook listener{};
	pw_stream_events events{};

	spa_video_info_raw format{};
	bool haveFormat = false;
	bool pwInitialized = false;

	std::mutex mutex;
	CapturedFrame frame;
	bool haveFrame = false;
	std::chrono::steady_clock::time_point lastCopy{};

	std::atomic<bool> failed{false};
	std::atomic<int> minIntervalMs{25};

	// Frame-rate benchmarking state for onProcess(), only touched (and only meaningful)
	// when the grabber_screen_benchmark debug category is enabled
	QAtomicInt currentFrameCount{0};
	QElapsedTimer frameTimer;	

	static void onStateChanged(void* data, pw_stream_state, pw_stream_state state, const char* error)
	{
		qCDebug(grabber_screen_flow) << "Stream state changed to" << getStreamState(state) << ", error:" << (error ? error : "(none)");
		auto* self = static_cast<Impl*>(data);
		if (state == PW_STREAM_STATE_ERROR || state == PW_STREAM_STATE_UNCONNECTED)
		{
			if (error != nullptr)
			{
				// UNCONNECTED is an expected, routine transition (e.g. permission revoked, portal
				// session ended normally) - not worth a Warning. A real problem still surfaces via
				// ERROR.
				if (state == PW_STREAM_STATE_ERROR)
				{
					qCWarning(grabber_screen_flow) << "Pipewire stream ended:" << error;
				}
				else
				{
					qCDebug(grabber_screen_flow) << "Pipewire stream ended:" << error;
				}
			}

			self->failed = true;
		}
	}

	static void onParamChanged(void* data, uint32_t id, const spa_pod* param)
	{
		qCDebug(grabber_screen_flow) << "Stream param changed, id=" << id << ", param=" << (param ? "valid" : "null");
		auto* self = static_cast<Impl*>(data);

		if (param == nullptr || id != SPA_PARAM_Format)
		{
			return;
		}

		spa_video_info info{};
		if (spa_format_parse(param, &info.media_type, &info.media_subtype) < 0 ||
			info.media_type != SPA_MEDIA_TYPE_video || info.media_subtype != SPA_MEDIA_SUBTYPE_raw)
		{
			return;
		}

		if (spa_format_video_raw_parse(param, &self->format) < 0)
		{
			return;
		}

		self->haveFormat = true;

		// Ask for memory the CPU can map directly (no DMA-BUF, so no tiling modifiers).
		uint8_t buffer[512];
		spa_pod_builder builder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
		const spa_pod* params[1];
		params[0] = static_cast<const spa_pod*>(spa_pod_builder_add_object(&builder,
			SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
			SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(4, 2, 8),
			SPA_PARAM_BUFFERS_dataType, SPA_POD_CHOICE_FLAGS_Int((1 << SPA_DATA_MemPtr) | (1 << SPA_DATA_MemFd))));
		pw_stream_update_params(self->stream, params, 1);

		qCDebug(grabber_screen_flow) << "Stream format changed to" << self->format.size.width
				<< "x" << self->format.size.height
				<< ", modifier:" << self->format.modifier
				<< ", format:" << self->format.format;
	}

	static void onProcess(void* data)
	{
		auto* self = static_cast<Impl*>(data);
		pw_buffer* pwBuffer = pw_stream_dequeue_buffer(self->stream);
		if (pwBuffer == nullptr)
		{
			return;
		}

		const spa_data& d = pwBuffer->buffer->datas[0];
		const auto now = std::chrono::steady_clock::now();

		if (self->haveFormat && d.data != nullptr && d.chunk != nullptr && d.chunk->size > 0 &&
			!(d.chunk->flags & SPA_CHUNK_FLAG_CORRUPTED) &&
			(now - self->lastCopy) >= std::chrono::milliseconds(self->minIntervalMs.load()))
		{
			const uint32_t width = self->format.size.width;
			const uint32_t height = self->format.size.height;
			const uint32_t size = d.chunk->size;
			const uint32_t offset = d.chunk->offset;
			const uint32_t strideFromSize = height > 0 ? size / height : 0;
			const uint32_t stride = d.chunk->stride > 0 ? static_cast<uint32_t>(d.chunk->stride) : strideFromSize;

			// The consumer reads width*4 bytes from each of `height` rows: never hand out a buffer
			// that is shorter than that or that lies outside the mapping.
			const uint64_t needed = height > 0 ? static_cast<uint64_t>(stride) * (height - 1) + static_cast<uint64_t>(width) * 4 : 0;
			if (needed > 0 && stride >= width * 4 && size >= needed && static_cast<uint64_t>(offset) + size <= d.maxsize)
			{
				self->lastCopy = now;
				const uint32_t fmt = self->format.format;

				PixelFormat pixelFormat;
				switch (fmt)
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

				std::lock_guard lock(self->mutex);
				self->frame.data = QByteArray(static_cast<const char*>(d.data) + offset, static_cast<int>(size));
				self->frame.width = static_cast<int>(width);
				self->frame.height = static_cast<int>(height);
				self->frame.stride = static_cast<int>(stride);
				self->frame.pixelFormat = pixelFormat;
				self->haveFrame = true;

				++self->currentFrameCount;
			}
		}

		qCDebug(grabber_screen_capture) << QString("Processing frame, Width: %1, Height: %2, SpaFormat: %3, PixelFormat: %4")
		.arg(self->frame.width)
		.arg(self->frame.height)
		.arg(self->format.format)
		.arg(pixelFormatToString(self->frame.pixelFormat));

		pw_stream_queue_buffer(self->stream, pwBuffer);

		if (grabber_screen_benchmark().isDebugEnabled())
		{
			// calculate average frametime
			if (self->currentFrameCount > 1)
			{
				if (self->currentFrameCount % 100 == 0)
				{
					qint64 timeElaped = self->frameTimer.restart() / 100;
					qCDebug(grabber_screen_benchmark) << self->currentFrameCount << "frames captured. avg time=" 
					<< timeElaped << "ms" << "/" << 1000 / timeElaped << "fps" 
					<< "vs." << self->minIntervalMs << "ms" << "/" << 1000 / self->minIntervalMs << "fps max configured";
				}
			}
			else
			{
				qCDebug(grabber_screen_benchmark) << self->currentFrameCount << ": frametimer started";
				self->frameTimer.start();
			}

		}
	}
};

PipeWireFrameSource::PipeWireFrameSource()
	: _impl(std::make_unique<Impl>())
{
	TRACK_SCOPE() << "Creating PipeWire frame source";
}

PipeWireFrameSource::~PipeWireFrameSource()
{
	TRACK_SCOPE();
	stop();
}

bool PipeWireFrameSource::start(int fd, quint32 nodeId, QString& error)
{
	qCDebug(grabber_screen_flow) << "Start Pipewire stream";
	stop();

	qCDebug(grabber_screen_flow) << "Initializing Pipewire";
	pw_init(nullptr, nullptr);

	Impl& s = *_impl;
	s.pwInitialized = true;
	s.failed = false;
	s.haveFormat = false;
	s.haveFrame = false;

	s.loop = pw_thread_loop_new("hyperion-pipewire", nullptr);
	if (s.loop == nullptr)
	{
		error = QStringLiteral("Cannot create the PipeWire loop");
		stop();
		s.failed = true;
		return false;
	}

	pw_thread_loop_lock(s.loop);

	s.context = pw_context_new(pw_thread_loop_get_loop(s.loop), nullptr, 0);
	const int ownFd = fcntl(fd, F_DUPFD_CLOEXEC, 3);   // pw_context_connect_fd takes ownership
	if (s.context != nullptr && ownFd >= 0)
	{
		s.core = pw_context_connect_fd(s.context, ownFd, nullptr, 0);
	}
	if (s.core == nullptr)
	{
		pw_thread_loop_unlock(s.loop);
		error = QStringLiteral("Cannot connect to the PipeWire remote");
		stop();
		s.failed = true;
		return false;
	}

	s.stream = pw_stream_new(s.core, "hyperion-screen-capture",
							 pw_properties_new(PW_KEY_MEDIA_TYPE, "Video", PW_KEY_MEDIA_CATEGORY, "Capture",
											   PW_KEY_MEDIA_ROLE, "Screen", nullptr));

	s.events.version = PW_VERSION_STREAM_EVENTS;
	s.events.state_changed = Impl::onStateChanged;
	s.events.param_changed = Impl::onParamChanged;
	s.events.process = Impl::onProcess;
	pw_stream_add_listener(s.stream, &s.listener, &s.events, _impl.get());

	uint8_t buffer[1024];
	spa_pod_builder builder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
	const spa_rectangle defSize{1920, 1080};
	const spa_rectangle minSize{1, 1};
	const spa_rectangle maxSize{16384, 16384};
	const spa_fraction defRate{DEFAULT_FPS, 1};
	const spa_fraction minRate{0, 1};
	const spa_fraction maxRate{MAX_FPS, 1};
	const spa_pod* params[1];
	params[0] = static_cast<const spa_pod*>(spa_pod_builder_add_object(&builder,
				SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
				SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video),
				SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
				SPA_FORMAT_VIDEO_format, SPA_POD_CHOICE_ENUM_Id(5,
					SPA_VIDEO_FORMAT_BGRx,
					SPA_VIDEO_FORMAT_BGRx,
					SPA_VIDEO_FORMAT_BGRA,
					SPA_VIDEO_FORMAT_RGBx,
					SPA_VIDEO_FORMAT_RGBA
				),
				SPA_FORMAT_VIDEO_size, SPA_POD_CHOICE_RANGE_Rectangle(&defSize, &minSize, &maxSize),
				SPA_FORMAT_VIDEO_framerate, SPA_POD_CHOICE_RANGE_Fraction(&defRate, &minRate, &maxRate))
			);

	const int rc = pw_stream_connect(s.stream, PW_DIRECTION_INPUT, nodeId,
									 static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS),
									 params, 1);
	if (rc < 0)
	{
		pw_thread_loop_unlock(s.loop);
		error = QStringLiteral("Cannot connect the PipeWire stream: %1").arg(QString::fromUtf8(strerror(-rc)));
		stop();
		s.failed = true;
		return false;
	}

	pw_thread_loop_unlock(s.loop);
	pw_thread_loop_start(s.loop);
	return true;
}

bool PipeWireFrameSource::startDirect(const QString& targetNodeName, QString& error)
{
	qCDebug(grabber_screen_flow) << "Start direct PipeWire stream for node" << targetNodeName;
	stop();

	qCDebug(grabber_screen_flow) << "Initializing PipeWire (direct)";
	pw_init(nullptr, nullptr);

	Impl& s = *_impl;
	s.pwInitialized = true;
	s.failed        = false;
	s.haveFormat    = false;
	s.haveFrame     = false;

	s.loop = pw_thread_loop_new("hyperion-pipewire-gs", nullptr);
	if (s.loop == nullptr)
	{
		error = QStringLiteral("Cannot create the PipeWire loop");
		stop();
		s.failed = true;
		return false;
	}

	pw_thread_loop_lock(s.loop);

	s.context = pw_context_new(pw_thread_loop_get_loop(s.loop), nullptr, 0);
	if (s.context != nullptr)
	{
		// Connect to the user's default PipeWire session socket directly.
		s.core = pw_context_connect(s.context, nullptr, 0);
	}
	if (s.core == nullptr)
	{
		pw_thread_loop_unlock(s.loop);
		error = QStringLiteral("Cannot connect to the PipeWire daemon");
		stop();
		s.failed = true;
		return false;
	}

	// Provide the target node name so WirePlumber links us automatically.
	const QByteArray targetNameBytes = targetNodeName.toUtf8();
	s.stream = pw_stream_new(s.core, "hyperion-gamescope-capture",
	                         pw_properties_new(
	                             PW_KEY_MEDIA_TYPE,      "Video",
	                             PW_KEY_MEDIA_CATEGORY,  "Capture",
	                             PW_KEY_MEDIA_ROLE,      "Screen",
	                             PW_KEY_TARGET_OBJECT,   targetNameBytes.constData(),
	                             nullptr));

	s.events.version       = PW_VERSION_STREAM_EVENTS;
	s.events.state_changed = Impl::onStateChanged;
	s.events.param_changed = Impl::onParamChanged;
	s.events.process       = Impl::onProcess;
	pw_stream_add_listener(s.stream, &s.listener, &s.events, _impl.get());

	uint8_t buffer[1024];
	spa_pod_builder builder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
	const spa_rectangle defSize{1920, 1080};
	const spa_rectangle minSize{1, 1};
	const spa_rectangle maxSize{16384, 16384};
	const spa_fraction defRate{DEFAULT_FPS, 1};
	const spa_fraction minRate{0, 1};
	const spa_fraction maxRate{MAX_FPS, 1};
	const spa_pod* params[1];
	params[0] = static_cast<const spa_pod*>(spa_pod_builder_add_object(&builder,
		SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
		SPA_FORMAT_mediaType,    SPA_POD_Id(SPA_MEDIA_TYPE_video),
		SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
		SPA_FORMAT_VIDEO_format, SPA_POD_CHOICE_ENUM_Id(5,
			SPA_VIDEO_FORMAT_BGRx,
			SPA_VIDEO_FORMAT_BGRx,
			SPA_VIDEO_FORMAT_BGRA,
			SPA_VIDEO_FORMAT_RGBx,
			SPA_VIDEO_FORMAT_RGBA
		),
		SPA_FORMAT_VIDEO_size,      SPA_POD_CHOICE_RANGE_Rectangle(&defSize, &minSize, &maxSize),
		SPA_FORMAT_VIDEO_framerate, SPA_POD_CHOICE_RANGE_Fraction(&defRate, &minRate, &maxRate))
	);

	// Use PW_ID_ANY so the session manager resolves PW_KEY_TARGET_OBJECT for us.
	const int rc = pw_stream_connect(s.stream, PW_DIRECTION_INPUT, PW_ID_ANY,
	                                 static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS),
	                                 params, 1);
	if (rc < 0)
	{
		pw_thread_loop_unlock(s.loop);
		error = QStringLiteral("Cannot connect the PipeWire stream: %1").arg(QString::fromUtf8(strerror(-rc)));
		stop();
		s.failed = true;
		return false;
	}

	pw_thread_loop_unlock(s.loop);
	pw_thread_loop_start(s.loop);
	return true;
}

void PipeWireFrameSource::stop()
{
	qCDebug(grabber_screen_flow) << "Stopping Pipewire stream";
	Impl& s = *_impl;
	if (s.loop != nullptr)
	{
		pw_thread_loop_stop(s.loop);
	}
	if (s.stream != nullptr)
	{
		spa_hook_remove(&s.listener);
		pw_stream_destroy(s.stream);
		s.stream = nullptr;
	}
	if (s.core != nullptr)
	{
		pw_core_disconnect(s.core);
		s.core = nullptr;
	}
	if (s.context != nullptr)
	{
		pw_context_destroy(s.context);
		s.context = nullptr;
	}
	if (s.loop != nullptr)
	{
		pw_thread_loop_destroy(s.loop);
		s.loop = nullptr;
	}
	if (s.pwInitialized)
	{
		qCDebug(grabber_screen_flow) << "Deinitializing Pipewire";
		pw_deinit();
		s.pwInitialized = false;
	}
	std::lock_guard lock(s.mutex);
	s.frame = CapturedFrame();
	s.haveFrame = false;
	qCDebug(grabber_screen_flow) << "Pipewire stream stopped, frame cleared";
}

bool PipeWireFrameSource::latest(CapturedFrame& out)
{
	std::lock_guard lock(_impl->mutex);
	if (!_impl->haveFrame)
	{
		qCDebug(grabber_screen_capture) << "No new frame available";
		return false;
	}
	out = _impl->frame;   // QByteArray is shared, no deep copy until the stream writes again
	return true;
}

void PipeWireFrameSource::setMinFrameInterval(int ms)
{
	qCDebug(grabber_screen_flow) << "Setting minimum frame interval to" << ms << "ms";
	_impl->minIntervalMs = qMax(1, ms);
}

bool PipeWireFrameSource::hasFailed() const
{
	qCDebug(grabber_screen_capture) << "Checking if Pipewire stream has failed:" << _impl->failed;
	return _impl->failed;
}
