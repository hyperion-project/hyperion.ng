#include <string>
#include <vector>

#include <QCoreApplication>
#include <QtGlobal>
#include <QResource>
#include <QLocale>
#include <QFile>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QPair>
#include <QThread>
#include <QByteArray>
#include <QMap>
#include <QMetaType>

#include "db/SettingsTable.h"
#include "events/EventScheduler.h"
#include "events/OsEventHandler.h"
#include <events/EventHandler.h>
#include <utils/Components.h>
#include <utils/Image.h>
#include <utils/settings.h>
#include "utils/ColorRgb.h"
#include "utils/Logger.h"
#include "utils/VideoMode.h"
#include "utils/global_defines.h"
#include <utils/GlobalSignals.h>

#include <HyperionConfig.h> // Required to determine the cmake options

// mDNS
#ifdef ENABLE_MDNS
#include <mdns/MdnsProvider.h>
#include <mdns/MdnsBrowser.h>
#endif

#include <jsonserver/JsonServer.h>
#include <webserver/WebServer.h>
#include "hyperiond.h"

// Flatbuffer Server
#ifdef ENABLE_FLATBUF_SERVER
#include <flatbufserver/FlatBufferServer.h>
#endif

// Protobuffer Server
#ifdef ENABLE_PROTOBUF_SERVER
#include <protoserver/ProtoServer.h>
#endif

// Message Forwarder
#if defined(ENABLE_FORWARDER)
#include <forwarder/MessageForwarder.h>
#endif

// ssdp
#include <ssdp/SSDPHandler.h>

// settings
#include <hyperion/SettingsManager.h>

// AuthManager
#include <hyperion/AuthManager.h>

// InstanceManager Hyperion
#include <hyperion/HyperionIManager.h>

#if defined(ENABLE_EFFECTENGINE)
// Init Python
#include <python/PythonInit.h>
#endif

#ifdef ENABLE_CEC
#include <cec/CECHandler.h>
#endif

namespace {
	const GlobalSignals * const ensureGlobalSignalsInitialized = GlobalSignals::getInstance();
}

HyperionDaemon* HyperionDaemon::daemon = nullptr;

HyperionDaemon::HyperionDaemon(const QString& rootPath, QObject* parent, bool logLvlOverwrite)
	: QObject(parent), _log(Logger::getInstance("DAEMON"))
	, _instanceManager(new HyperionIManager(this))
	, _settingsManager(new SettingsManager(NO_INSTANCE_ID, this)) // init settings, this settingsManager accesses global settings which are independent from instances
	#if defined(ENABLE_EFFECTENGINE)
	, _pyInit(new PythonInit())
	#endif
	, _authManager(new AuthManager(this))
	, _currVideoMode(VideoMode::VIDEO_2D)
{
	HyperionDaemon::daemon = this;

	// Register metas for thread queued connection
	qRegisterMetaType<Image<ColorRgb>>("Image<ColorRgb>");
	qRegisterMetaType<hyperion::Components>("hyperion::Components");
	qRegisterMetaType<settings::type>("settings::type");
	qRegisterMetaType<VideoMode>("VideoMode");
	qRegisterMetaType<QMap<quint8, QJsonObject>>("QMap<quint8,QJsonObject>");
	qRegisterMetaType<QVector<ColorRgb>>("QVector<ColorRgb>");

	// set inital log lvl if the loglvl wasn't overwritten by arg
	if (!logLvlOverwrite)
	{
		handleSettingsUpdate(settings::LOGGER, getSetting(settings::LOGGER));
	}

#if defined(ENABLE_EFFECTENGINE)
	// init EffectFileHandler
	_effectFileHandler.reset(new EffectFileHandler(rootPath, getSetting(settings::EFFECTS)));
	connect(this, &HyperionDaemon::settingsChanged, _effectFileHandler.data(), &EffectFileHandler::handleSettingsUpdate);
#endif

	// spawn all Hyperion instances (non blocking)
	_instanceManager->startAll();

	//Cleaning up Hyperion before quit
	connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &HyperionDaemon::stopServices);

	//Handle services dependent on the on first instance's availability
	connect(_instanceManager.get(), &HyperionIManager::instanceStateChanged, this, &HyperionDaemon::handleInstanceStateChange);

	// pipe settings changes from HyperionIManager to Daemon
	connect(_instanceManager.get(), &HyperionIManager::settingsChanged, this, &HyperionDaemon::settingsChanged);

	// listen for setting changes of framegrabber and v4l2
	connect(this, &HyperionDaemon::settingsChanged, this, &HyperionDaemon::handleSettingsUpdate);

	// forward videoModes from HyperionIManager to Daemon evaluation
	connect(_instanceManager.get(), &HyperionIManager::requestVideoMode, this, &HyperionDaemon::setVideoMode);
	// return videoMode changes from Daemon to HyperionIManager
	connect(this, &HyperionDaemon::videoMode, _instanceManager.get(), &HyperionIManager::newVideoMode);

	createNetworkServices();
	createNetworkInputCaptureServices();
	createNetworkOutputServices();

	startNetworkServices();
	startEventServices();
	startGrabberServices();

	startNetworkOutputServices();
}

