#pragma once

#include <utils/Logger.h>
#include <utils/settings.h>
#include <utils/Components.h>
#include <utils/Image.h>

class Hyperion;
class QTimer;

///
/// @brief Capture Control class which is a interface to the HyperionDaemon native capture classes.
/// It controls the instance based enable/disable of capture feeds and PriorityMuxer registrations
///
class CaptureCont : public QObject
{
	Q_OBJECT
public:
	CaptureCont(Hyperion* hyperion);
	~CaptureCont();

	void setScreenCaptureEnable(bool enable);
	void setVideoCaptureEnable(bool enable);
	void setAudioCaptureEnable(bool enable);

	void stop();

private slots:
	///
	/// @brief Handle component state change of Video- (V4L/MF) and Screen capture
	/// @param component  The component from enum
	/// @param enable     The new state
	///
	void handleCompStateChangeRequest(hyperion::Components component, bool enable);

	///
	/// @brief Handle settings update from Hyperion Settingsmanager emit or this constructor
	/// @param type   settingyType from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(settings::type type, const QJsonDocument& config);

	///
	/// @brief forward screen image
	/// @param image  The image
	///
	void handleScreenImage(const QString& name, const Image<ColorRgb>& image);

	///
	/// @brief forward video (v4l, MF) image
	/// @param image  The image
	///
	void handleVideoImage(const QString& name, const Image<ColorRgb> & image);

	///
	/// @brief forward audio image
	/// @param image  The image
	///
	void handleAudioImage(const QString& name, const Image<ColorRgb>& image);

	///
	/// @brief Sets the video source to inactive
	///
	void onVideoIsInactive();

	///
	/// @brief Sets the screen source to inactive
	///
	void onScreenIsInactive();

	///
	/// @brief Sets the audio source to inactive
	///
	void onAudioIsInactive();


private:
	/// Hyperion instance
	Hyperion* _hyperion;

	/// Reflect state of screen capture and prio
	bool _screenCaptureEnabled;
	int _screenCapturePriority;
	QString _screenCaptureName;
	QTimer* _screenCaptureInactiveTimer;

	/// Reflect state of video capture and prio
	bool _videoCaptureEnabled;
	int _videoCapturePriority;
	QString _videoCaptureName;
	QTimer* _videoInactiveTimer;

	/// Reflect state of audio capture and prio
	bool _audioCaptureEnabled;
	int _audioCapturePriority;
	QString _audioCaptureName;
	QTimer* _audioCaptureInactiveTimer;
};
