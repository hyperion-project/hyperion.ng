#include "PipeWireFrameSource.h"

#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>
#include <spa/param/video/type-info.h>
#include <spa/utils/dict.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>

#include <QDir>
#include <QElapsedTimer>
#include <QHash>

#include <grabber/GrabberTrace.h>
#include <utils/PixelFormat.h>

namespace
{
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

		// Ask for memory the CPU can map directly. When a (linear) modifier was negotiated the
		// producer hands out DMA-BUFs, which are mmap-able as long as they are linear.
		const bool hasModifier = spa_pod_find_prop(param, nullptr, SPA_FORMAT_VIDEO_modifier) != nullptr;
		const int dataTypes = hasModifier
		                          ? (1 << SPA_DATA_DmaBuf)
		                          : ((1 << SPA_DATA_MemPtr) | (1 << SPA_DATA_MemFd));

		uint8_t buffer[512];
		spa_pod_builder builder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
		const spa_pod* params[1];
		params[0] = static_cast<const spa_pod*>(spa_pod_builder_add_object(&builder,
		                                                                   SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
		                                                                   SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(4, 2, 8),
		                                                                   SPA_PARAM_BUFFERS_dataType, SPA_POD_CHOICE_FLAGS_Int(dataTypes)));
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

		// DMA-BUFs are not always pre-mapped by MAP_BUFFERS; map the fd for this frame then.
		const auto* mapped = static_cast<const uint8_t*>(d.data);
		auto* localMap = MAP_FAILED;
		size_t localMapSize = 0;
		if (mapped == nullptr && d.fd >= 0 && d.chunk != nullptr && (d.type == SPA_DATA_DmaBuf || d.type == SPA_DATA_MemFd))
		{
			localMapSize = static_cast<size_t>(d.maxsize) + d.mapoffset;
			localMap = mmap(nullptr, localMapSize, PROT_READ, MAP_SHARED, static_cast<int>(d.fd), 0);
			if (localMap != MAP_FAILED)
			{
				mapped = static_cast<const uint8_t*>(localMap) + d.mapoffset;
			}
		}