HyperionDaemon::~HyperionDaemon()
{
	Info(_log, "Hyperion daemon stopped");
}

void HyperionDaemon::handleInstanceStateChange(InstanceState state, quint8 instance)
{
	switch (state)
	{
	case InstanceState::H_STARTING:
	break;
	case InstanceState::H_STARTED:
	{
		startNetworkInputCaptureServices();
#if defined(ENABLE_FORWARDER)
		QMetaObject::invokeMethod(_messageForwarder.get(),
			[this, instance]() {
				if (_messageForwarder->connectToInstance(instance))
				{
					_messageForwarder->start();
				}
			},
			Qt::QueuedConnection);
#endif
	}
	break;
	case InstanceState::H_STOPPED:
#if defined(ENABLE_FORWARDER)
		QMetaObject::invokeMethod(_messageForwarder.get(),
			[this, instance]() { _messageForwarder->disconnectFromInstance(instance); },
			Qt::QueuedConnection);
#endif

		if(_instanceManager->getRunningInstanceIdx().empty())
		{
#if defined(ENABLE_FORWARDER)
			QMetaObject::invokeMethod(_messageForwarder.get(), &MessageForwarder::stop, Qt::QueuedConnection);
#endif
			stopNetworkInputCaptureServices();
		}
	break;
	case InstanceState::H_CREATED:
	break;
	case InstanceState::H_ON_STOP:
	break;
	case InstanceState::H_DELETED:
	break;

	default:
		qWarning() << "HyperionDaemon::handleInstanceStateChange - Unhandled state:" << static_cast<int>(state);
	break;
	}

}

void HyperionDaemon::setVideoMode(VideoMode mode)
{
	if (_currVideoMode != mode)
	{
		_currVideoMode = mode;
		emit videoMode(mode);
	}
}

QJsonDocument HyperionDaemon::getSetting(settings::type type) const
{
	return _settingsManager->getSetting(type);
}

void HyperionDaemon::stopServices()
{
	Info(_log, "Stopping Hyperion services...");

	stopEventServices();
	stopGrabberServices();

	// Ensure that all Instances and their threads are stopped
	QEventLoop loopInstances;
	QObject::connect(_instanceManager.get(), &HyperionIManager::areAllInstancesStopped, &loopInstances, &QEventLoop::quit);
	_instanceManager->stopAll();
	loopInstances.exec();

	stopNetworkOutputServices();
	stopNetworkServices();

#if defined(ENABLE_EFFECTENGINE)
	// Finalize Python environment when all sub-interpreters were stopped, i.e. all must have been stopped before
	delete _pyInit;
#endif
}

