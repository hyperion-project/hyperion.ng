#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QStringList>
#include <QMultiMap>

#include <utils/Logger.h>
#include <utils/Components.h>
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/VideoMode.h>
#include <utils/PixelFormat.h>
#include <utils/settings.h>
#include <utils/VideoStandard.h>

class Grabber;
class GlobalSignals;
class QTimer;

/// Map of Hyperion instances with grabber name that requested screen capture
static QMap<int, QString> GRABBER_SYS_CLIENTS;
static QMap<int, QString> GRABBER_V4L_CLIENTS;

///
/// This class will be inherted by FramebufferWrapper and others which contains the real capture interface
///
class GrabberWrapper : public QObject
{
	Q_OBJECT
public:
	GrabberWrapper(const QString& grabberName, Grabber * ggrabber, unsigned width, unsigned height, unsigned updateRate_Hz = 0);

	~GrabberWrapper() override;

	static GrabberWrapper* instance;
	static GrabberWrapper* getInstance(){ return instance; }

	///
	/// Starts the grabber which produces led values with the specified update rate
	///
	virtual bool start();

	///
	/// Starts maybe the grabber which produces led values with the specified update rate
	///
	virtual void tryStart();

	///
	/// Stop grabber
	///
	virtual void stop();

	///
	/// Check if grabber is active
	///
	virtual bool isActive() const;

	///
	/// @brief Get active grabber name
	/// @param hyperionInd The instance index
	/// @return Active grabbers
	///
	virtual QStringList getActive(int inst) const;

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
	virtual void setVideoMode(VideoMode videoMode);

	///
	/// Set the Flip mode
	/// @param flipMode The new flip mode
	///
	virtual void setFlipMode(QString flipMode);

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
	/// @param type   settingsType from enum
	/// @param config configuration object
	///
	virtual void handleSettingsUpdate(settings::type type, const QJsonDocument& config);

signals:
	///
	/// @brief Emit the final processed image
	///
	void systemImage(const QString& name, const Image<ColorRgb>& image);

private slots:
	/// @brief Handle a source request event from Hyperion.
	/// Will start and stop grabber based on active listeners count
	void handleSourceRequest(hyperion::Components component, int hyperionInd, bool listen);

	///
	/// @brief Update Update capture rate
	/// @param type   interval between frames in millisecons
	///
	void updateTimer(int interval);

protected:

	///
/// @brief Opens the input device.
///
/// @return True, on success (i.e. device is ready)
///
	virtual bool open() { return true; }

	///
	/// @brief Closes the input device.
	///
	/// @return True on success (i.e. device is closed)
	///
	virtual bool close() { return true; }


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