		if (self->haveFormat && mapped != nullptr && d.chunk != nullptr && d.chunk->size > 0 &&
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
				self->frame.data = QByteArray(reinterpret_cast<const char*>(mapped) + offset, static_cast<int>(size));
				self->frame.width = static_cast<int>(width);
				self->frame.height = static_cast<int>(height);
				self->frame.stride = static_cast<int>(stride);
				self->frame.pixelFormat = pixelFormat;
				self->haveFrame = true;

				++self->currentFrameCount;
			}
		}

		if (localMap != MAP_FAILED)
		{
			munmap(localMap, localMapSize);
		}

		qCDebug(grabber_screen_capture) << QString("Processing frame, Width: %1, Height: %2, SpaFormat: %3, PixelFormat: %4")
		                                       .arg(self->frame.width)
		                                       .arg(self->frame.height)
		                                       .arg(self->format.format)
		                                       .arg(pixelFormatToString(self->frame.pixelFormat));

		pw_stream_queue_buffer(self->stream, pwBuffer);

		if (self->haveFrame && grabber_screen_benchmark().isDebugEnabled())
		{
			// calculate average frametime
			if (self->currentFrameCount > 1)
			{
				if (self->currentFrameCount % 100 == 0)
				{
					const qint64 timeElaped = std::max<qint64>(1, self->frameTimer.restart() / 100);
					const int configuredInterval = std::max(1, static_cast<int>(self->minIntervalMs.load()));
					qCDebug(grabber_screen_benchmark) << self->currentFrameCount << "frames captured. avg time="
					                                  << timeElaped << "ms" << "/" << 1000 / timeElaped << "fps"
					                                  << "vs." << configuredInterval << "ms" << "/" << 1000 / configuredInterval << "fps max configured";
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
	const int ownFd = fcntl(fd, F_DUPFD_CLOEXEC, 3); // pw_context_connect_fd takes ownership
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
	                                                                   SPA_FORMAT_VIDEO_format, SPA_POD_CHOICE_ENUM_Id(5, SPA_VIDEO_FORMAT_BGRx, SPA_VIDEO_FORMAT_BGRx, SPA_VIDEO_FORMAT_BGRA, SPA_VIDEO_FORMAT_RGBx, SPA_VIDEO_FORMAT_RGBA),
	                                                                   SPA_FORMAT_VIDEO_size, SPA_POD_CHOICE_RANGE_Rectangle(&defSize, &minSize, &maxSize),
	                                                                   SPA_FORMAT_VIDEO_framerate, SPA_POD_CHOICE_RANGE_Fraction(&defRate, &minRate, &maxRate)));

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

// Gamescope renders into GPU buffers. Requiring the LINEAR modifier (0) makes it hand out
// plain mmap-able DMA-BUFs instead of tiled/compressed layouts that cannot be decoded here.
static const spa_pod* buildGamescopeFormat(spa_pod_builder* builder)
{
	const spa_rectangle defSize{1920, 1080};
	const spa_rectangle minSize{1, 1};
	const spa_rectangle maxSize{16384, 16384};
	const spa_fraction defRate{DEFAULT_FPS, 1};
	const spa_fraction minRate{0, 1};
	const spa_fraction maxRate{PipeWireFrameSource::MAX_FPS, 1};

	spa_pod_frame objFrame;
	spa_pod_frame choiceFrame;
	spa_pod_builder_push_object(builder, &objFrame, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat);
	spa_pod_builder_add(builder,
	                    SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video),
	                    SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
	                    SPA_FORMAT_VIDEO_format, SPA_POD_CHOICE_ENUM_Id(5, SPA_VIDEO_FORMAT_BGRx, SPA_VIDEO_FORMAT_BGRx, SPA_VIDEO_FORMAT_BGRA, SPA_VIDEO_FORMAT_RGBx, SPA_VIDEO_FORMAT_RGBA),
	                    SPA_FORMAT_VIDEO_size, SPA_POD_CHOICE_RANGE_Rectangle(&defSize, &minSize, &maxSize),
	                    SPA_FORMAT_VIDEO_framerate, SPA_POD_CHOICE_RANGE_Fraction(&defRate, &minRate, &maxRate),
	                    0);
	spa_pod_builder_prop(builder, SPA_FORMAT_VIDEO_modifier, SPA_POD_PROP_FLAG_MANDATORY);
	spa_pod_builder_push_choice(builder, &choiceFrame, SPA_CHOICE_Enum, 0);
	spa_pod_builder_long(builder, 0);
	spa_pod_builder_long(builder, 0);
	spa_pod_builder_pop(builder, &choiceFrame);
	return static_cast<const spa_pod*>(spa_pod_builder_pop(builder, &objFrame));
}

bool PipeWireFrameSource::startDirect(const QString& targetNodeName, QString& error)
{
	qCDebug(grabber_screen_flow) << "Start direct PipeWire stream for node" << targetNodeName;
	stop();

	qCDebug(grabber_screen_flow) << "Initializing PipeWire (direct)";
	pw_init(nullptr, nullptr);

	Impl& s = *_impl;
	s.pwInitialized = true;
	s.failed = false;
	s.haveFormat = false;
	s.haveFrame = false;

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
	                             PW_KEY_MEDIA_TYPE, "Video",
	                             PW_KEY_MEDIA_CATEGORY, "Capture",
	                             PW_KEY_MEDIA_ROLE, "Screen",
	                             PW_KEY_TARGET_OBJECT, targetNameBytes.constData(),
	                             nullptr));

	s.events.version = PW_VERSION_STREAM_EVENTS;
	s.events.state_changed = Impl::onStateChanged;
	s.events.param_changed = Impl::onParamChanged;
	s.events.process = Impl::onProcess;
	pw_stream_add_listener(s.stream, &s.listener, &s.events, _impl.get());

	uint8_t buffer[1024];
	spa_pod_builder builder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
	const spa_pod* params[1];
	params[0] = buildGamescopeFormat(&builder);

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

