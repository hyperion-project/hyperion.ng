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
	/// @brief Get a list of all available devices
	/// @return List of all available devices on success else empty List
	///
	virtual QStringList getDevices() const;

	///
	/// @brief Get the device name by path
	/// @param devicePath The device path
	/// @return The name of the device on success else empty String
	///
	virtual QString getDeviceName(const QString& devicePath) const;

	///
	/// @brief Get a map of name/index pair of supported device inputs
	/// @param devicePath The device path
	/// @return multi pair of name/index on success else empty pair
	///
	virtual QMultiMap<QString, int> getDeviceInputs(const QString& devicePath) const;

	///
	/// @brief Get a list of all available device encoding formats depends on device input
	/// @param devicePath The device path
	/// @param inputIndex The device input index
	/// @return List of device encoding formats on success else empty List
	///
	virtual QStringList getAvailableEncodingFormats(const QString& devicePath, const int& deviceInput) const;

	///
	/// @brief Get a map of available device resolutions (width/heigth) depends on device input and encoding format
	/// @param devicePath The device path
	/// @param inputIndex The device input index
	/// @param encFormat The device encoding format
	/// @return Map of resolutions (width/heigth) on success else empty List
	///
	virtual QMultiMap<int, int> getAvailableDeviceResolutions(const QString& devicePath, const int& deviceInput, const PixelFormat& encFormat) const;

	///
	/// @brief Get a list of available device framerates depends on encoding format and resolution
	/// @param devicePath The device path
	/// @param inputIndex The device input index
	/// @param encFormat The device encoding format
	/// @param width The device width
	/// @param heigth The device heigth
	/// @return List of framerates on success else empty List
	///
	virtual QStringList getAvailableDeviceFramerates(const QString& devicePath, const int& deviceInput, const PixelFormat& encFormat, const unsigned width, const unsigned height) const;

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
