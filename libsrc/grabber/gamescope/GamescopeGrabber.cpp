#include <grabber/gamescope/GamescopeGrabber.h>

#include <chrono>
#include <cstring>
#include <sys/mman.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include <spa/utils/dict.h>
#pragma GCC diagnostic pop

namespace {
	constexpr auto RETRY_INTERVAL = std::chrono::milliseconds(2000);
}

GamescopeGrabber::GamescopeGrabber(int cropLeft, int cropRight, int cropTop, int cropBottom)
	: Grabber("GAMESCOPE", cropLeft, cropRight, cropTop, cropBottom)
{
	// Always available: whether a gamescope session is currently running is a
	// transient condition the background thread keeps retrying against, not a
	// platform capability decided once at startup.
	_isAvailable = true;

	_thread.emplace(&GamescopeGrabber::pipewireThreadMain, this);
}

GamescopeGrabber::~GamescopeGrabber()
{
	stop();
}

void GamescopeGrabber::stop()
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

int GamescopeGrabber::grabFrame(Image<ColorRgb>& image, bool /*forceUpdate*/)
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

QJsonObject GamescopeGrabber::discover(const QJsonObject& /*params*/)
{
	QJsonObject inputsDiscovered;
	inputsDiscovered["device"] = "gamescope";
	inputsDiscovered["device_name"] = "Gamescope";
	inputsDiscovered["type"] = "screen";

	// Reported here purely so this grabber shows up as a selectable entry in the
	// "Device discovered" UI - there's only ever one thing to capture (whichever
	// gamescope session happens to be running, if any), not multiple screens/cards
	// to enumerate the way X11/DRM do. Report a placeholder resolution rather than
	// 0x0 when no session is currently connected.
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
	input["name"] = "Gamescope";
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

void GamescopeGrabber::onCoreError(void* userdata, uint32_t id, int seq, int res, const char* message)
{
	auto* self = static_cast<GamescopeGrabber*>(userdata);
	Error(self->_log, "[pipewire] Error id: %d seq: %d res: %d (%s) %s", id, seq, res, strerror(res), message);
}

void GamescopeGrabber::onCoreDone(void* userdata, uint32_t id, int seq)
{
	auto* self = static_cast<GamescopeGrabber*>(userdata);

	// Gamescope node discovery: the registry has advertised all currently available
	// globals by the time this sync's "done" fires, so it's safe to stop the loop and
	// check whether "gamescope" showed up.
	if (self->_discoveryMode && id == PW_ID_CORE && seq == self->_discoverySyncSeq)
	{
		pw_main_loop_quit(self->_loop);
	}
}

void GamescopeGrabber::onRegistryGlobal(void* userdata, uint32_t id, uint32_t /*permissions*/, const char* type, uint32_t /*version*/, const struct spa_dict* props)
{
	auto* self = static_cast<GamescopeGrabber*>(userdata);

	if (self->_gamescopeNodeId != 0 || props == nullptr)
	{
		return;
	}

	if (std::string(type) != PW_TYPE_INTERFACE_Node)
	{
		return;
	}

	const char* nodeName = spa_dict_lookup(props, PW_KEY_NODE_NAME);
	if (nodeName != nullptr && std::string(nodeName) == "gamescope")
	{
		self->_gamescopeNodeId = id;
	}
}

void GamescopeGrabber::onStreamStateChanged(void* userdata, enum pw_stream_state /*old*/, enum pw_stream_state state, const char* error)
{
	auto* self = static_cast<GamescopeGrabber*>(userdata);

	if (state == PW_STREAM_STATE_ERROR || state == PW_STREAM_STATE_UNCONNECTED)
	{
		if (error != nullptr)
		{
			Warning(self->_log, "[pipewire] gamescope stream ended: %s", error);
		}

		self->_connected = false;
		self->_width = 0;
		self->_height = 0;

		{
			std::lock_guard<std::mutex> lock(self->_bufferMutex);
			self->_frontBuffer.valid = false;
		}

		// Unblock runStream()'s pw_main_loop_run() so the outer retry loop can
		// go back to looking for a fresh "gamescope" node.
		if (self->_loop)
		{
			pw_main_loop_quit(self->_loop);
		}
	}
}

void GamescopeGrabber::onStreamParamChanged(void* userdata, uint32_t id, const struct spa_pod* param)
{
	auto* self = static_cast<GamescopeGrabber*>(userdata);

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

	self->_width = static_cast<int>(self->_format.info.raw.size.width);
	self->_height = static_cast<int>(self->_format.info.raw.size.height);
	self->_connected = true;
}

void GamescopeGrabber::onStreamProcess(void* userdata)
{
	auto* self = static_cast<GamescopeGrabber*>(userdata);

	struct pw_buffer* pwBuffer = pw_stream_dequeue_buffer(self->_stream);
	if (pwBuffer == nullptr)
	{
		return;
	}

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

	// Real GPU-rendered content (an actual game via gamescope, as opposed to a
	// simple test app) can hand back DMA-BUF buffers that aren't pre-mapped
	// with PROT_READ even though PW_STREAM_FLAG_MAP_BUFFERS is set - data can
	// legitimately be null here, not just garbage. Map the fd ourselves for
	// the duration of this frame as a defensive fallback (the stream only
	// ever negotiates the LINEAR modifier for DMA-BUF, so a plain mmap is
	// enough - no tiled/compressed layout to decode).
	void* readPtr = spaBuffer->datas[0].data;
	void* localMap = MAP_FAILED;
	size_t localMapSize = 0;
	const bool needRemap = readPtr == nullptr
		&& (spaBuffer->datas[0].type == SPA_DATA_MemFd || spaBuffer->datas[0].type == SPA_DATA_DmaBuf)
		&& spaBuffer->datas[0].fd >= 0;

	if (needRemap)
	{
		localMapSize = static_cast<size_t>(spaBuffer->datas[0].maxsize) + chunk->offset;
		localMap = mmap(nullptr, localMapSize, PROT_READ, MAP_SHARED, static_cast<int>(spaBuffer->datas[0].fd), 0);
		if (localMap != MAP_FAILED)
		{
			readPtr = localMap;
		}
	}

	if (readPtr == nullptr)
	{
		pw_stream_queue_buffer(self->_stream, pwBuffer);
		return;
	}

	const auto* src = static_cast<const uint8_t*>(readPtr) + chunk->offset;
	const size_t byteCount = stride * static_cast<size_t>(height);

	{
		std::lock_guard<std::mutex> lock(self->_bufferMutex);
		self->_frontBuffer.data.assign(src, src + byteCount);
		self->_frontBuffer.width = width;
		self->_frontBuffer.height = height;
		self->_frontBuffer.stride = stride;
		self->_frontBuffer.pixelFormat = pixelFormat;
		self->_frontBuffer.valid = true;
	}

	if (localMap != MAP_FAILED)
	{
		munmap(localMap, localMapSize);
	}

	pw_stream_queue_buffer(self->_stream, pwBuffer);
}

bool GamescopeGrabber::connectCore()
{
	// _coreEvents is a member (not a local) because Pipewire's listener registration
	// stores a pointer to it rather than copying it - it must stay alive for as long
	// as the core connection exists, well past this function returning.
	_coreEvents.version = PW_VERSION_CORE_EVENTS;
	_coreEvents.done = &GamescopeGrabber::onCoreDone;
	_coreEvents.error = &GamescopeGrabber::onCoreError;

	_loop = pw_main_loop_new(nullptr);
	_context = pw_context_new(pw_main_loop_get_loop(_loop), nullptr, 0);
	_core = pw_context_connect(_context, nullptr, 0);

	if (_core == nullptr)
	{
		Error(_log, "[pipewire] Failed to connect to Pipewire");
		pw_context_destroy(_context);
		_context = nullptr;
		pw_main_loop_destroy(_loop);
		_loop = nullptr;
		return false;
	}

	pw_core_add_listener(_core, &_coreListener, &_coreEvents, this);
	return true;
}

void GamescopeGrabber::discoverGamescopeNode()
{
	_gamescopeNodeId = 0;

	pw_registry_events registryEvents{};
	registryEvents.version = PW_VERSION_REGISTRY_EVENTS;
	registryEvents.global = &GamescopeGrabber::onRegistryGlobal;

	pw_registry* registry = pw_core_get_registry(_core, PW_VERSION_REGISTRY, 0);
	spa_hook registryListener{};
	pw_registry_add_listener(registry, &registryListener, &registryEvents, this);

	_discoveryMode = true;
	_discoverySyncSeq = pw_core_sync(_core, PW_ID_CORE, 0);
	pw_main_loop_run(_loop);
	_discoveryMode = false;

	spa_hook_remove(&registryListener);
	pw_proxy_destroy(reinterpret_cast<pw_proxy*>(registry));
}

void GamescopeGrabber::runStream()
{
	pw_stream_events streamEvents{};
	streamEvents.version = PW_VERSION_STREAM_EVENTS;
	streamEvents.param_changed = &GamescopeGrabber::onStreamParamChanged;
	streamEvents.process = &GamescopeGrabber::onStreamProcess;
	streamEvents.state_changed = &GamescopeGrabber::onStreamStateChanged;

	pw_properties* props = pw_properties_new(
		PW_KEY_MEDIA_TYPE, "Video",
		PW_KEY_MEDIA_CATEGORY, "Capture",
		PW_KEY_MEDIA_ROLE, "Screen",
		nullptr
	);

	_stream = pw_stream_new_simple(pw_main_loop_get_loop(_loop), "HyperionGamescopeStream", props, &streamEvents, this);

	uint8_t buffer[1024];
	spa_pod_builder podBuilder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

	spa_rectangle rectMin = SPA_RECTANGLE(1, 1);
	spa_rectangle rectDefault = SPA_RECTANGLE(320, 240);
	spa_rectangle rectMax = SPA_RECTANGLE(4096, 4096);

	spa_fraction rateMin = SPA_FRACTION(0, 1);
	spa_fraction rateDefault = SPA_FRACTION(25, 1);
	spa_fraction rateMax = SPA_FRACTION(1000, 1);

	const spa_pod* params[1] = {
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
			// Real GPU-rendered content (an actual game, as opposed to a simple
			// test app) gets composited into GPU-native DMA-BUF buffers, which
			// may use a tiled/compressed modifier this code has no way to
			// decode. Explicitly requiring the LINEAR modifier (value 0 - see
			// DRM_FORMAT_MOD_LINEAR in drm_fourcc.h, not included here to avoid
			// a new libdrm dependency for one well-known constant) makes
			// gamescope either hand back a plain CPU-readable buffer or fail to
			// negotiate a format cleanly, instead of negotiating successfully
			// and then failing later at the buffer-import step.
			SPA_POD_Propf(SPA_FORMAT_VIDEO_modifier, SPA_POD_PROP_FLAG_MANDATORY, SPA_POD_CHOICE_ENUM_Long(2, 0L, 0L))
		))
	};

	pw_stream_connect(_stream, PW_DIRECTION_INPUT, _gamescopeNodeId,
		static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS),
		params, 1);

	// Blocks until either the stream errors/disconnects (onStreamStateChanged quits
	// the loop so we can retry discovery) or stop() quits it for good.
	pw_main_loop_run(_loop);

	pw_stream_disconnect(_stream);
	pw_stream_destroy(_stream);
	_stream = nullptr;

	_connected = false;
}

void GamescopeGrabber::waitBeforeRetry()
{
	std::unique_lock<std::mutex> lock(_retryMutex);
	_retryCv.wait_for(lock, RETRY_INTERVAL, [this] { return _stopping.load(); });
}

void GamescopeGrabber::pipewireThreadMain()
{
	pw_init(nullptr, nullptr);

	if (!connectCore())
	{
		pw_deinit();
		return;
	}

	while (!_stopping.load())
	{
		discoverGamescopeNode();

		if (_stopping.load())
		{
			break;
		}

		if (_gamescopeNodeId == 0)
		{
			waitBeforeRetry();
			continue;
		}

		runStream();

		if (_stopping.load())
		{
			break;
		}

		waitBeforeRetry();
	}

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

	pw_deinit();
}
