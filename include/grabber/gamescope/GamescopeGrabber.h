#pragma once

#include <hyperion/Grabber.h>

#include <QJsonArray>
#include <QJsonObject>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>
#pragma GCC diagnostic pop

///
/// The GamescopeGrabber captures gamescope's composited output directly, bypassing
/// xdg-desktop-portal's ScreenCast. Gamescope sets XDG_SESSION_TYPE=x11 for its child
/// processes (for legacy X11 game compatibility) even though it's a Wayland compositor,
/// and doesn't run a portal ScreenCast backend even when targeted directly. It does,
/// however, expose its composited output as a plain (non-portal-gated) Pipewire node
/// named literally "gamescope" - the same mechanism obs-vkcapture/obs-gstreamer use to
/// stream Steam Deck Game Mode. This grabber connects to the local Pipewire socket and
/// captures that node directly, with no Wayland protocol or portal permission dialog
/// involved.
///
/// A gamescope session can start and end at any time relative to this grabber's own
/// lifetime, so it runs a persistent background thread that keeps looking for the
/// "gamescope" node, connects and streams while it exists, and goes back to looking
/// whenever the stream disconnects (the session ended) or was never found in the first
/// place. isAvailable() always reports true once constructed - whether a gamescope
/// session is *currently* live is a transient condition expressed through grabFrame()
/// returning -1, not through isAvailable() flipping, matching how Hyperion's other
/// grabbers signal a transient per-frame failure.
///
/// This is intentionally gamescope-specific: it is not a general Wayland desktop
/// capture backend. Outside a gamescope session no "gamescope" node exists, discovery
/// simply never succeeds, and grabFrame() keeps returning -1.
///
class GamescopeGrabber : public Grabber
{
public:
	explicit GamescopeGrabber(int cropLeft = 0, int cropRight = 0, int cropTop = 0, int cropBottom = 0);
	~GamescopeGrabber() override;

	int grabFrame(Image<ColorRgb>& image, bool forceUpdate = false) override;

	bool setupScreen() override { return true; }
	QSize getScreenSize() const override { return QSize(_width, _height); }

	///
	/// @brief Reports this grabber to the "Device discovered" UI (Configuration ->
	/// Capturing Hardware -> Screen Capture). Not a base-class override - discovery
	/// dispatch is compile-time templated per grabber type (see JsonInfo::discoverGrabber),
	/// matching the same ad-hoc contract every other grabber's discover() follows.
	///
	QJsonObject discover(const QJsonObject& params);

private:
	struct FrameBuffer
	{
		std::vector<uint8_t> data;
		int width{0};
		int height{0};
		size_t stride{0};
		PixelFormat pixelFormat{PixelFormat::NO_CHANGE};
		bool valid{false};
	};

	static void onCoreDone(void* userdata, uint32_t id, int seq);
	static void onCoreError(void* userdata, uint32_t id, int seq, int res, const char* message);
	static void onRegistryGlobal(void* userdata, uint32_t id, uint32_t permissions, const char* type, uint32_t version, const struct spa_dict* props);
	static void onStreamParamChanged(void* userdata, uint32_t id, const struct spa_pod* param);
	static void onStreamProcess(void* userdata);
	static void onStreamStateChanged(void* userdata, enum pw_stream_state old, enum pw_stream_state state, const char* error);

	void pipewireThreadMain();
	bool connectCore();
	void discoverGamescopeNode();
	void runStream();
	void waitBeforeRetry();
	void stop();

	std::optional<std::thread> _thread;
	pw_main_loop* _loop{nullptr};
	pw_context* _context{nullptr};
	pw_core* _core{nullptr};
	pw_stream* _stream{nullptr};
	spa_hook _coreListener{};
	pw_core_events _coreEvents{};
	spa_video_info _format{};

	bool _discoveryMode{false};
	int _discoverySyncSeq{0};
	uint32_t _gamescopeNodeId{0};

	std::atomic<bool> _connected{false};
	std::atomic<bool> _stopping{false};
	std::mutex _retryMutex;
	std::condition_variable _retryCv;

	std::mutex _bufferMutex;
	FrameBuffer _frontBuffer;
};