void HyperionDaemon::createNetworkServices()
{
	// connect and apply settings for AuthManager
	connect(this, &HyperionDaemon::settingsChanged, _authManager.get(), &AuthManager::handleSettingsUpdate);
	_authManager->handleSettingsUpdate(settings::NETWORK, _settingsManager->getSetting(settings::NETWORK));

#ifdef ENABLE_MDNS
	// Create mDNS-Provider and mDNS-Browser in own thread
	_mDnsThread.reset(new QThread());
	_mDnsThread->setObjectName("mDNSThread");
	_mDNSProvider.reset(new MdnsProvider());
	_mDNSProvider->moveToThread(_mDnsThread.get());
	connect(_mDnsThread.get(), &QThread::started, _mDNSProvider.get(), &MdnsProvider::init);

	MdnsBrowser::getInstance(_mDnsThread.get());
#endif

	// Create SSDP server
	_ssdpHandlerThread.reset(new QThread());
	_ssdpHandlerThread->setObjectName("SSDPThread");
	_ssdpHandler.reset(new SSDPHandler());
	_ssdpHandler->moveToThread(_ssdpHandlerThread.get());
	connect(_ssdpHandlerThread.get(), &QThread::started, _ssdpHandler.get(), &SSDPHandler::initServer);
	connect(this, &HyperionDaemon::settingsChanged, _ssdpHandler.get(), &SSDPHandler::handleSettingsUpdate);
	_ssdpHandler->handleSettingsUpdate(settings::GENERAL, _settingsManager->getSetting(settings::GENERAL));

	// Create JSON server in own thread
	_jsonServerThread.reset(new QThread());
	_jsonServerThread->setObjectName("JSONServerThread");
	_jsonServer.reset(new JsonServer(getSetting(settings::JSONSERVER)));
	_jsonServer->moveToThread(_jsonServerThread.get());

	connect(_jsonServer.get(), &JsonServer::publishService, _ssdpHandler.get(), &SSDPHandler::onPortChanged);
#ifdef ENABLE_MDNS
	connect(_jsonServer.get(), &JsonServer::publishService, _mDNSProvider.get(), &MdnsProvider::publishService);
#endif

	connect(_jsonServerThread.get(), &QThread::started, _jsonServer.get(), &JsonServer::initServer);
	connect(this, &HyperionDaemon::settingsChanged, _jsonServer.get(), &JsonServer::handleSettingsUpdate);

	// Create Webserver in own thread
	_webServerThread.reset(new QThread());
	_webServerThread->setObjectName("WebServerThread");
	_webServer.reset(new WebServer(getSetting(settings::WEBSERVER), false));
	_webServer->moveToThread(_webServerThread.get());

	connect(_webServer.get(), &WebServer::stateChange, _ssdpHandler.get(), &SSDPHandler::onStateChange);
	connect(_webServer.get(), &WebServer::publishService, _ssdpHandler.get(), &SSDPHandler::onPortChanged);
	connect(_ssdpHandler.get(), &SSDPHandler::descriptionUpdated, _webServer.get(), &WebServer::onSsdpDescriptionUpdated);
#ifdef ENABLE_MDNS
	connect(_webServer.get(), &WebServer::publishService, _mDNSProvider.get(), &MdnsProvider::publishService);
#endif

	connect(_webServerThread.get(), &QThread::started, _webServer.get(), &WebServer::initServer);
	connect(this, &HyperionDaemon::settingsChanged, _webServer.get(), &WebServer::handleSettingsUpdate);

	// Create SSL Webserver in own thread
	_sslWebServerThread.reset(new QThread());
	_sslWebServerThread->setObjectName("SSLWebServerThread");
	_sslWebServer.reset(new WebServer(getSetting(settings::WEBSERVER), true));
	_sslWebServer->moveToThread(_sslWebServerThread.get());

	connect(_sslWebServer.get(), &WebServer::publishService, _ssdpHandler.get(), &SSDPHandler::onPortChanged);
	connect(_ssdpHandler.get(), &SSDPHandler::descriptionUpdated, _sslWebServer.get(), &WebServer::onSsdpDescriptionUpdated);
#ifdef ENABLE_MDNS
	connect(_sslWebServer.get(), &WebServer::publishService, _mDNSProvider.get(), &MdnsProvider::publishService);
#endif

	connect(_sslWebServerThread.get(), &QThread::started, _sslWebServer.get(), &WebServer::initServer);
	connect(this, &HyperionDaemon::settingsChanged, _sslWebServer.get(), &WebServer::handleSettingsUpdate);
}

