#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

#include <utils/Logger.h>
#include <utils/Components.h>
#include <hyperion/Hyperion.h>
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/VideoMode.h>
#include <utils/settings.h>

class Grabber;
class DispmanxFrameGrabber;
class QTimer;

///
/// This class will be inherted by FramebufferWrapper and others which contains the real capture interface
///
class GrabberWrapper : public QObject
{
	Q_OBJECT
public:
	GrabberWrapper(QString grabberName, Grabber * ggrabber, unsigned width, unsigned height, const unsigned updateRate_Hz);

	virtual ~GrabberWrapper();

	///
	/// Starts the grabber wich produces led values with the specified update rate
	///
	virtual bool start();

	///
	/// Stop grabber
	///
	virtual void stop();

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
			emit systemImage(_image);
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
	void systemImage(const Image<ColorRgb>& image);

protected:

	QString _grabberName;

	/// Pointer to Hyperion for writing led values
	Hyperion * _hyperion;

	/// The timer for generating events with the specified update rate
	QTimer* _timer;

	/// The calced update rate [ms]
	int _updateInterval_ms;

	/// The Logger instance
	Logger * _log;

	// forwarding enabled
	bool _forward;

	Grabber *_ggrabber;

	/// The image used for grabbing frames
	Image<ColorRgb> _image;
};
