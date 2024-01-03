#pragma once

#include <QApplication>
#include <QObject>
#include <QJsonObject>
#include <QScopedPointer>

#include <hyperion/HyperionIManager.h>

#ifdef ENABLE_DISPMANX
	#include <grabber/dispmanx/DispmanxWrapper.h>
#else
	typedef QObject DispmanxWrapper;
#endif

#if defined(ENABLE_V4L2) || defined(ENABLE_MF)
	#include <grabber/video/VideoWrapper.h>
#else
	typedef QObject VideoWrapper;
#endif

#ifdef ENABLE_FB
	#include <grabber/framebuffer/FramebufferWrapper.h>
#else
	typedef QObject FramebufferWrapper;
#endif

#ifdef ENABLE_AMLOGIC
	#include <grabber/amlogic/AmlogicWrapper.h>
#else
	typedef QObject AmlogicWrapper;
#endif

#ifdef ENABLE_OSX
	#include <grabber/osx/OsxWrapper.h>
#else
	typedef QObject OsxWrapper;
#endif

#ifdef ENABLE_X11
	#include <grabber/x11/X11Wrapper.h>
#else
	typedef QObject X11Wrapper;
#endif

#ifdef ENABLE_XCB
	#include <grabber/xcb/XcbWrapper.h>
#else
	typedef QObject XcbWrapper;
#endif

#ifdef ENABLE_QT
	#include <grabber/qt/QtWrapper.h>
#else
	typedef QObject QtWrapper;
#endif

#ifdef ENABLE_DX
	#include <grabber/directx/DirectXWrapper.h>
#else
	typedef QObject DirectXWrapper;
#endif

#include <hyperion/GrabberWrapper.h>
#ifdef ENABLE_AUDIO
	#include <grabber/audio/AudioWrapper.h>
#else
	typedef QObject AudioWrapper;
#endif


#include <utils/Logger.h>
#include <utils/VideoMode.h>

// settings management
#include <utils/settings.h>
#include <utils/Components.h>

#include <events/EventHandler.h>
#include <events/OsEventHandler.h>
#include <events/EventScheduler.h>

class HyperionIManager;
class SysTray;
class JsonServer;
#ifdef ENABLE_MDNS
class MdnsProvider;
#endif
class WebServer;
class SettingsManager;
#if defined(ENABLE_EFFECTENGINE)
class PythonInit;
#endif
class SSDPHandler;
class FlatBufferServer;
class ProtoServer;
class AuthManager;
class NetOrigin;
class CECHandler;

class HyperionDaemon : public QObject
{
	Q_OBJECT

	friend SysTray;

public:
	HyperionDaemon(const QString& rootPath, QObject *parent, bool logLvlOverwrite, bool readonlyMode = false);
	~HyperionDaemon() override;

	///
	/// @brief Get webserver pointer (systray)
	///
	WebServer *getWebServerInstance() { return _webserver.data(); }

	///
	/// @brief Get the current videoMode
	///
	VideoMode getVideoMode() const { return _currVideoMode; }

	///
	/// @brief get the settings
	///
	QJsonDocument getSetting(settings::type type) const;

	static HyperionDaemon* getInstance() { return daemon; }
	static HyperionDaemon* daemon;

public slots:
	void stoppServices();

signals:
	///////////////////////////////////////
	/// FROM HYPERIONDAEMON TO HYPERION ///
	///////////////////////////////////////

	///
	/// @brief After eval of setVideoMode this signal emits with a new one on change
	///
	void videoMode(VideoMode mode);

	///////////////////////////////////////
	/// FROM HYPERION TO HYPERIONDAEMON ///
	///////////////////////////////////////

	///
	/// @brief PIPE settings events from Hyperion class to HyperionDaemon components
	///
	void settingsChanged(settings::type type, const QJsonDocument& data);