void HyperionDaemon::startNetworkServices()
{
	//Start JSON-Server only after WebServery were started to avoid port mismatches
	connect(_sslWebServerThread.get(), &QThread::started, [=]() {
		_jsonServerThread->start();
		});

	_webServerThread->start();
	_sslWebServerThread->start();

#if defined(ENABLE_MDNS)
	_mDnsThread->start();
#endif
	_ssdpHandlerThread->start();
}

void HyperionDaemon::stopNetworkServices()
{
#if defined(ENABLE_MDNS)
	QMetaObject::invokeMethod(MdnsBrowser::getInstance().get(), "stop", Qt::QueuedConnection);
	QMetaObject::invokeMethod(_mDNSProvider.get(), &MdnsProvider::stop, Qt::QueuedConnection);
	if (_mDnsThread->isRunning()) {
		_mDnsThread->quit();
		_mDnsThread->wait();
	}
#endif

	if (_webServerThread->isRunning()) {
		_webServerThread->quit();
		_webServerThread->wait();
	}

	QMetaObject::invokeMethod(_sslWebServer.get(), &WebServer::stop, Qt::QueuedConnection);
	if (_sslWebServerThread->isRunning()) {
		_sslWebServerThread->quit();
		_sslWebServerThread->wait();
	}

	if (_jsonServerThread->isRunning()) {
		_jsonServerThread->quit();
		_jsonServerThread->wait();
	}
	_jsonServer.reset(nullptr);

	QMetaObject::invokeMethod(_ssdpHandler.get(), &SSDPHandler::stop, Qt::QueuedConnection);
	if (_ssdpHandlerThread->isRunning()) {
		_ssdpHandlerThread->quit();
		_ssdpHandlerThread->wait();
	}
}

void HyperionDaemon::createNetworkInputCaptureServices()
{
#if defined(ENABLE_FLATBUF_SERVER)
	// Create FlatBuffer server in thread
	_flatBufferServerThread.reset(new QThread());
	_flatBufferServerThread->setObjectName("FlatBufferServerThread");
	_flatBufferServer.reset(new FlatBufferServer(getSetting(settings::FLATBUFSERVER)));
	_flatBufferServer->moveToThread(_flatBufferServerThread.get());

	connect(_flatBufferServer.get(), &FlatBufferServer::publishService, _ssdpHandler.get(), &SSDPHandler::onPortChanged);
#ifdef ENABLE_MDNS
	connect(_flatBufferServer.get(), &FlatBufferServer::publishService, _mDNSProvider.get(), &MdnsProvider::publishService);
#endif

	connect(_flatBufferServerThread.get(), &QThread::started, _flatBufferServer.get(), &FlatBufferServer::initServer);
	connect(this, &HyperionDaemon::settingsChanged, _flatBufferServer.get(), &FlatBufferServer::handleSettingsUpdate);
#endif

#if defined(ENABLE_PROTOBUF_SERVER)
	// Create Proto server in thread
	_protoServerThread.reset(new QThread());
	_protoServerThread->setObjectName("ProtoServerThread");
	_protoServer.reset(new ProtoServer(getSetting(settings::PROTOSERVER)));
	_protoServer->moveToThread(_protoServerThread.get());

	connect(_protoServer.get(), &ProtoServer::publishService, _ssdpHandler.get(), &SSDPHandler::onPortChanged);
#ifdef ENABLE_MDNS
	connect(_protoServer.get(), &ProtoServer::publishService, _mDNSProvider.get(), &MdnsProvider::publishService);
#endif

	connect(_protoServerThread.get(), &QThread::started, _protoServer.get(), &ProtoServer::initServer);
	connect(this, &HyperionDaemon::settingsChanged, _protoServer.get(), &ProtoServer::handleSettingsUpdate);
#endif
}