// ---------------------------------------------------------------------------
// Gamescope node discovery
// ---------------------------------------------------------------------------
namespace
{

	struct DiscoveryState
	{
		QList<GamescopeNodeInfo> nodes;
		// Speichert Zuordnung von Client-ID zu echter Host-PID
		QHash<uint32_t, QString> clientPids;
		int syncSeq{0};
		pw_thread_loop* loop{nullptr};
		bool done{false};
	};

	// Helper: Find the deepest child process (the game) under the Gamescope PID
	static QString getGameBinaryFromGamescopePid(const QString& parentPidStr)
	{
		if (parentPidStr.isEmpty())
		{
			return QString();
		}

		QDir procDir("/proc");
		QStringList childrenPids;

		// 1. Find ALL child processes of the current PID
		QStringList filters;
		filters << "[0-9]*";
		for (const QString& pidDir : procDir.entryList(filters, QDir::Dirs))
		{
			QFile statFile(QString("/proc/%1/stat").arg(pidDir));
			if (statFile.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				QByteArray line = statFile.readLine();
				qsizetype lastCloseParenthesis = line.lastIndexOf(')');
				if (lastCloseParenthesis != -1)
				{
					QStringList statParts = QString::fromUtf8(line.mid(lastCloseParenthesis + 2)).split(' ', Qt::SkipEmptyParts);
					if (statParts.size() >= 2)
					{
						QString ppid = statParts.at(1);
						if (ppid == parentPidStr)
						{
							childrenPids << pidDir;
						}
					}
				}
			}
		}

		// 2. Analyze the children
		QString bestBinaryName;
		for (const QString& childPid : childrenPids)
		{
			QFile commFile(QString("/proc/%1/comm").arg(childPid));
			if (commFile.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				QString childName = QString::fromUtf8(commFile.readAll()).trimmed();

				// Completely ignore the Gamescope helper process
				if (childName == QLatin1String("gamescopereaper"))
				{
					continue;
				}

				// If this child has further subprocesses (e.g. bwrap -> game or bash -> game),
				// continue deeper into the tree
				QString nestedName = getGameBinaryFromGamescopePid(childPid);
				if (!nestedName.isEmpty())
				{
					return nestedName;
				}

				// If we are at the end of the tree, this is our target binary
				bestBinaryName = childName;
			}
		}

		// If no valid child binary was found,
		// read the current process's cmdline as a last resort,
		// to find arguments after "bwrap" or "gamescope", for example.
		if (bestBinaryName.isEmpty() || bestBinaryName == QLatin1String("bwrap"))
		{
			QFile cmdFile(QString("/proc/%1/cmdline").arg(parentPidStr));
			if (cmdFile.open(QIODevice::ReadOnly))
			{
				// cmdline is separated by '\0' (null bytes)
				QList<QByteArray> args = cmdFile.readAll().split('\0');
				// Look for the '--' separator, after which Gamescope or bwrap starts the game
				qsizetype dashDashIndex = args.indexOf("--");
				if (dashDashIndex != -1 && dashDashIndex + 1 < args.size())
				{
					QString cmdTarget = QString::fromUtf8(args.at(dashDashIndex + 1));
					if (!cmdTarget.isEmpty())
					{
						return QFileInfo(cmdTarget).fileName(); // Returns e.g. "glmark2"
					}
				}
			}
		}

		return bestBinaryName;
	}