	///
	/// @brief PIPE component state changes events from Hyperion class to HyperionDaemon components
	///
	void compStateChangeRequest(hyperion::Components component, bool enable);

private slots:
	///
	/// @brief Handle settings update from Hyperion Settingsmanager emit or this constructor
	/// @param type   settingyType from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(settings::type type, const QJsonDocument& config);

	///
	/// @brief Listen for videoMode changes and emit videoMode in case of a change, update _currVideoMode
	/// @param mode  The requested video mode
	///
	void setVideoMode(VideoMode mode);

	/// @brief Handle whenever the state of a instance (HyperionIManager) changes according to enum instanceState
	/// @param instaneState  A state from enum
	/// @param instance      The index of instance
	///
	void handleInstanceStateChange(InstanceState state, quint8 instance);

private:

	void createNetworkServices();
	void startNetworkServices();
	void stopNetworkServices();

	void startEventServices();
	void stopEventServices();

	void startGrabberServices();
	void stopGrabberServices();

	void updateScreenGrabbers(const QJsonObject& grabberConfig);
	void updateVideoGrabbers(const QJsonObject& grabberConfig);
	void updateAudioGrabbers(const QJsonObject& grabberConfig);

	void startGrabberDispmanx(const QJsonObject & grabberConfig);
	void startGrabberAmlogic(const QJsonObject & grabberConfig);
	void startGrabberFramebuffer(const QJsonObject & grabberConfig);
	void startGrabberOsx(const QJsonObject & grabberConfig);
	void startGrabberX11(const QJsonObject & grabberConfig);
	void startGrabberXcb(const QJsonObject & grabberConfig);
	void startGrabberQt(const QJsonObject & grabberConfig);
	void startGrabberDx(const QJsonObject & grabberConfig);

	Logger* _log;

	QScopedPointer<HyperionIManager> _instanceManager;
	QScopedPointer<SettingsManager> _settingsManager;
	QScopedPointer<AuthManager> _authManager;

	QScopedPointer<NetOrigin> _netOrigin;
	QScopedPointer<JsonServer, QScopedPointerDeleteLater> _jsonServer;
	QScopedPointer<WebServer, QScopedPointerDeleteLater> _webserver;
	QScopedPointer<WebServer, QScopedPointerDeleteLater> _sslWebserver;
	QScopedPointer<SSDPHandler, QScopedPointerDeleteLater> _ssdp;
#ifdef ENABLE_MDNS
	QScopedPointer<MdnsProvider, QScopedPointerDeleteLater> _mDNSProvider;
#endif

#if defined(ENABLE_FLATBUF_SERVER)
	QScopedPointer<FlatBufferServer, QScopedPointerDeleteLater> _flatBufferServer;
#endif
#if defined(ENABLE_PROTOBUF_SERVER)
	QScopedPointer<ProtoServer, QScopedPointerDeleteLater> _protoServer;
#endif

	QScopedPointer<EventHandler> _eventHandler;
	QScopedPointer<OsEventHandler> _osEventHandler;
	QScopedPointer<EventScheduler> _eventScheduler;
#ifdef ENABLE_CEC
	QScopedPointer<CECHandler> _cecHandler;
#endif

	QScopedPointer<VideoWrapper> _videoGrabber;
	QScopedPointer<DispmanxWrapper> _dispmanx;
	QScopedPointer<X11Wrapper> _x11Grabber;
	QScopedPointer<XcbWrapper> _xcbGrabber;
	QScopedPointer<AmlogicWrapper> _amlGrabber;
	QScopedPointer<FramebufferWrapper> _fbGrabber;
	QScopedPointer<OsxWrapper> _osxGrabber;
	QScopedPointer<QtWrapper> _qtGrabber;
	QScopedPointer<DirectXWrapper> _dxGrabber;
	QScopedPointer<AudioWrapper> _audioGrabber;

	QString                    _prevType;
	VideoMode                  _currVideoMode;

#if defined(ENABLE_EFFECTENGINE)
	PythonInit*                _pyInit;
#endif
};
