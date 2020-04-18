#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QStringList>

#include <utils/Logger.h>
#include <utils/Components.h>
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/VideoMode.h>
#include <utils/settings.h>

class Grabber;
class GlobalSignals;
class QTimer;

/// List of Hyperion instances that requested screen capt
static QList<int> GRABBER_SYS_CLIENTS;
static QList<int> GRABBER_V4L_CLIENTS;

///
/// This class will be inherted by FramebufferWrapper and others which contains the real capture interface
///
class GrabberWrapper : public QObject
{
	Q_OBJECT
public:
	GrabberWrapper(QString grabberName, Grabber * ggrabber, unsigned width, unsigned height, const unsigned updateRate_Hz = 0);

	virtual ~GrabberWrapper();

	static GrabberWrapper* instance;
	static GrabberWrapper* getInstance(){ return instance; }

	///
	/// Starts the grabber wich produces led values with the specified update rate
	///
	virtual bool start();

	///
	/// Starts maybe the grabber wich produces led values with the specified update rate
	///
	virtual void tryStart();

	///
	/// Stop grabber
	///
	virtual void stop();

	///
	/// @brief Get a list of all available V4L devices
	/// @return List of all available V4L devices on success else empty List
	///
	virtual QStringList getV4L2devices();

	///
	/// @brief Get the V4L device name
	/// @param devicePath The device path
	/// @return The name of the V4L device on success else empty String
	///
	virtual QString getV4L2deviceName(QString devicePath);

	///
	/// @brief Get a list of supported device resolutions
	/// @param devicePath The device path
	/// @return List of resolutions on success else empty List
	///
	virtual QStringList getResolutions(QString devicePath);

	///
	/// @brief Get a list of supported device framerates
	/// @param devicePath The device path
	/// @return List of framerates on success else empty List
	///
	virtual QStringList getFramerates(QString devicePath);

	static QStringList availableGrabbers();

public:
	template <typename Grabber_T>
	bool transferFrame(Grabber_T &grabber)
	{
		unsigned w = grabber.getImageWidth();
		unsigned h = grabber.getImageHeight();
		if ( _image.width() != w || _image.height() != h)
		{
			_image.resize(w, h);
		}

		int ret = grabber.grabFrame(_image);
		if (ret >= 0)
		{
			emit systemImage(_grabberName, _image);
			return true;
		}
		return false;
	}

public slots:
	///
	/// virtual method, should perform single frame grab and computes the led-colors
	///
	virtual void action() = 0;

	///
	/// Set the video mode (2D/3D)
	/// @param[in] mode The new video mode
	///
	virtual void setVideoMode(const VideoMode& videoMode);

	///
	/// Set the crop values
	/// @param  cropLeft    Left pixel crop
	/// @param  cropRight   Right pixel crop
	/// @param  cropTop     Top pixel crop
	/// @param  cropBottom  Bottom pixel crop
	///
	virtual void setCropping(unsigned cropLeft, unsigned cropRight, unsigned cropTop, unsigned cropBottom);

	///
	/// @brief Handle settings update from HyperionDaemon Settingsmanager emit
	/// @param type   settingyType from enum
	/// @param config configuration object
	///
	virtual void handleSettingsUpdate(const settings::type& type, const QJsonDocument& config);

signals:
	///
	/// @brief Emit the final processed image
	///
	void systemImage(const QString& name, const Image<ColorRgb>& image);

private slots:
	/// @brief Handle a source request event from Hyperion.
	/// Will start and stop grabber based on active listeners count
	void handleSourceRequest(const hyperion::Components& component, const int hyperionInd, const bool listen);

protected:
	QString _grabberName;

	/// The timer for generating events with the specified update rate
	QTimer* _timer;

	/// The calced update rate [ms]
	int _updateInterval_ms;

	/// The Logger instance
	Logger * _log;

	Grabber *_ggrabber;

	/// The image used for grabbing frames
	Image<ColorRgb> _image;
};