	static void onDiscoveryRegistryGlobal(void* data, uint32_t id, uint32_t /*permissions*/, const char* type, uint32_t /*version*/, const struct spa_dict* props)
	{
		if (!props)
			return;

		auto* state = static_cast<DiscoveryState*>(data);

		// CASE 1: A Client object was found; store its verified system PID.
		if (strcmp(type, PW_TYPE_INTERFACE_Client) == 0)
		{
			const char* secPid = spa_dict_lookup(props, PW_KEY_SEC_PID); // "pipewire.sec.pid"
			if (secPid)
			{
				state->clientPids.insert(id, QString::fromUtf8(secPid));
			}
			return;
		}

		// CASE 2: A Node object was found; process it if it belongs to Gamescope.
		if (strcmp(type, PW_TYPE_INTERFACE_Node) != 0)
		{
			return;
		}

		const char* mediaClass = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
		if (!mediaClass)
		{
			return;
		}

		if (strcmp(mediaClass, "Stream/Output/Video") != 0 && strcmp(mediaClass, "Video/Source") != 0)
		{
			return;
		}

		const char* nodeName = spa_dict_lookup(props, PW_KEY_NODE_NAME);
		if (!nodeName || strncasecmp(nodeName, "gamescope", 9) != 0)
		{
			return;
		}

		GamescopeNodeInfo info;
		info.id = id;
		info.name = QString::fromUtf8(nodeName);

		const char* desc = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
		if (desc && *desc)
		{
			qDebug().noquote() << "[Hyperion-GS] Found Gamescope Node Description:" << desc;
			info.displayName = QString::fromUtf8(desc);
		}
		else
		{
			info.displayName = info.name;
		}

		// Resolve the PID through the client ID instead of the empty application.process.id.
		const char* clientIdStr = spa_dict_lookup(props, PW_KEY_CLIENT_ID);
		QString resolvedPid;

		if (clientIdStr)
		{
			auto clientId = static_cast<uint32_t>(std::strtoul(clientIdStr, nullptr, 10));
			// Retrieve the previously stored PID of the associated client.
			resolvedPid = state->clientPids.value(clientId);
		}

		if (!resolvedPid.isEmpty())
		{
			// Identify the game running inside this specific Gamescope instance.
			QString gameBinary = getGameBinaryFromGamescopePid(resolvedPid);
			if (!gameBinary.isEmpty())
			{
				info.displayName = gameBinary;
			}
			info.displayName += QStringLiteral(" (PID %1)").arg(resolvedPid);
		}

		// Debug-Output
		qCDebug(grabber_screen_properties).noquote() << "[Hyperion-GS] Found Running Gamescope Node | ID:" << info.id;
		qCDebug(grabber_screen_properties).noquote() << "  -> node.name:                 " << info.name;
		qCDebug(grabber_screen_properties).noquote() << "  -> resolved gamescope.pid:    " << (!resolvedPid.isEmpty() ? resolvedPid : "[Not found via client.id]");
		qCDebug(grabber_screen_properties).noquote() << "  -> node.displayName:          " << info.displayName;
		qCDebug(grabber_screen_properties).noquote() << "--------------------------------------------------";

		state->nodes.append(info);
	}

	static void onDiscoveryCoreDone(void* data, uint32_t id, int seq)
	{
		if (id != PW_ID_CORE)
		{
			return;
		}
		auto* state = static_cast<DiscoveryState*>(data);
		if (seq == state->syncSeq)
		{
			state->done = true;
			pw_thread_loop_signal(state->loop, false);
		}
	}

} // namespace

