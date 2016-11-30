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
#include <udplistener/UDPListener.h>
#include <QJsonObject>

class HyperionDaemon : public QObject
{
public:
	HyperionDaemon(QString configFile, QObject *parent=nullptr);
	~HyperionDaemon();
	
	void loadConfig(const QString & configFile);
	void run();

	void startInitialEffect();
	void createKODIVideoChecker();
	void startNetworkServices();

	// grabber creators
	void createGrabberV4L2();
	void createSystemFrameGrabber();

private:
	void createGrabberDispmanx();
	void createGrabberAmlogic();
	void createGrabberFramebuffer(const QJsonObject & grabberConfig);
	void createGrabberOsx(const QJsonObject & grabberConfig);
	void createGrabberX11(const QJsonObject & grabberConfig);

	Logger*             _log;
	QJsonObject         _qconfig;
	KODIVideoChecker*   _kodiVideoChecker;
	JsonServer*         _jsonServer;
	ProtoServer*        _protoServer;
	BoblightServer*     _boblightServer;
	UDPListener*        _udpListener;
	std::vector<V4L2Wrapper*>  _v4l2Grabbers;
	DispmanxWrapper*    _dispmanx;
#ifdef ENABLE_X11
	X11Wrapper*         _x11Grabber;
#endif
	AmlogicWrapper*     _amlGrabber;
	FramebufferWrapper* _fbGrabber; 
	OsxWrapper*         _osxGrabber;
	Hyperion*           _hyperion;
	
	unsigned            _grabber_width;
	unsigned            _grabber_height;
	unsigned            _grabber_frequency;
	int                 _grabber_priority;
	unsigned            _grabber_cropLeft;
	unsigned            _grabber_cropRight;
	unsigned            _grabber_cropTop;
    unsigned            _grabber_cropBottom;
};
