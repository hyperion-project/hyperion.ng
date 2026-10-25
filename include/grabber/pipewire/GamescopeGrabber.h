#pragma once

#include <QElapsedTimer>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <memory>

#include <utils/ColorRgb.h>
#include <hyperion/Grabber.h>

class PipeWireFrameSource;

///
/// @brief Screen capture for Gamescope sessions via a direct PipeWire connection.
///
/// Gamescope registers its output as a PipeWire video producer node (node.name
/// "gamescope") without requiring the xdg-desktop-portal handshake. This grabber
/// connects to the running PipeWire daemon in the user session and targets that
/// node by name, so no permission dialog is shown.
///
/// The PipeWire frame-consumer logic is kept inside PipeWireFrameSource so it
/// is shared with WaylandGrabber without duplication.
///
/// The grabber self-heals: when Gamescope is not yet running (or exits between
/// games) the stream fails, and grabFrame() schedules a reconnect with an
/// exponential back-off so the daemon keeps operating without manual intervention.
///
class GamescopeGrabber : public Grabber
{
	Q_OBJECT

public:
	explicit GamescopeGrabber(int cropLeft = 0, int cropRight = 0,
	                          int cropTop = 0, int cropBottom = 0);
	~GamescopeGrabber() override;

	///
	/// @brief True when a PipeWire session is reachable in the current environment.
	///
	bool isAvailable(bool logError = false) override;

	///
	/// @brief Connects to the Gamescope PipeWire node. Frames arrive asynchronously;
	///        grabFrame() returns -1 until the first frame is ready.
	///
	bool open();
	bool close();

	///
	/// @brief Converts the most-recent frame received from Gamescope.
	///
	int grabFrame(Image<ColorRgb>& image, bool forceUpdate = false) override;

	bool setWidthHeight(int /*width*/, int /*height*/) override { return true; }

	///
	/// @brief The stream is sampled no more often than frames are consumed.
	///
	bool setFramerate(int fps) override;

	///
	/// @brief Resetting the enabled state also clears any retry back-off,
	///        so the grabber reconnects immediately on the next grabFrame() call.
	///
	void setEnabled(bool enable) override;

	///
	/// @brief Crop in source pixels; validated against the real frame size.
	///
	void setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom) override;

	///
	/// @brief Enumerate Gamescope PipeWire nodes visible in the current session.
	///        Returns a discover-style JSON object compatible with the Hyperion
	///        web-configuration UI (video_inputs list).
	///
	QJsonObject discover(const QJsonObject& params);

private:
	void scheduleRetry(int delayMs);
	void updateImageSize(int frameWidth, int frameHeight);
	int  frameIntervalMs() const;

	std::unique_ptr<PipeWireFrameSource> _source;

	QElapsedTimer _retryTimer;
	int           _retryDelayMs = 0;

	// Physical dimensions of the most-recent frame
	int _sourceWidth  = 0;
	int _sourceHeight = 0;

	// PipeWire target node name set in open(); stored so open() is idempotent.
	QString _targetNode;
};