QList<GamescopeNodeInfo> PipeWireFrameSource::discoverGamescopeNodes()
{
	DiscoveryState state;
	pw_init(nullptr, nullptr);

	pw_thread_loop* loop = pw_thread_loop_new("hyperion-gs-discovery", nullptr);
	if (!loop)
	{
		pw_deinit();
		return {};
	}
	state.loop = loop;

	pw_thread_loop_lock(loop);

	pw_context* ctx = pw_context_new(pw_thread_loop_get_loop(loop), nullptr, 0);
	pw_core* core = ctx ? pw_context_connect(ctx, nullptr, 0) : nullptr;
	if (!core)
	{
		pw_thread_loop_unlock(loop);
		if (ctx)
		{
			pw_context_destroy(ctx);
		}
		pw_thread_loop_destroy(loop);
		pw_deinit();
		return {};
	}

	// Listen for core "done" events to know when all registry globals are delivered.
	pw_core_events coreEvents{};
	coreEvents.version = PW_VERSION_CORE_EVENTS;
	coreEvents.done = onDiscoveryCoreDone;
	spa_hook coreHook{};
	pw_core_add_listener(core, &coreHook, &coreEvents, &state);

	// Enumerate all registry globals and filter inside the callback.
	pw_registry* registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);
	pw_registry_events regEvents{};
	regEvents.version = PW_VERSION_REGISTRY_EVENTS;
	regEvents.global = onDiscoveryRegistryGlobal;
	spa_hook regHook{};
	pw_registry_add_listener(registry, &regHook, &regEvents, &state);

	// Sync: the done callback fires when all pending messages have been processed.
	state.syncSeq = pw_core_sync(core, PW_ID_CORE, 0);

	pw_thread_loop_start(loop);

	// Wait (up to 3 s) for the sync done signal.
	while (!state.done)
	{
		if (pw_thread_loop_timed_wait(loop, 3) != 0)
		{
			break;
		}
	}

	pw_thread_loop_unlock(loop);
	pw_thread_loop_stop(loop);

	spa_hook_remove(&regHook);
	spa_hook_remove(&coreHook);
	pw_proxy_destroy(reinterpret_cast<pw_proxy*>(registry));
	pw_core_disconnect(core);
	pw_context_destroy(ctx);
	pw_thread_loop_destroy(loop);
	pw_deinit();

	return state.nodes;
}

// ---------------------------------------------------------------------------
// startDirect — connect directly by numeric PipeWire node ID
// ---------------------------------------------------------------------------
bool PipeWireFrameSource::startDirect(uint32_t targetNodeId, const QString& targetNodeName, QString& error)
{
	qCDebug(grabber_screen_flow) << "Start direct PipeWire stream for node ID" << targetNodeId
	                             << "(" << targetNodeName << ")";
	stop();

	pw_init(nullptr, nullptr);

	Impl& s = *_impl;
	s.pwInitialized = true;
	s.failed = false;
	s.haveFormat = false;
	s.haveFrame = false;

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

	// When we have a specific numeric node ID we do NOT set PW_KEY_TARGET_OBJECT —
	// we pass the ID directly to pw_stream_connect() so no session manager is needed.
	s.stream = pw_stream_new(s.core, "hyperion-gamescope-capture",
	                         pw_properties_new(
	                             PW_KEY_MEDIA_TYPE, "Video",
	                             PW_KEY_MEDIA_CATEGORY, "Capture",
	                             PW_KEY_MEDIA_ROLE, "Screen",
	                             nullptr));

	s.events.version = PW_VERSION_STREAM_EVENTS;
	s.events.state_changed = Impl::onStateChanged;
	s.events.param_changed = Impl::onParamChanged;
	s.events.process = Impl::onProcess;
	pw_stream_add_listener(s.stream, &s.listener, &s.events, _impl.get());

	uint8_t buffer[1024];
	spa_pod_builder builder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
	const spa_pod* params[1];
	params[0] = buildGamescopeFormat(&builder);

	const int rc = pw_stream_connect(s.stream, PW_DIRECTION_INPUT,
	                                 targetNodeId,
	                                 static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS),
	                                 params, 1);
	if (rc < 0)
	{
		pw_thread_loop_unlock(s.loop);
		error = QStringLiteral("Cannot connect the PipeWire stream to node %1 (%2): %3")
		            .arg(targetNodeId)
		            .arg(targetNodeName)
		            .arg(QString::fromUtf8(strerror(-rc)));
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
	out = _impl->frame; // QByteArray is shared, no deep copy until the stream writes again
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