void HyperionDaemon::startNetworkInputCaptureServices()
{
#if defined(ENABLE_FLATBUF_SERVER)
	if (!_flatBufferServerThread->isRunning())
	{
		_flatBufferServerThread->start();
	}
#endif
#if defined(ENABLE_PROTOBUF_SERVER)
	if (!_protoServerThread->isRunning())
	{
		_protoServerThread->start();
	}
#endif
}

void HyperionDaemon::stopNetworkInputCaptureServices()
{
#if defined(ENABLE_PROTOBUF_SERVER)
	if (_protoServerThread->isRunning()) {
		QMetaObject::invokeMethod(_protoServer.get(), &ProtoServer::stop, Qt::QueuedConnection);
		_protoServerThread->quit();
		_protoServerThread->wait();
	}
#endif

#if defined(ENABLE_FLATBUF_SERVER)
	if (_flatBufferServerThread->isRunning()) {
		QMetaObject::invokeMethod(_flatBufferServer.get(), &FlatBufferServer::stop, Qt::QueuedConnection);
		_flatBufferServerThread->quit();
		_flatBufferServerThread->wait();
	}
#endif
}

void HyperionDaemon::createNetworkOutputServices()
{
#if defined(ENABLE_FORWARDER)
	// Create Message Forwarder in thread
	_messageForwarderThread.reset(new QThread());
	_messageForwarderThread->setObjectName("MessageForwarderThread");
	_messageForwarder.reset(new MessageForwarder(getSetting(settings::NETFORWARD)));
	_messageForwarder->moveToThread(_messageForwarderThread.get());

	connect(_messageForwarderThread.get(), &QThread::started, _messageForwarder.get(), &MessageForwarder::init);
	connect(this, &HyperionDaemon::settingsChanged, _messageForwarder.get(), &MessageForwarder::handleSettingsUpdate);
#endif
}

void HyperionDaemon::startNetworkOutputServices()
{
#if defined(ENABLE_FORWARDER)
	if (!_messageForwarderThread->isRunning())
	{
		_messageForwarderThread->start();
	}
#endif
}

void HyperionDaemon::stopNetworkOutputServices()
{
#if defined(ENABLE_FORWARDER)
	if (_messageForwarderThread->isRunning())
	{
		QMetaObject::invokeMethod(_messageForwarder.get(), &MessageForwarder::stop, Qt::QueuedConnection);
		_messageForwarderThread->quit();
		_messageForwarderThread->wait();
	}
#endif
}

void HyperionDaemon::startEventServices()
{
	EventHandler::getInstance();

	_eventScheduler.reset(new EventScheduler());
	_eventScheduler->handleSettingsUpdate(settings::SCHEDEVENTS, getSetting(settings::SCHEDEVENTS));
	connect(this, &HyperionDaemon::settingsChanged, _eventScheduler.get(), &EventScheduler::handleSettingsUpdate);

	_osEventHandler.reset(new OsEventHandler());
	_osEventHandler->handleSettingsUpdate(settings::OSEVENTS, getSetting(settings::OSEVENTS));
	connect(this, &HyperionDaemon::settingsChanged, _osEventHandler.get(), &OsEventHandler::handleSettingsUpdate);

#if defined(ENABLE_CEC)
	_cecHandlerThread.reset(new QThread());
	_cecHandlerThread->setObjectName("CECThread");
	_cecHandler.reset(new CECHandler(getSetting(settings::CECEVENTS)));
	_cecHandler->moveToThread(_cecHandlerThread.get());
	connect(_cecHandlerThread.get(), &QThread::started, _cecHandler.get(), &CECHandler::start);
	connect(this, &HyperionDaemon::settingsChanged, _cecHandler.get(), &CECHandler::handleSettingsUpdate);
	Info(_log, "CEC event handler created");
	_cecHandlerThread->start();
#else
	Debug(_log, "The CEC handler is not supported on this platform");
#endif
}

