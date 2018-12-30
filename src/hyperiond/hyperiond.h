#pragma once

#include <QObject>
#include <QJsonObject>

#ifdef ENABLE_DISPMANX
	#include <grabber/DispmanxWrapper.h>
#else
	typedef QObject DispmanxWrapper;
#endif

#ifdef ENABLE_V4L2
	#include <grabber/V4L2Wrapper.h>
#else
	typedef QObject V4L2Wrapper;
#endif

#ifdef ENABLE_FB
	#include <grabber/FramebufferWrapper.h>
#else
	typedef QObject FramebufferWrapper;
#endif

#ifdef ENABLE_AMLOGIC
	#include <grabber/AmlogicWrapper.h>
#else
	typedef QObject AmlogicWrapper;
#endif

#ifdef ENABLE_OSX
	#include <grabber/OsxWrapper.h>
#else
	typedef QObject OsxWrapper;
#endif

#ifdef ENABLE_X11
	#include <grabber/X11Wrapper.h>
#else
	typedef QObject X11Wrapper;
#endif

#include <utils/Logger.h>
#include <utils/Image.h>
#include <utils/VideoMode.h>

// settings management
#include <utils/settings.h>

class Hyperion;
class SysTray;
class JsonServer;
class UDPListener;
class Stats;
class BonjourBrowserWrapper;
class WebServer;
class SettingsManager;
class PythonInit;
class SSDPHandler;
class FlatBufferServer;

class HyperionDaemon : public QObject
{
	Q_OBJECT

	friend SysTray;

public:
	HyperionDaemon(QString configFile, QString rootPath, QObject *parent, const bool& logLvlOverwrite );
	~HyperionDaemon();

	///
	/// @brief Get webserver pointer (systray)
	///
	WebServer* getWebServerInstance() { return _webserver; };

	///
	/// @brief Get the current videoMode
	///
	const VideoMode & getVideoMode() { return _currVideoMode; };

	///
	/// @brief get the settings
	///
	const QJsonDocument getSetting(const settings::type& type);

	void startNetworkServices();

	static HyperionDaemon* getInstance() { return daemon; };
	static HyperionDaemon* daemon;

public slots:
	void freeObjects();

signals:
	///
	/// @brief PIPE settings events from Hyperion class to HyperionDaemon components
	///
	void settingsChanged(const settings::type& type, const QJsonDocument& data);

	///
	/// @brief PIPE SystemCapture images from SystemCapture over HyperionDaemon to Hyperion class
	///
	void systemImage(const Image<ColorRgb>&  image);

	///
	/// @brief PIPE v4lCapture images from v4lCapture over HyperionDaemon to Hyperion class
	///
	void v4lImage(const Image<ColorRgb> & image);

	///
	/// @brief After eval of setVideoMode this signal emits with a new one on change
	///
	void videoMode(const VideoMode& mode);

private slots:
	///
	/// @brief Handle settings update from Hyperion Settingsmanager emit or this constructor
	/// @param type   settingyType from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(const settings::type& type, const QJsonDocument& config);

	///
	/// @brief Listen for videoMode changes and emit videoMode in case of a change, update _currVideoMode
	/// @param mode  The requested video mode
	///
	void setVideoMode(const VideoMode& mode);

private:
	void createGrabberDispmanx();
	void createGrabberAmlogic();
	void createGrabberFramebuffer(const QJsonObject & grabberConfig);
	void createGrabberOsx(const QJsonObject & grabberConfig);
	void createGrabberX11(const QJsonObject & grabberConfig);

	Logger*                _log;
	BonjourBrowserWrapper* _bonjourBrowserWrapper;
	PythonInit*            _pyInit;
	WebServer*             _webserver;
	JsonServer*            _jsonServer;
	UDPListener*           _udpListener;
	std::vector<V4L2Wrapper*>  _v4l2Grabbers;
	DispmanxWrapper*       _dispmanx;
	X11Wrapper*            _x11Grabber;
	AmlogicWrapper*        _amlGrabber;
	FramebufferWrapper*    _fbGrabber;
	OsxWrapper*            _osxGrabber;
	Hyperion*              _hyperion;
	Stats*                 _stats;
	SSDPHandler*           _ssdp;
	FlatBufferServer* _flatBufferServer;

	unsigned            _grabber_width;
	unsigned            _grabber_height;
	unsigned            _grabber_frequency;
	unsigned            _grabber_cropLeft;
	unsigned            _grabber_cropRight;
	unsigned            _grabber_cropTop;
    unsigned            _grabber_cropBottom;

	VideoMode _currVideoMode;
	SettingsManager*  _settingsManager;
};
