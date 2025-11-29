#ifndef HYPERIOND_H
#define HYPERIOND_H

#include <memory>

#include <QApplication>
#include <QObject>
#include <QJsonObject>
#include <QScopedPointer>
#include <QWeakPointer>

#include <hyperion/HyperionIManager.h>


#ifdef ENABLE_AMLOGIC
	#include <grabber/amlogic/AmlogicWrapper.h>
#else
	using AmlogicWrapper = QObject;
#endif

#ifdef ENABLE_DISPMANX
	#include <grabber/dispmanx/DispmanxWrapper.h>
#else
	using DispmanxWrapper = QObject;
#endif

#ifdef ENABLE_DDA
	#include <grabber/dda/DDAWrapper.h>
#else
	using DDAWrapper = QObject;
#endif

#ifdef ENABLE_DRM
	#include <grabber/drm/DRMWrapper.h>
#else
	using FramebufferWrapper = QObject;
#endif

#ifdef ENABLE_DX
	#include <grabber/directx/DirectXWrapper.h>
#else
	using DirectXWrapper = QObject;
#endif

#ifdef ENABLE_FB
	#include <grabber/framebuffer/FramebufferWrapper.h>
#else
	using FramebufferWrapper = QObject;
#endif

#ifdef ENABLE_OSX
	#include <grabber/osx/OsxWrapper.h>
#else
	using OsxWrapper = QObject;
#endif

#ifdef ENABLE_QT
	#include <grabber/qt/QtWrapper.h>
#else
	using OsxWrapper = QObject;
#endif

#ifdef ENABLE_X11
	#include <grabber/x11/X11Wrapper.h>
#else
	using X11Wrapper = QObject;
#endif

#ifdef ENABLE_XCB
	#include <grabber/xcb/XcbWrapper.h>
#else
	using XcbWrapper = QObject;
#endif

#if defined(ENABLE_V4L2) || defined(ENABLE_MF)
	#include <grabber/video/VideoWrapper.h>
#else
	using VideoWrapper = QObject;
#endif

#include <hyperion/GrabberWrapper.h>
#ifdef ENABLE_AUDIO
	#include <grabber/audio/AudioWrapper.h>
#else
	using AudioWrapper = QObject;
#endif


#include <utils/Logger.h>
#include <utils/VideoMode.h>

// settings management
#include <utils/settings.h>
#include <utils/Components.h>

#include <events/EventHandler.h>
#include <events/OsEventHandler.h>
#include <events/EventScheduler.h>

#if defined(ENABLE_EFFECTENGINE)
#include <effectengine/EffectFileHandler.h>
#endif

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
#if defined(ENABLE_FORWARDER)
class MessageForwarder;
#endif
class AuthManager;
class CECHandler;

class HyperionDaemon : public QObject
{
	Q_OBJECT

	friend SysTray;

public:
	HyperionDaemon(const QString& rootPath, QObject *parent, bool logLvlOverwrite);
	~HyperionDaemon() override;

	///
	/// @brief Get webserver pointer (systray)
	///
	WebServer *getWebServerInstance() { return _webServer.data(); }

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
	void stopServices();

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

	void createNetworkInputCaptureServices();
	void startNetworkInputCaptureServices();
	void stopNetworkInputCaptureServices();

	void createNetworkOutputServices();
	void startNetworkOutputServices();
	void stopNetworkOutputServices();

	void startGrabberServices();
	void restartGrabberServices();
	void stopGrabberServices();

	void updateScreenGrabbers(const QJsonDocument& grabberConfig);
	void updateVideoGrabbers(const QJsonObject& grabberConfig);
	void updateAudioGrabbers(const QJsonObject& grabberConfig);

	QString evalScreenGrabberType();

	template<typename GrabberType>
	void startGrabber(QScopedPointer<GrabberWrapper>& sharedGrabber, const QJsonDocument& grabberConfig, bool enableGrabber = true) {

		QString typeName = GrabberType::GRABBERTYPE;
		if (!enableGrabber)
		{
			Debug(_log, "The %s grabber is not enabled on this platform", QSTRING_CSTR(typeName));
			sharedGrabber.reset();
			return;
		}

		std::unique_ptr<GrabberType> grabber = std::make_unique<GrabberType>(grabberConfig);

		if (!grabber)
		{
			Error(_log, "Failed to cast grabber type %s to GrabberWrapper", QSTRING_CSTR(typeName));
		}
		else
		{
			if (!grabber->getGrabber()->isAvailable())
			{
				Debug(_log, "The %s grabber is not available on this platform", QSTRING_CSTR(typeName));
				return;
			}
			connect(this, &HyperionDaemon::videoMode, grabber.get(), &GrabberType::setVideoMode);
			connect(this, &HyperionDaemon::settingsChanged, grabber.get(), &GrabberType::handleSettingsUpdate);

			Info(_log, "%s grabber created", QSTRING_CSTR(typeName));
			grabber->tryStart();

			 // Transfer ownership to sharedGrabber
			sharedGrabber.reset(grabber.release());
		}
	}

	QSharedPointer<Logger> _log;

	/// Core services
	QScopedPointer<SettingsManager> _settingsManager;

#if defined(ENABLE_EFFECTENGINE)
	PythonInit* _pyInit;
#endif

	/// Network services
	QScopedPointer<JsonServer> _jsonServer;
	QScopedPointer<QThread> _jsonServerThread;
	QScopedPointer<WebServer> _webServer;
	QScopedPointer<QThread> _webServerThread;
	QScopedPointer<WebServer> _sslWebServer;
	QScopedPointer<QThread> _sslWebServerThread;
	QScopedPointer<SSDPHandler> _ssdpHandler;
	QScopedPointer<QThread> _ssdpHandlerThread;
	
#ifdef ENABLE_MDNS
	QScopedPointer<MdnsProvider> _mDNSProvider;
	QScopedPointer<QThread> _mDnsThread;
#endif
#if defined(ENABLE_FLATBUF_SERVER)
	QScopedPointer<FlatBufferServer> _flatBufferServer;
	QScopedPointer<QThread> _flatBufferServerThread;
#endif
#if defined(ENABLE_PROTOBUF_SERVER)
	QScopedPointer<ProtoServer> _protoServer;
	QScopedPointer<QThread> _protoServerThread;
#endif
#if defined(ENABLE_FORWARDER)
	QScopedPointer<MessageForwarder> _messageForwarder;
	QScopedPointer<QThread> _messageForwarderThread;
#endif

	/// Event services
	QScopedPointer<OsEventHandler> _osEventHandler;
	QScopedPointer<EventScheduler> _eventScheduler;
#ifdef ENABLE_CEC
	QScopedPointer<CECHandler> _cecHandler;
	QScopedPointer<QThread> _cecHandlerThread;
#endif

	/// Grabber services
	QScopedPointer<GrabberWrapper> _screenGrabber;
	QScopedPointer<VideoWrapper> _videoGrabber;
	QScopedPointer<AudioWrapper> _audioGrabber;

	QString                    _prevType;
	VideoMode                  _currVideoMode;
	QWeakPointer<HyperionIManager> _instanceManagerWeak;
};

#endif // HYPERIOND_H