void HyperionDaemon::stopEventServices()
{
#if defined(ENABLE_CEC)
	QMetaObject::invokeMethod(_cecHandler.get(), &CECHandler::stop, Qt::QueuedConnection);
	if (_cecHandlerThread->isRunning())
	{
		_cecHandlerThread->quit();
		_cecHandlerThread->wait();
	}
#endif

	_osEventHandler.reset(nullptr);
	_eventScheduler.reset(nullptr);
}

void HyperionDaemon::startGrabberServices()
{
	// start screen capture
	handleSettingsUpdate(settings::SYSTEMCAPTURE, getSetting(settings::SYSTEMCAPTURE));

	// start video capture (v4l2 && media foundation)
	handleSettingsUpdate(settings::V4L2, getSetting(settings::V4L2));

	// start audio capture
	handleSettingsUpdate(settings::AUDIO, getSetting(settings::AUDIO));
}

void HyperionDaemon::stopGrabberServices()
{
	_screenGrabber.reset();
	_videoGrabber.reset();
	_audioGrabber.reset();
}

void HyperionDaemon::handleSettingsUpdate(settings::type settingsType, const QJsonDocument& config)
{
	if (settingsType == settings::LOGGER)
	{
		const QJsonObject& logConfig = config.object();

		std::string const level = logConfig["level"].toString("warn").toStdString(); // silent warn verbose debug
		if (level == "silent")
		{
			Logger::setLogLevel(Logger::LOG_OFF);
		}
		else if (level == "warn")
		{
			Logger::setLogLevel(Logger::LOG_WARNING);
		}
		else if (level == "verbose")
		{
			Logger::setLogLevel(Logger::LOG_INFO);
		}
		else if (level == "debug")
		{
			Logger::setLogLevel(Logger::LOG_DEBUG);
		}
	}

	if (settingsType == settings::SYSTEMCAPTURE)
	{
		updateScreenGrabbers(config);
	}
	else if (settingsType == settings::V4L2)
	{
		const QJsonObject& grabberConfig = config.object();
		updateVideoGrabbers(grabberConfig);
	}
	else if (settingsType == settings::AUDIO)
	{
		const QJsonObject& grabberConfig = config.object();
		updateAudioGrabbers(grabberConfig);
	}
}

void HyperionDaemon::updateScreenGrabbers(const QJsonDocument& grabberConfig)
{
#if !defined(ENABLE_DISPMANX) && !defined(ENABLE_OSX) && !defined(ENABLE_FB) && !defined(ENABLE_X11) && !defined(ENABLE_XCB) && !defined(ENABLE_AMLOGIC) && !defined(ENABLE_QT) && !defined(ENABLE_DX) && !defined(ENABLE_DDA)
	Info(_log, "No screen capture supported on this platform");
	return;
#endif

#ifdef ENABLE_OSX
	QString type = grabberConfig["device"].toString("osx");
#else
	QString type = grabberConfig["device"].toString("auto");
#endif

	if (type == "auto")
	{
		type = evalScreenGrabberType();
	}

	if (_prevType != type)
	{
		//Stop previous grabber
		_screenGrabber.reset();

		if (type == "auto")
		{
			Error(_log, "The automatic screen grabber type evaluation did not work.");
			return;
		}
#ifdef ENABLE_AMLOGIC
		else if (type == "amlogic")
		{
			startGrabber<AmlogicWrapper>(_screenGrabber, grabberConfig);
		}
#endif
#ifdef ENABLE_DISPMANX
		else if (type == "dispmanx")
		{
			startGrabber<DispmanxWrapper>(_screenGrabber, grabberConfig);
		}
#endif
#ifdef ENABLE_DX
		else if (type == "dx")
		{
			startGrabber<DirectXWrapper>(_screenGrabber, grabberConfig);
		}
#endif
#ifdef ENABLE_DDA
		else if (type == "dda")
		{
			startGrabber<DDAWrapper>(_screenGrabber, grabberConfig);
		}
#endif
#ifdef ENABLE_FB
		else if (type == "framebuffer")
		{
			startGrabber<FramebufferWrapper>(_screenGrabber, grabberConfig);
		}
#endif
#ifdef ENABLE_OSX
		else if (type == "osx")
		{

			startGrabber<OsxWrapper>(_screenGrabber, grabberConfig);

		}
#endif
#ifdef ENABLE_QT
		else if (type == "qt")
		{
			startGrabber<QtWrapper>(_screenGrabber, grabberConfig);
		}
#endif
#ifdef ENABLE_X11
		else if (type == "x11")
		{
			startGrabber<X11Wrapper>(_screenGrabber, grabberConfig);
		}
#endif
#ifdef ENABLE_XCB
		else if (type == "xcb")
		{
			startGrabber<XcbWrapper>(_screenGrabber, grabberConfig);
		}
#endif
		else
		{
			Warning(_log, "The %s grabber is not enabled on this platform", QSTRING_CSTR(type));
			return;
		}
		_prevType = type;
	}
}

