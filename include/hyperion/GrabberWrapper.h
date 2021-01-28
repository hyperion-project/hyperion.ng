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

#include <grabber/GrabberType.h>

class Grabber;
class GlobalSignals;
class QTimer;

///
/// This class will be inherited by GrabberWrappers which contains the real capture interface
///
class GrabberWrapper : public QObject
{
	Q_OBJECT
public:
	GrabberWrapper(const QString& grabberName, Grabber * ggrabber,int updateRate_Hz = DEFAULT_RATE_HZ);

	~GrabberWrapper() override;

	static GrabberWrapper* instance;
	static GrabberWrapper* getInstance(){ return instance; }

	static const int DEFAULT_RATE_HZ;
	static const int DEFAULT_MIN_GRAB_RATE_HZ;
	static const int DEFAULT_MAX_GRAB_RATE_HZ;
	static const int DEFAULT_PIXELDECIMATION;

	static QMap<int, QString> GRABBER_SYS_CLIENTS;
	static QMap<int, QString> GRABBER_V4L_CLIENTS;
	static QMap<int, QString> GRABBER_AUDIO_CLIENTS;
	static bool GLOBAL_GRABBER_SYS_ENABLE;
	static bool GLOBAL_GRABBER_V4L_ENABLE;
	static bool GLOBAL_GRABBER_AUDIO_ENABLE;

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
	/// @param type Filter for a given grabber type
	/// @return Active grabbers
	///
	virtual QStringList getActive(int inst, GrabberTypeFilter type = GrabberTypeFilter::ALL) const;

	bool getSysGrabberState() const { return GLOBAL_GRABBER_SYS_ENABLE; }
	void setSysGrabberState(bool sysGrabberState){ GLOBAL_GRABBER_SYS_ENABLE = sysGrabberState; }
	bool getV4lGrabberState() const { return GLOBAL_GRABBER_V4L_ENABLE; }
	void setV4lGrabberState(bool v4lGrabberState){ GLOBAL_GRABBER_V4L_ENABLE = v4lGrabberState; }
	bool getAudioGrabberState() const { return GLOBAL_GRABBER_AUDIO_ENABLE; }
	void setAudioGrabberState(bool audioGrabberState) { GLOBAL_GRABBER_AUDIO_ENABLE = audioGrabberState; }

	static QStringList availableGrabbers(GrabberTypeFilter type = GrabberTypeFilter::ALL);

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
	virtual void setFlipMode(const QString &flipMode);

	///
	/// Set the crop values
	/// @param  cropLeft    Left pixel crop
	/// @param  cropRight   Right pixel crop
	/// @param  cropTop     Top pixel crop
	/// @param  cropBottom  Bottom pixel crop
	///
	virtual void setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom);

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

	/// @brief Update Update capture rate
	/// @param type   interval between frames in milliseconds
	///
	void updateTimer(int interval);


	QString _grabberName;

	/// The Logger instance
	Logger * _log;

	/// The timer for generating events with the specified update rate
	QTimer* _timer;

	/// The calculated update rate [ms]
	int _updateInterval_ms;

	Grabber *_ggrabber;

	/// The image used for grabbing frames
	Image<ColorRgb> _image;
};
