#pragma once

#include <QElapsedTimer>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <memory>

#include <utils/ColorRgb.h>
#include <hyperion/Grabber.h>

class PortalScreenCast;
class PipeWireFrameSource;

///
/// @brief Screen capture on Wayland through the xdg-desktop-portal ScreenCast interface.
///
/// The portal negotiates a PipeWire stream. The screen is chosen in a compositor dialog, once:
/// the portal's restore token is stored, so later starts reuse the choice without asking.
///
class WaylandGrabber : public Grabber
{
	Q_OBJECT

public:
	explicit WaylandGrabber(int cropLeft = 0, int cropRight = 0, int cropTop = 0, int cropBottom = 0);
	~WaylandGrabber() override;

	///
	/// @brief True when running in a Wayland session inside a GUI application.
	///
	bool isAvailable(bool logError = false) override;

	///
	/// @brief Starts the portal session. Asynchronous: frames arrive once the portal answered
	/// (after the user confirmed the dialog, on the first run), until then grabFrame() returns -1.
	///
	bool open();
	bool close();

	///
	/// @brief Converts the most recent frame received from the compositor.
	///
	int grabFrame(Image<ColorRgb>& image, bool forceUpdate = false) override;

	bool setWidthHeight(int /*width*/, int /*height*/) override { return true; }

	///
	/// @brief The stream is copied no more often than frames are consumed.
	///
	bool setFramerate(int fps) override;

	///
	/// @brief Enabling the grabber again also forgets a dismissed dialog and any retry backoff.
	///
	void setEnabled(bool enable) override;

	///
	/// @brief Crop in source pixels; validated against the real frame size, not the decimated one.
	///
	void setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom) override;

	///
	/// @brief Describe the capture source (does not open a portal session).
	///
	QJsonObject discover(const QJsonObject& params);

private slots:
	void onPortalStarted(int pipewireFd, quint32 nodeId);
	void onPortalFailed(const QString& reason, bool userCancelled);

private:
	void updateImageSize(int frameWidth, int frameHeight);
	void releaseSession();
	void teardownAndScheduleRetry(bool userCancelled);
	int frameIntervalMs() const;
	static QString stateFile(const QString& name);

	std::unique_ptr<PortalScreenCast> _portal;
	std::unique_ptr<PipeWireFrameSource> _source;
	QElapsedTimer _retryTimer;
	int _retryDelayMs = 0;
	bool _userCancelled = false;
	int _sourceWidth = 0;
	int _sourceHeight = 0;
};
