#pragma once

#include <QApplication>
#include <QObject>
#include <QJsonObject>

#include <hyperion/HyperionIManager.h>

#ifdef ENABLE_DISPMANX
	#include <grabber/DispmanxWrapper.h>
#else
	typedef QObject DispmanxWrapper;
#endif

#if defined(ENABLE_V4L2) || defined(ENABLE_MF)
	#include <grabber/VideoWrapper.h>
#else
	typedef QObject VideoWrapper;
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

#ifdef ENABLE_XCB
	#include <grabber/XcbWrapper.h>
#else
	typedef QObject XcbWrapper;
#endif

#ifdef ENABLE_QT
	#include <grabber/QtWrapper.h>
#else
	typedef QObject QtWrapper;
#endif

#ifdef ENABLE_DX
	#include <grabber/DirectXWrapper.h>
#else
	typedef QObject DirectXWrapper;
#endif

#include <hyperion/GrabberWrapper.h>

#include <utils/Logger.h>
#include <utils/VideoMode.h>

// settings management
#include <utils/settings.h>
#include <utils/Components.h>

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
	~HyperionDaemon();

	///
	/// @brief Get webserver pointer (systray)
	///
	WebServer *getWebServerInstance() { return _webserver; }

	///
	/// @brief Get the current videoMode
	///
	VideoMode getVideoMode() const { return _currVideoMode; }

	///
	/// @brief get the settings
	///
	QJsonDocument getSetting(settings::type type) const;

	void startNetworkServices();

	static HyperionDaemon* getInstance() { return daemon; }
	static HyperionDaemon* daemon;

public slots:
	void freeObjects();

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
	void createGrabberDispmanx(const QJsonObject & grabberConfig);
	void createGrabberAmlogic(const QJsonObject & grabberConfig);
	void createGrabberFramebuffer(const QJsonObject & grabberConfig);
	void createGrabberOsx(const QJsonObject & grabberConfig);
	void createGrabberX11(const QJsonObject & grabberConfig);
	void createGrabberXcb(const QJsonObject & grabberConfig);
	void createGrabberQt(const QJsonObject & grabberConfig);
	void createCecHandler();
	void createGrabberDx(const QJsonObject & grabberConfig);

	Logger*                    _log;
	HyperionIManager*          _instanceManager;
	AuthManager*               _authManager;
#ifdef ENABLE_MDNS
	MdnsProvider*                _mDNSProvider;
#endif
	NetOrigin*                 _netOrigin;
#if defined(ENABLE_EFFECTENGINE)
	PythonInit*                _pyInit;
#endif
	WebServer*                 _webserver;
	WebServer*                 _sslWebserver;
	JsonServer*                _jsonServer;
	VideoWrapper*              _videoGrabber;
	DispmanxWrapper*           _dispmanx;
	X11Wrapper*                _x11Grabber;
	XcbWrapper*                _xcbGrabber;
	AmlogicWrapper*            _amlGrabber;
	FramebufferWrapper*        _fbGrabber;
	OsxWrapper*                _osxGrabber;
	QtWrapper*                 _qtGrabber;
	DirectXWrapper*            _dxGrabber;
	SSDPHandler*               _ssdp;

	#ifdef ENABLE_CEC
	CECHandler*                _cecHandler;
	#endif
	#if defined(ENABLE_FLATBUF_SERVER)
	FlatBufferServer*          _flatBufferServer;
	#endif
	#if defined(ENABLE_PROTOBUF_SERVER)
	ProtoServer*               _protoServer;
	#endif
	int                        _grabber_width;
	int                        _grabber_height;
	int                        _grabber_pixelDecimation;
	int                        _grabber_frequency;
	int                        _grabber_cropLeft;
	int                        _grabber_cropRight;
	int                        _grabber_cropTop;
	int                        _grabber_cropBottom;

	QString                    _prevType;

	VideoMode                  _currVideoMode;
	SettingsManager*           _settingsManager;
};
