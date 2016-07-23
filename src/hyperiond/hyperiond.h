#pragma once

#include <QObject>

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

#include <kodivideochecker/KODIVideoChecker.h>
#include <jsonserver/JsonServer.h>
#include <protoserver/ProtoServer.h>
#include <boblightserver/BoblightServer.h>
#include <webconfig/WebConfig.h>
#include <udplistener/UDPListener.h>

class HyperionDaemon : public QObject
{
public:
	HyperionDaemon(std::string configFile, QObject *parent=nullptr);
	~HyperionDaemon();
	
	void loadConfig(const std::string & configFile);
	void run();

	void startInitialEffect();
	void createKODIVideoChecker();
	void startNetworkServices();

	// grabber creators
	void createGrabberV4L2();
	void createSystemFrameGrabber();

private:
	void createGrabberDispmanx(const Json::Value & grabberConfig);
	void createGrabberAmlogic(const Json::Value & grabberConfig);
	void createGrabberFramebuffer(const Json::Value & grabberConfig);
	void createGrabberOsx(const Json::Value & grabberConfig);
	void createGrabberX11(const Json::Value & grabberConfig);

	Logger*             _log;
	Json::Value         _config;
	KODIVideoChecker*   _kodiVideoChecker;
	JsonServer*         _jsonServer;
	ProtoServer*        _protoServer;
	BoblightServer*     _boblightServer;
	UDPListener*        _udpListener;
	V4L2Wrapper*        _v4l2Grabber;
	DispmanxWrapper*    _dispmanx;
	X11Wrapper*         _x11Grabber;
	AmlogicWrapper*     _amlGrabber;
	FramebufferWrapper* _fbGrabber; 
	OsxWrapper*         _osxGrabber;
	WebConfig*          _webConfig;
	Hyperion*           _hyperion;
};