void HyperionDaemon::updateVideoGrabbers(const QJsonObject& /*grabberConfig*/)
{
#if defined(ENABLE_V4L2) || defined(ENABLE_MF)
	_videoGrabber.reset(new VideoWrapper());
	_videoGrabber->handleSettingsUpdate(settings::V4L2, getSetting(settings::V4L2));

#if defined(ENABLE_MF)
	Debug(_log, "Media Foundation grabber created");
#elif defined(ENABLE_V4L2)
	Debug(_log, "V4L2 grabber created");
#endif
	// connect to HyperionDaemon signal
	connect(this, &HyperionDaemon::videoMode, _videoGrabber.get(), &VideoWrapper::setVideoMode);
	connect(this, &HyperionDaemon::settingsChanged, _videoGrabber.get(), &VideoWrapper::handleSettingsUpdate);

	Debug(_log, "Video grabber created");
#else
	Warning(_log, "No video capture supported on this platform");
#endif
}

void HyperionDaemon::updateAudioGrabbers(const QJsonObject& /*grabberConfig*/)
{
#ifdef ENABLE_AUDIO
	// Create Audio Grabber
	_audioGrabber.reset(new AudioWrapper());
	_audioGrabber->handleSettingsUpdate(settings::AUDIO, getSetting(settings::AUDIO));

	connect(this, &HyperionDaemon::settingsChanged, _audioGrabber.get(), &AudioWrapper::handleSettingsUpdate);

	Debug(_log, "Audio grabber created");
#else
	Warning(_log, "No audio capture supported on this platform");
#endif
}

QString HyperionDaemon::evalScreenGrabberType()
{
	QString type;

	// dispmanx -> on raspi
	if (QFile::exists("/dev/vchiq"))
	{
		type = "dispmanx";
	}
	// amlogic -> /dev/amvideo exists
	else
	{
		if (QFile::exists("/dev/amvideo"))
		{
			type = "amlogic";

			QString const amlDevice("/dev/amvideocap0");
			if (!QFile::exists(amlDevice))
			{
				Error(_log, "grabber device '%s' for type amlogic not found!", QSTRING_CSTR(amlDevice));
			}
		}
		else
		{
			// x11 -> if DISPLAY is set
			QByteArray const envDisplay = qgetenv("DISPLAY");
			if (!envDisplay.isEmpty())
			{
#if defined(ENABLE_X11)
				type = "x11";
#elif defined(ENABLE_XCB)
				type = "xcb";
#else
				type = "qt";
#endif
			}
			// qt -> if nothing other applies
			else
			{
				type = "qt";
			}
		}
	}

	return type;
}
