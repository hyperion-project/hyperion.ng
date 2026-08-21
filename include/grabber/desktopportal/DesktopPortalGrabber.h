#pragma once

#include <hyperion/Grabber.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVariant>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>
#pragma GCC diagnostic pop

///
/// The DesktopPortalGrabber captures a normal (non-gamescope) Wayland desktop session via
/// xdg-desktop-portal's ScreenCast interface - the standard, permission-gated mechanism every
/// other Wayland screen-capture consumer (browsers, OBS, etc.) uses. This is the counterpart to
/// GamescopeGrabber: gamescope itself has no portal ScreenCast backend, so that grabber bypasses
/// the portal entirely; a normal desktop session (KDE Plasma, GNOME, ...) has no equivalent
/// unmediated capture path, so this grabber goes through the portal instead.
///
/// Negotiation (CreateSession -> SelectSources -> Start -> OpenPipeWireRemote, all over D-Bus)
/// happens on a persistent background thread, same shape as GamescopeGrabber's retry loop:
/// negotiate -> stream while connected -> on disconnect, close the portal session and
/// renegotiate. A restore_token saved to disk lets every renegotiation after the first reuse the
/// original permission grant (persist_mode = persistent-until-revoked) without re-prompting.
/// isAvailable() is always true once constructed; grabFrame() returns -1 whenever nothing is
/// currently connected - see GamescopeGrabber's header for why this lifecycle shape is used
/// instead of a one-shot isAvailable() check.
///
class DesktopPortalGrabber : public Grabber
{
public:
	explicit DesktopPortalGrabber(int cropLeft = 0, int cropRight = 0, int cropTop = 0, int cropBottom = 0);
	~DesktopPortalGrabber() override;

	int grabFrame(Image<ColorRgb>& image, bool forceUpdate = false) override;

	bool setupScreen() override { return true; }
	QSize getScreenSize() const override { return QSize(_width, _height); }

	///
	/// @brief Reports this grabber to the "Device discovered" UI (Configuration -> Capturing
	/// Hardware -> Screen Capture). Not a base-class override, same ad-hoc contract as every
	/// other grabber's discover() (see JsonInfo::discoverGrabber).
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

	// Unlike GamescopeGrabber, there's no registry-based node discovery here (the portal hands
	// back the target node id directly in Start()'s response), so no pw_core_sync/"done"
	// handler is needed - only the error callback is wired up, for diagnostics.
	static void onCoreError(void* userdata, uint32_t id, int seq, int res, const char* message);
	static void onStreamParamChanged(void* userdata, uint32_t id, const struct spa_pod* param);
	static void onStreamProcess(void* userdata);
	static void onStreamStateChanged(void* userdata, enum pw_stream_state old, enum pw_stream_state state, const char* error);

	void portalThreadMain();
	bool connectCore();
	void runStream();
	void waitBeforeRetry();
	void stop();
	void clearDmaBufMappings();

	// Portal (D-Bus) negotiation - implemented in DesktopPortalGrabber.cpp using a private
	// QObject-based request/response watcher (see the .moc-included helper there). Returns
	// false if negotiation didn't produce a usable session (permission denied, portal error,
	// etc.) - the caller just retries later, same as gamescope's "node not found yet".
	bool negotiatePortalSession();
	void closePortalSession();

	struct PortalResponse
	{
		bool ok{false};
		uint32_t code{2};
		QVariantMap results;
	};
	PortalResponse callPortalRequest(const QString& method, const QVariantList& args, QVariantMap options);

	QString restoreTokenPath() const;
	QString readRestoreToken() const;
	void writeRestoreToken(const QString& token) const;

	std::optional<std::thread> _thread;
	pw_main_loop* _loop{nullptr};
	pw_context* _context{nullptr};
	pw_core* _core{nullptr};
	pw_stream* _stream{nullptr};
	spa_hook _coreListener{};
	pw_core_events _coreEvents{};
	spa_video_info _format{};

	// Portal session state, populated by negotiatePortalSession() and consumed by runStream().
	QString _sessionHandle;
	int _pipewireFd{-1};
	uint32_t _portalNodeId{0};

	std::atomic<bool> _connected{false};
	std::atomic<bool> _stopping{false};
	std::mutex _retryMutex;
	std::condition_variable _retryCv;

	std::mutex _bufferMutex;
	FrameBuffer _frontBuffer;

	// DMA-BUF fds are stable for the lifetime of a stream connection - PipeWire cycles through
	// a small, fixed pool of buffers rather than allocating a fresh one per frame - so mmap()
	// each fd once and reuse the mapping instead of mmap()/munmap() on every single frame (the
	// latter measured at ~110%+ CPU for a full-desktop-resolution stream; see FINDINGS.md).
	// Cleared on every disconnect/reconnect since a fresh negotiation gets a fresh buffer pool.
	std::unordered_map<int, std::pair<void*, size_t>> _dmaBufMappings;
};
